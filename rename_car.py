import re
import sys


MAX_NAME_LEN_LINE_REGEX = r'#define MAX_LEN_GAP_DEVICE_NAME\s+0x\d+'
DEVICE_NAME_LINE_REGEX = r'(uint8_t app_gap_device_name\[\] = {).*(};\n)'
CURLEN_LINE_REGEX = r'(\s+)\d+(,\s+/\* curlen \*/\n)'
DEVICE_NAME_LEN_LINE_REGEX = r'(const uint16_t app_gap_device_name_len = )\d(;\n)'
ADV_PACKET_LINE_REGEX = r'(const uint8_t cy_bt_adv_packet_elem_1\[)\d+(\] = { ).*( };\n)'
ADV_NAME_COMMENT_LINE_REGEX = r'\s*/\* Complete local name \*/\n'
ADV_NAME_LEN_LINE_REGEX = r'(\s*\.len = )\d+(,\s*\n)'


def replace_name(name: str) -> None:
    with open("GeneratedSource/cycfg_gatt_db.h", "r") as f:
        lines = f.readlines()

    # Get the maximum device name
    max_len_line = next(filter(re.compile(MAX_NAME_LEN_LINE_REGEX).match, lines))
    max_len = int(re.search(r'0x\d+', max_len_line).group(0), 16)
    # Verify name is a valid length
    if len(name) > max_len:
        print(f"\033[1;31mInvalid name. Must be {max_len} characters at most\033[0m")
        exit(1)

    with open("GeneratedSource/cycfg_gatt_db.c", "r+") as f:
        lines = f.readlines()
        n_replaced = 0
        # Update lines containing the device name and its length
        for i, l in enumerate(lines):
            if match := re.match(DEVICE_NAME_LINE_REGEX, l):
                _l = match.groups()
                split_name = "'" + "', '".join(name) + "'"
                lines[i] = f"{split_name}, '\\0',".join(_l)
                n_replaced += 1
            elif (match := re.match(CURLEN_LINE_REGEX, l)) or (match := re.match(DEVICE_NAME_LEN_LINE_REGEX, l)):
                _l = match.groups()
                lines[i] = str(len(name)).join(_l)
                n_replaced += 1
                if n_replaced >= 3:
                    break
        else:
            # If we didn't find and replace all lines of interest, panic
            print("\033[1;31mUnable to find and replace name and its length in generated source\033[0m")
            exit(1)
        # Rewrite the file
        f.truncate(0)
        f.seek(0)
        f.writelines(lines)

    with open("GeneratedSource/cycfg_gap.c", "r+") as f:
        lines = f.readlines()
        n_replaced = 0
        name_struct_next = False
        # Update lines containing the raw device name in bytes and its length
        for i, l in enumerate(lines):
            if match := re.match(ADV_PACKET_LINE_REGEX, l):
                _l = match.groups()
                hex_name = ', '.join('0x{:02x}'.format(ord(b)) for b in name)
                lines[i] = _l[0] + str(len(name)) + _l[1] + hex_name + _l[2]
                n_replaced += 1
            elif (match := re.match(ADV_NAME_COMMENT_LINE_REGEX, l)) or (match := re.match(ADV_NAME_LEN_LINE_REGEX, l)):
                # Abuse := notation to make hidden switching logic for entering/exiting the local name struct
                if name_struct_next := not name_struct_next:
                    continue
                _l = match.groups()
                lines[i] = str(len(name)).join(_l)
                n_replaced += 1
                if n_replaced >= 2:
                    break
        else:
            # If we didn't find and replace both lines of interest, panic
            print("\033[1;31mUnable to find and replace advertised name and its length in generated source\033[0m")
            exit(1)
        # Rewrite the file
        f.truncate(0)
        f.seek(0)
        f.writelines(lines)


if __name__ == '__main__':
    replace_name(' '.join(sys.argv[1:]))

"""
@file process_mp3.py
@author James Vollmer (jrvollmer@wisc.edu) - Team 01
@brief Python script to convert MP3 files to an array of samples
"""
import librosa
import matplotlib.pyplot as plt
import numpy as np
import re
from argparse import ArgumentParser
from itertools import batched


def generate_dma_modus_design(n_samples, channel, file_path):
    INDENT = ' '*4
    MAX_DESCRIPTOR_SAMPLES = 256

    assert 1 <= n_samples <= 65536, f"Invalid number of samples: {n_samples}"

    n_descriptors, n_samples_last_desc = divmod(n_samples, MAX_DESCRIPTOR_SAMPLES)
    n_descriptors += 1

    lines = [
        f'{INDENT*4}<Personality template="dma" version="3.0">\n',
        f'{INDENT*5}<Block location="cpuss[0].dw1[0].chan[{channel}]" locked="true"/>\n',
        f'{INDENT*5}<Parameters>\n',
        f'{INDENT*6}<Param id="BUFFERABLE" value="false"/>\n'
    ]
    lines.extend([
        f'{INDENT*6}<Param id="CHAIN_TO_{i}" value="{(i + 1) % n_descriptors}"/>\n'
        for i in range(n_descriptors)
    ])
    lines.append(f'{INDENT*6}<Param id="CHANNEL_PRIORITY" value="3"/>\n')
    lines.extend([
        (
            f'{INDENT*6}<Param id="CHAN_STATE_COMPL_{i}" value="CY_DMA_CHANNEL_ENABLED"/>\n'
            if i < (n_descriptors - 1) else
            f'{INDENT*6}<Param id="CHAN_STATE_COMPL_{i}" value="CY_DMA_CHANNEL_DISABLED"/>\n'
        )
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="CRC_{i}" value="false"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="CRC_DATA_REVERSE" value="false"/>\n',
        f'{INDENT*6}<Param id="CRC_DATA_XOR" value="0"/>\n',
        f'{INDENT*6}<Param id="CRC_POLYNOMIAL" value="79764919"/>\n',
        f'{INDENT*6}<Param id="CRC_REMINDER_REVERSE" value="false"/>\n',
        f'{INDENT*6}<Param id="CRC_REMINDER_XOR" value="0"/>\n'
    ])
    lines.extend([
        f'{INDENT*6}<Param id="DATA_TRANSFER_WIDTH_{i}" value="WordToWord"/>\n'
        for i in range(n_descriptors)
    ])
    lines.append(f'{INDENT*6}<Param id="DESCR_SELECTION" value="0"/>\n')
    lines.extend([
        f'{INDENT*6}<Param id="ENABLE_CHAINING_{i}" value="true"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="INTR_OUT_{i}" value="CY_DMA_1ELEMENT"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="NUM_OF_DESCRIPTORS" value="{n_descriptors}"/>\n',
        f'{INDENT*6}<Param id="PREEMPTABLE" value="false"/>\n'
    ])
    lines.extend([
        f'{INDENT*6}<Param id="TRIG_DEACT_{i}" value="CY_DMA_RETRIG_IM"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="TRIG_IN_TYPE_{i}" value="CY_DMA_1ELEMENT"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="TRIG_OUT_TYPE_{i}" value="CY_DMA_1ELEMENT"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="X_DST_INCREMENT_{i}" value="0"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        (
            f'{INDENT*6}<Param id="X_NUM_OF_ELEMENTS_{i}" value="{MAX_DESCRIPTOR_SAMPLES}"/>\n'
            if i < (n_descriptors - 1) else
            f'{INDENT*6}<Param id="X_NUM_OF_ELEMENTS_{i}" value="{n_samples_last_desc}"/>\n'
        )
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="X_SRC_INCREMENT_{i}" value="1"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="Y_DST_INCREMENT_{i}" value="1"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="Y_NUM_OF_ELEMENTS_{i}" value="1"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="Y_SRC_INCREMENT_{i}" value="1"/>\n'
        for i in range(n_descriptors)
    ])
    lines.extend([
        f'{INDENT*6}<Param id="inFlash" value="true"/>\n',
        f'{INDENT*5}</Parameters>\n',
        f'{INDENT*4}</Personality>\n'
    ])

    with open(file_path, "w+") as f:
        f.writelines(lines)


def process_mp3(mp3_file, sampling_rate, out_file, duration, dma_channel):
    # Process the audio file to get an array of sample points
    _, sr_auto = librosa.load(mp3_file, sr=None, duration=duration)
    s, sr = librosa.load(mp3_file, sr=sampling_rate, duration=duration)
    print("Initial sample count:", len(s))

    # Scale and offset samples to be within [0, 4095]
    rng = max(s) - min(s)
    s = np.astype(np.round(4095.0 * s / rng), np.int32)
    s = s[s != 0] - min(s)
    print("Trimmed sample count:", len(s))
    print("Auto sample rate:", sr_auto)
    print("Used sample rate:", sr)
    
    # Write samples and required info to header file
    lines = [
        f'#define N_SAMPLES           ({len(s)})\n',
        f'#define SAMPLE_RATE         ({sr}UL)\n',
        f'#define SAMPLE_RATE_AUTO    ({sr_auto}UL)\n',
        '\n',
        'uint32_t soundByteSamplesLUT[] = {\n'
    ]
    for sb in batched(s, n=100):
        lines.append('    ' + ', '.join([f'0x{_s:03x}' for _s in sb]) + ',\n')
    # Remove trailing comma
    lines[-1] = lines[-1][:-2] + '\n'
    lines.append('};\n')

    out_path_base = re.sub(r'\..*$', '', out_file)
    with open(out_path_base + '.h', "w+") as f:
        f.writelines(lines)

    generate_dma_modus_design(len(s), dma_channel, out_path_base + '-dma_design._modus')

    # Plot for manual validation of waveform
    plt.plot(range(len(s)), s)
    plt.show()


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("-f", "--mp3-file", type=str, help="Path to the mp3 file to process")
    parser.add_argument("-o", "--output-file", type=str, default="./samples.h", help="Path to the output header file (extension is optional)")
    parser.add_argument("-r", "--sample-rate", choices=range(1, 20001), type=int, default=20000, help="Sampling rate when processing the audio file")
    parser.add_argument("-d", "--duration", type=float, default=None, help="How many seconds of the audio file to keep in sample")
    parser.add_argument("-c", "--dma-channel", type=int, choices=range(0, 16), default=None, help="Total number of samples to keep. ")
    args = parser.parse_args()

    process_mp3(args.mp3_file, args.sample_rate, args.output_file, args.duration, args.dma_channel)

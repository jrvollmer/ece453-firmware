# Bucky Kart Firmware
Firmware for the Bucky Kart project. Runs on the PSoC 6 BLE

### Naming Cars
When building or programming, you can specify a car/BLE server name using the following syntax: `make <build|program> CAR=<car_name>`

### Some Notes About CLI Conflicts
Our CLI makes `printf()` calls problematic. Some such calls are located at the bottom of `mtb_shared/mtb-pdl-cat1/release-v3.11.1/drivers/source/cy_ipc_bt.c`. If you encounter issues with running this firmware, you may need to comment out these lines.

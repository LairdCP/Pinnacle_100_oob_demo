# Pinnacle 100 Out of Box Demo

> **Note:** Not recommended for new designs. Use https://github.com/LairdCP/Pinnacle-100-Firmware.git.

Download firmware releases from [here!](https://github.com/LairdCP/Pinnacle_100_oob_demo/releases)

The Pinnacle 100 Out of Box Demo (OOB Demo) is designed to showcase gathering sensor data over BLE and transferring it to the cloud. The demo can operate in two modes:

- [LTE-M and AWS](#lte-m-and-aws)
- [NB-IoT and LwM2M](#nb-iot-and-lwm2m)

These two modes are selected at compile time. See the following sections for documentation on the demo and how it operates.

## LTE-M and AWS

The default [build task](.vscode/tasks.json) is setup to build the demo source code for LTE-M and AWS operation. [Read here](docs/readme_ltem_aws.md) for details on how the demo operates.

## NB-IoT and LwM2M

The OOB Demo can be compiled to work with NB-IoT and LwM2M communication to the cloud with the `build lwm2m` task in [tasks.json](.vscode/tasks.json).

For more details on the LwM2M demo, [read here](docs/readme_nbiot_lwm2m.md).

## Firmware Updates

[Read here](docs/firmware_update.md) for instructions on how to update firmware running on the Pinnacle 100 modem.

## Development

### Cloning and Building the Source

This is a Zephyr-based repository, **DO NOT** `git clone` this repo. To clone and build the project properly, please see the instructions in the [Pinnacle_100_OOB_Demo_Manifest](https://github.com/LairdCP/Pinnacle_100_OOB_Demo_Manifest) repository.

### BLE Profiles

Details on the BLE profiles used to interface with the mobile app can be found [here](docs/ble.md)

### Development and Debug

See [here](docs/development.md) for details on developing and debugging this app.

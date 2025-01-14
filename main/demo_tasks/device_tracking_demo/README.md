# Device Tracking Demo App

## Preface

This demo app is part of the broader iot-reference-esp32c3 project. Please see the project level
[README](../../../README.md) for context.

## Overview

TODO

## Usage

Please see [USAGE](USAGE.md) for information on how to use (not build) this demo app.

## Building

Please see [BUILDING](BUILDING.md) for information on how to build (not use) this demo app.

## Introduction

This repository contains a project that demonstrates how to integrate FreeRTOS modular software libraries with the hardware capabilities of [Espressif SoCs](https://www.espressif.com/en/products/socs/) and the enhanced security capabilities.
The project contains reference implementations that demonstrate IoT application tasks that run concurrently and communicate with enhanced security with [AWS IoT](https://aws.amazon.com/iot-core/). The implementation also shows how to perform over-the-air firmware updates that use the [AWS IoT OTA service](https://docs.aws.amazon.com/freertos/latest/userguide/freertos-ota-dev.html) and the secure bootloader capabilities of Secure Boot V2.

The reference implementation is tested to run on the following IoT development boards:
1. ESP32
2. ESP32-C3
3. ESP32-S3

See the [Featured IoT Reference Integration page](https://www.freertos.org/featured-freertos-iot-integration-targeting-an-espressif-esp32-c3-risc-v-mcu) on FreeRTOS.org for more details about the DS peripheral, Secure Boot and OTA.

## Cloning the Repository

To clone using HTTPS:

```
 git clone https://github.com/FreeRTOS/iot-reference-esp32c3.git --recurse-submodules
```

Using SSH:

```
 git clone git@github.com:FreeRTOS/iot-reference-esp32c3.git --recurse-submodules
```

If you have downloaded the repo without using the --recurse-submodules argument, you should run:

```
git submodule update --init --recursive
```

## Demos

This repository currently supports 3 demos implemented as FreeRTOS [tasks](https://www.freertos.org/taskandcr.html), each of which utilize the same MQTT connection. The demos use the [coreMQTT](https://www.freertos.org/mqtt/index.html) library, while the [coreMQTT-Agent](https://www.freertos.org/mqtt-agent/index.html) library is employed to manage thread safety for the MQTT connection. The demos are the following:

* **ota_over_mqtt_demo**: This demo uses the [AWS IoT OTA service](https://docs.aws.amazon.com/freertos/latest/userguide/freertos-ota-dev.html) for FreeRTOS to configure and create OTA updates. The OTA client software on the ESP32-C3 uses the [AWS IoT OTA library](https://www.freertos.org/ota/index.html) and runs in the background within a FreeRTOS agent (or daemon) task. A new firmware image is first signed and uploaded to the OTA service, and the project is then configured to store the corresponding public key certificate. The demo subscribes to, and listens on, an OTA job topic in order to be notified of an OTA update. Upon receiving notification of a pending OTA update, the device downloads the firmware patch and performs code signature verification of the downloaded image by using the public key certificate. On successful verification, the device reboots and the updated image is activated. The OTA client then performs a self-test on the updated image to check for its integrity.
* **sub_pub_unsub_demo**: The demo creates tasks which subscribe to a topic on AWS IoT Core, publish a constant string to the same topic, receive their publish (since they are subscribed to the topic they publish to), and then unsubscribe from the topic in a loop.
* **temp_sub_pub_and_led_control_demo**: This demo creates a task which subscribes to a topic on AWS IoT Core. This task then reads the temperature from the onboard temperature sensor, publishes this information in JSON format to the same topic, and then receives this publish (since it is subscribed to the same topic it just published to) in a loop. This demo also enables a user to send a JSON packet back to the device to turn an LED off or on.

All three demos can be selected to run together concurrently as separate tasks.

## Getting started with the demos

To get started and run the demos, follow the [Getting Started Guide](GettingStartedGuide.md).

## Contributing

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more information.

## License

Example source code under ./main/ is licensed under the MIT-0 License. See the [LICENSE](LICENSE) file. For all other source code licenses including components/, see source header documentation.

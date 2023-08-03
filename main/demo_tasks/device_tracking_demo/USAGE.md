# Using the Device Tracking Demo App

## Preface

This demo app is part of the broader iot-reference-esp32c3 project. Please see the project level
[README](../../../README.md) for context.

## Introduction

Once one has a built image (see [BUILDING](BUILDING.md)), it can be flashed to a device and then the Device Tracking
demo app running on the device will stream its location (GPS points) to AWS IoT.

## Overview

The Device Tracking demo app is part of the broader iot-reference-esp32c3 project and therefore inherits its
instructions (see [Getting Started Guie](../../../GettingStartedGuide.md)) with the following exceptions and
additions:

Ignore the specific references to the ESP32-C3 model - this fork is intended to be compatible with all ESP32 models.

By its nature, this demo app is only interesting on devices with an inertial measurement unit (IMU, aka accelerometer)
or GPS hardware. For example, an [M5stack Core2](https://shop.m5stack.com/products/m5stack-core2-esp32-iot-development-kit).
Most [Espressif development boards](https://www.espressif.com/en/products/devkits) do not have this hardware.

This fork defaults to using the device MAC address as the MQTT client id (a.k.a. thing name) rather than a hard-coded
value so that a single built image can be used by many devices without client id conflict. The project-level
instructions imply you can setup the cloud-side with a thing name of your choice, but unless you have a built image
customized with your custom thing name you will need to use your device's MAC address (all caps) - see
[Client Identifier](#ClientIdentifier) below.

## Steps

### Setup AWS IoT Core

Use the project level [Setup AWS IoT Core instructions](../../../GettingStartedGuide.md#21-setup-aws-iot-core)
instructions to configure the cloud-side, including creation of an AWS IoT thing and its cert/key.

### Provision Device Cert/Key

Use the project level [Provision...](../../../GettingStartedGuide.md#23-provision-the-esp32-c3-with-the-private-key-device-certificate-and-ca-certificate-in-development-mode)
instructions to write the device cert/key to a persistent partition of the device's flash.

Implied in the instructions is availability of the configure_esp_secure_cert.py script. If you have built the image
then it will be in the managed_components/espressif__esp_secure_cert_mgr/tools directory of your project. Otherwise,
you may need to clone or download the [esp_secure_cert_mgr](https://github.com/espressif/esp_secure_cert_mgr) project.

### USB Driver

The next step requires the device to be connected via USB. This will likely require a USB driver for your device.

For example:

* [M5stack Core2 USB Drivers](https://docs.m5stack.com/en/core/core2#:~:text=the%20bottom%20side-,USB%20drive,-Before%20using%2C%20please)
* [M5stack AWS EduKit USB Drivers](https://docs.m5stack.com/en/core/core2_for_aws#:~:text=Plastic%20(PC%20)-,Driver%20Installation,-Click%20the%20link)

### Flash

Use the project level [Build and Flash...](../../../GettingStartedGuide.md#3-build-and-flash-the-demo-project)
instructions to flash the image to the device and monitor its serial output.

Implied in the instructions is availability of the idf.py script that is part of the [Espressif IoT Development
Framework (ESP-IDF)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html). One *does* need the
ESP-IDF, but does not necessarily need to have a full iot-reference-esp32c3 project directory. After installing the
ESP-IDF (for example, using the basic [ESP-IDF Windows Installer](https://dl.espressif.com/dl/esp-idf/)), a command
similar to the following can be used to flash and monitor the device. Customize the paths and COM port as needed.
The referenced .bin files constitute the built image.

```
C:\Espressif\python_env\idf5.1_py3.11_env\Scripts\python.exe C:\Espressif\frameworks\esp-idf-v5.1\components\esptool_py\esptool\esptool.py -p COM4 -b 460800 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size 4MB --flash_freq 40m 0x1000 build\bootloader\bootloader.bin 0xb000 build\partition_table\partition-table.bin 0x19000 build\ota_data_initial.bin 0x20000 build\FeaturedFreeRTOSIoTIntegration.bin
```

### Device-Side Monitoring

Use the project level [Monitoring...](../../../GettingStartedGuide.md#4-monitoring-the-demo) instructions to
observe the device serial output.

Assuming the device has not yet undergone WiFi Provisioning, you will need to use the ESP BLE Prov app plus the URL
output via serial. The latter will provide a QR code for the former, allowing the app to send your WiFi network
settings to the device via Bluetooth. This is a one-time procedure.

The following are points of interest to observe in the serial output.

Device Tracking demo app launch:
```
I (1393) device_tracking: Launching Device Tracking app...
I (1403) device_tracking: AWS IoT SDK coreMQTT version: v2.1.1
I (1403) device_tracking: AWS IoT MQTT Client ID: D4:D4:DA:5C:B1:C0
```

Device Tracking demo app init:
```
I (2833) device_tracking: Creating GPS points queue with depth 3000 items = 10 min...
I (2833) device_tracking: Creating task to produce GPS points...
I (2833) device_tracking: Initializing SNTP service with 'pool.ntp.org'
I (2843) device_tracking: Waiting up to 60 seconds for SNTP sync...
I (2853) device_tracking: Creating task to publish GPS points...
```

General project WiFi and MQTT broker connect:
```
I (6443) app_wifi: Connected with IP Address:192.168.68.51
I (6443) esp_netif_handlers: sta ip: 192.168.68.51, mask: 255.255.252.0, gw: 192.168.68.1
I (6443) core_mqtt_agent_manager: WiFi connected.
I (6453) main_task: Returned from app_main()
I (8973) coreMQTT: MQTT connection established with the broker.
```

Device Tracking demo app SNTP sync complete:
```
I (33063) device_tracking: ...SNTP sync complete.
I (33063) device_tracking: Current date/time: Tue Aug 01 16:25:06 2023 UTC
```

Device Tracking demo app sending GPS points:
```
I (33113) iot: Publishing MQTT Message: [D4:D4:DA:5C:B1:C0/location] { "SampleTime": 1690907106, "Position": [ -93.275024, 44.984210 ] }
I (33123) coreMQTT: Publishing message to D4:D4:DA:5C:B1:C0/location.
```

### Cloud-Side Monitoring

At this point, the device is publishing GPS points to the MQTT topic '[client-id]/location' within the AWS account
associated with the MQTT Endpoint embedded in the device image.

One can use the [AWS IoT Core Test Client](https://docs.aws.amazon.com/iot/latest/developerguide/view-mqtt-messages.html)
(within the AWS Management Console) to observe the GPS point messages.

### Cloud-Side Integration

One can now create an [AWS IoT Core Rule](https://docs.aws.amazon.com/iot/latest/developerguide/iot-rules.html) as
desired, for example to propagate the GPS points to the [Amazon Location Service](https://aws.amazon.com/pm/location/).

Another fine choice would be to propagate the GPS points into the excellent [Pet Tracker](https://catalog.workshops.aws/how-to-build-a-pet-tracker/en-US)
application, which can display the device's location in real-time on a map within a web app. With the workshop built,
create an AWS IoT Core Rule as follows:

```
SELECT "pettracker" AS id, get(Position, 0) AS lng, get(Position, 1) as lat, parse_time("yyyy-MM-dd'T'HH:mm:ss.SSS'Z'", SampleTime*1000) AS timestamp FROM '+/location'
```

## Client Identifier

Build-time configuration dictates whether the image will use a hard-coded client identifier or the device MAC address
(ex: "A0:B1:C2:D3:E4"). The client id used by the device must match the thing registration within AWS IoT Core.

If using the device MAC address: Perhaps the device MAC address is on a label on the device or otherwise known.
Otherwise, one can run the built image and while it will not successfully connect to AWS IoT Core it will get far
enough to display its client identifier in the serial output, which can then be used to complete the AWS IoT Core
setup.

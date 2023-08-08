# Microvisor test image sample project

This repo shows an example of Microvisor test image. Microvisor test image runs in the final stage of the [Microvisor factory flow](https://www.twilio.com/docs/iot/microvisor/manufacturing#the-factory-flow) and its purpose is to test the device- and application-specific hardware after the manufacturing. As such it is a part of [DUT SPI flash image](https://www.twilio.com/docs/iot/microvisor/manufacturing#the-pre-factory-flow) you need to give to [Microvisor manufacturing fixture](TODO: link)

Every test image is as unique as the device it is testing. Some devices will be able to fully self-test, while others with require a bespoke test rig. Some will be able to connect to Microvisor cloud, and from there to other remote endpoints, while others, e.g. those equipped with a cellular modem that does not support the region device is manufactured it, will have to stay offline. Some test processes will be fully automated, while others will need a technician interacting with the device. The set of peripherals you use during the test will also be unique to your device.

Whatever your test procedure looks like, the test image will need to be in control of it. It is up to the test image to signal to the kernel (and eventualy the [test fixture](TODO: link)) whether the test has been successful. It does so by issuing `mvTestingComplete(uint32_t result)` [Microvisor call](https://www.twilio.com/docs/iot/microvisor/syscalls/factory). It should be the last call in the testing procedure, there is no guarantee that any code after it will run.

Successful test is indicated by `mvTestingComplete(0)`, Microvisor will then proceed to run the production application contained in the [SPI flash image](https://www.twilio.com/docs/iot/microvisor/manufacturing#the-pre-factory-flow).

Test failure is indicated by any non-zero value of the `result` parameter. The value is opaque to Microvisor and is shown to the operator of the [test fixture](TODO: link) as is. It can serve as a coarse indicator of the failure reason in case no other means of interaction are available.

## Microvisor test image environment

Microvisor test image is in most regards like a normal Microvisor application. During the development you can deploy it in the same way you would [deploy a normal application](https://www.twilio.com/docs/iot/microvisor/applications-and-bundles#create-and-upload-bundles) to run in Microvisor application environment. When [run as a part of the manufacturing process](https://www.twilio.com/docs/iot/microvisor/manufacturing#the-microvisor-test) the enviromnent is sligtly different though.

1. `mvTestingComplete()` call becomes available. The application (or test image run in application environment) can call it too, but it will have no effect and return `MV_STATUS_UNAVAILABLE`.
2. Manufacturing console requires provisioning console to be available. For that it needs to reserve some peripherals to the secure space, respectively making then unavailable to the application. These include:
    * GPDMA channels 4 and 5
    * Pins PB0 and PE4
    * EXTI4 interrupt line
    * LPTIM2 timer

## Building the test image

We currently support the following build platforms:

* Linux — native development under [Ubuntu 20.0.4](#build-in-ubuntu).
* Windows — development in [Ubuntu 20.0.4 via Windows Subsystem for Linux 2](#build-under-windows).
* Mac — development via [Docker container](#build-with-docker).

## Build under Windows

The recommended solution for working with Microvisor on Windows is via [Windows Subsystem for Linux (WSL)](https://learn.microsoft.com/en-us/windows/wsl/install).

You will need Administrator privileges to install WSL.

### Install WSL

1. Open an Administrator-level Powershell instance.
1. Run `wsl --install -d ubuntu`.
1. Open the Ubuntu instance shell from your Start menu and [follow the Ubuntu instructions below](#build-in-ubuntu).

## Build with Docker

1. Build the image:

```shell
docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t microvisor-hardware-test-image .
```

2. Edit `env.list` to set account credentials and (optionally) device SID. 
3. Run the image:

```shell
docker run -it --rm -v $(pwd)/:/home/mvisor/project/ \
  --env-file env.list \
  --name microvisor-hardware-test microvisor-hardware-test-image
```

Under Docker, the image is built, signed by the cloud and written to `artifacts` directory.  If device SID is set the image will also be deployed over the air to the device to run as a normal application, and debug stream will be open.

## Build in Ubuntu

### Install Libraries and Tools

Under Ubuntu, run the following:

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi \
  git curl build-essential cmake libsecret-1-dev jq openssl
```

### Twilio CLI

Install the Twilio CLI. This is required to sign test bundle, view streamed logs and for remote debugging. You need version 4.0.1 or above.

**Note** If you have already installed the Twilio CLI using *npm*, we recommend removing it and then reinstalling as outlined below. Remove the old version with `npm remove -g twilio-cli`.

```bash
wget -qO- https://twilio-cli-prod.s3.amazonaws.com/twilio_pub.asc | sudo apt-key add -
sudo touch /etc/apt/sources.list.d/twilio.list
echo 'deb https://twilio-cli-prod.s3.amazonaws.com/apt/ /' | sudo tee /etc/apt/sources.list.d/twilio.list
sudo apt update
sudo apt install -y twilio
```

Close your terminal window or tab, and open a new one. Now run:

```bash
twilio plugins:install @twilio/plugin-microvisor
```

The process outlined below requires Plugin 0.3.11 or above.

### Set Environment Variables

Running the Twilio CLI and the Microvisor Plugin for uploading the built code to the Twilio cloud and subsequent deployment to your Microvisor Nucleo Board uses the following Twilio credentials stored as environment variables. They should be added to your shell profile:

```bash
export TWILIO_ACCOUNT_SID=ACxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TWILIO_AUTH_TOKEN=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export MV_DEVICE_SID=UVxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

You can get the first two from your Twilio Console [account dashboard](https://console.twilio.com/).

Enter the following command to get your target device’s SID and, if set, its unique name:

```bash
twilio api:microvisor:v1:devices:list
```

It is also accessible via the QR code on the back of your development board. Scan the code with your mobile phone and a suitable app, and the board’s SID is the third `/`-separated field.

### Build and test the demo

#### Build with CMake

```bash
cd twilio-microvisor-hardware-test-demo
mkdir build
cd build
cmake ..
make -j$(nproc)
```

#### Bundle

Binary needs to be bundled before it can be deployed to Microvisor cloud
```bash
twilio microvisor apps bundle ./HardwareTest/hardware_test_sample.bin ./HardwareTest/hardware_test_sample.bundle
```

#### Deploy for testing

First application needs to be created on the cloud.

```bash
APPLICATION_SID=$(twilio microvisor apps create ./HardwareTest/hardware_test_sample.bundle -o json | jq -r .[0].sid)
```

Next it needs to be assigned to a device.

```bash
twilio api:microvisor:v1:devices:update --sid $MV_DEVICE_SID --target-app $APPLICATION_SID
```

#### View Log Output

You can stream logs from the test application as following.

```bash
twilio microvisor logs stream $MV_DEVICE_SID}
```

### Creating manufacturing artifact
As device might not be able to connect to Microvisor cloud in the manufacturing facility, test image should come to the factory fixture pre-signed. Once you're done developing and testing you image, you can bundle it as usual:

```bash
twilio microvisor apps bundle ./build/HardwareTest/hardware_test_sample.bin ./build/HardwareTest/hardware_test_sample.bundle
```

That creates an unsigned bundle. You can then have it signed by the cloud with:

```bash
twilio microvisor apps create ./build/HardwareTest/hardware_test_sample.bundle --bundle-out ./build/HardwareTest/hardware_test_sample.signed.bundle
```

Note that it's the same command that creates a normal application, but with an additional parameter to save the signed bundle. The bundle will be uploaded to Microvisor cloud and will be available to your devices as an application to continue running the tests with it.

## Support/Feedback

Please contact [Twilio Support](https://support.twilio.com/).

## Copyright

The sample code is © 2023, Twilio, Inc.

FreeRTOS is © 2021, Amazon Web Services, Inc

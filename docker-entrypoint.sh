#!/bin/bash
set -e

cd $(dirname $0)

[ -d build ] && rm -rf build

mkdir build

cd build
cmake ..
make -j$(nproc)

twilio microvisor apps bundle ./HardwareTest/hardware_test_sample.bin ./HardwareTest/hardware_test_sample.bundle

if [[ -z "${MV_GA}" ]]; then
  [ -d artifacts ] && rm -rf artifacts

  mkdir artifacts

  # Deploy -- requires env vars for device SID and Twilio creds to be set
  twilio microvisor apps create ./HardwareTest/hardware_test_sample.bundle --bundle-out ../artifacts/apptest.bundle
fi

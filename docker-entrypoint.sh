#!/bin/bash
set -e

cd $(dirname $0)

[ -d build ] && rm -rf build

mkdir build

cd build
cmake ..
make -j$(nproc)

twilio microvisor apps bundle ./HardwareTest/ndb_hardware_test.bin ./HardwareTest/ndb_hardware_test.zip

if [[ -z "${MV_GA}" ]]; then
  [ -d artifacts ] && rm -rf artifacts

  mkdir artifacts

  # Deploy -- requires env vars for device SID and Twilio creds to be set
  twilio microvisor apps create ./HardwareTest/ndb_hardware_test.zip --bundle-out ../artifacts/ndb_hardware_test.signed.zip
fi

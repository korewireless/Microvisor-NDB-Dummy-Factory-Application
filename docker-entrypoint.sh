#!/bin/bash
set -e

cd $(dirname $0)

[ -d build ] && rm -rf build
[ -d artifacts ] && rm -rf artifacts

echo ${MV_GA}

if [[ -n "${MV_GA}" ]]; then
  # Build only
  twilio microvisor:deploy . -b
else 
  # Build and deploy -- requires env vars for device SID and Twilio creds to be set
  cd build
  cmake ..
  make -j$(nproc)

  twilio microvisor apps bundle ./HardwareTest/hardware_test_sample.bin ./HarwareTest/hardware_test_sample.bundle
  twilio microvisor apps create ./HardwareTest/hardware_test_sample.bundle --bundle-out ../artifacts/apptest.bundle
fi

#!/bin/bash
set -e

cd $(dirname $0)

[ -d build ] && rm -rf build

mkdir build

cd build
cmake ..
make -j$(nproc)

twilio microvisor apps bundle ./App/ndb_dummy_application.bin ./App/ndb_dummy_application.zip

if [[ -z "${MV_GA}" ]]; then
  [ -d artifacts ] && rm -rf artifacts

  mkdir artifacts

  # Deploy -- requires env vars for device SID and Twilio creds to be set
  twilio microvisor apps create ./App/ndb_dummy_application.zip --bundle-out ../artifacts/ndb_dummy_application.signed.zip
fi

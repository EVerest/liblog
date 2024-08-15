#!/bin/sh

set -e

cmake \
    -B build \
    -S "$EXT_MOUNT/source" \
    -G Ninja \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE_PATH/dist"

ninja -j$(nproc) -C build install

trap "cp build/Testing/Temporary/LastTest.log /ext/ctest-report" EXIT
trap "cp -R build/everest-log_gcovr_coverage /ext/gcovr_coverage" EXIT

ninja -j$(nproc) -C build test

pip install gcovr==6

ninja -j$(nproc) -C build everest-log_gcovr_coverage

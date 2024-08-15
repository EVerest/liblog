#!/bin/sh

set -e

copy_logs() {
    cp build/Testing/Temporary/LastTest.log /ext/ctest-report
    cp -R build/everest-log_gcovr_coverage /ext/gcovr_coverage
}

cmake \
    -B build \
    -S "$EXT_MOUNT/source" \
    -G Ninja \
    -DBUILD_TESTING=ON \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="$WORKSPACE_PATH/dist"

ninja -j$(nproc) -C build install

trap "copy_logs" ERR

ninja -j$(nproc) -C build test

pip install gcovr==6

ninja -j$(nproc) -C build everest-log_gcovr_coverage

copy_logs

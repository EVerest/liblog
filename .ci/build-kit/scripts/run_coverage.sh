#!/bin/sh

ninja -C "$EXT_MOUNT/build" everest-log_gcovr_coverage
retVal=$?

# Copy the generated coverage report to the mounted directory in any case
cp -R "$EXT_MOUNT/build/everest-log_gcovr_coverage" "$EXT_MOUNT/gcovr-coverage"

if [ $retVal -ne 0 ]; then
    echo "Coverage failed with return code $retVal"
    exit $retVal
fi

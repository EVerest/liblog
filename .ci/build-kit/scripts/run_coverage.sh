#!/bin/sh

ninja \
    -C "$EXT_MOUNT/build" \
    everest-log_gcovr_coverage \
    everest-log_gcovr_coverage_xml
retVal=$?

# Copy the generated coverage report and xml to the mounted directory in any case
cp -R "$EXT_MOUNT/build/everest-log_gcovr_coverage" "$EXT_MOUNT/gcovr-coverage"
cp "$EXT_MOUNT/build/everest-log_gcovr_coverage_xml.xml" "$EXT_MOUNT/gcovr-coverage-xml.xml"

if [ $retVal -ne 0 ]; then
    echo "Coverage failed with return code $retVal"
    exit $retVal
fi

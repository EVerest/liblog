#!/bin/sh

ninja \
    -C "$EXT_MOUNT/build" \
    everest-log_gcovr_coverage
retValHTML=$?

cat "$EXT_MOUNT/build/Testing/Temporary/LastTest.log"

ninja \
    -C "$EXT_MOUNT/build" \
    everest-log_gcovr_coverage_xml
retValXML=$?

cat "$EXT_MOUNT/build/Testing/Temporary/LastTest.log" 

# Copy the generated coverage report and xml to the mounted directory in any case
cp -R "$EXT_MOUNT/build/everest-log_gcovr_coverage" "$EXT_MOUNT/gcovr-coverage"
cp "$EXT_MOUNT/build/everest-log_gcovr_coverage_xml.xml" "$EXT_MOUNT/gcovr-coverage-xml.xml"

if [ $retValHTML -ne 0 ]; then
    echo "Coverage HTML report failed with return code $retValHTML"
    exit $retValHTML
fi

if [ $retValXML -ne 0 ]; then
    echo "Coverage XML report failed with return code $retValXML"
    exit $retValXML
fi

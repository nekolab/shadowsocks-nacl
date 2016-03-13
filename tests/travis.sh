#!/bin/bash -e

echo "Generating test binary..."
if [ "$(uname)" == "Darwin" ]; then
  dd if=/dev/urandom of=test.bin bs=1m count=10
else
  dd if=/dev/urandom of=test.bin bs=1M count=10
fi
echo

echo "Starting python server..."
python2.7 -m SimpleHTTPServer 6001 &
PYTHON_SERVER_PID=$!
echo

python2.7 tests/script.py

echo "Sweeping..."
kill $PYTHON_SERVER_PID
rm test.bin

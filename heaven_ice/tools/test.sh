#!/bin/bash -eu

if [ ! -f "$ROM" ]; then
  echo "Rom not found: $ROM"
  exit 1
fi

make release

echo "Running..."
time ./build/release/heaven_ice/heaven_ice native "$ROM" \
  --read-events events_record_in.yasf  \
  --speed 32  \
  --exit-after-playback \
  --display none  \
  --verbose > out

if [ ! -s out-before ]; then
  echo 'out-before does not exit'
  exit 1
fi

echo "Diffing..."
OUTPUT=$(mktemp)
time ./build/release/diffo/diffo diff out-before out > "$OUTPUT"

if [ -s "$OUTPUT" ]; then
  less -R "$OUTPUT"
  echo "Differences found!"
  rm "$OUTPUT"
  exit 1
fi

rm "$OUTPUT"
echo "OK!"

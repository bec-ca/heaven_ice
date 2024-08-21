#!/bin/bash -eu

make dev
./build/dev/heaven_ice/heaven_ice to-cpp "$ROM" > tmp
mv tmp heaven_ice/generated.cpp
clang-format -i heaven_ice/generated.cpp


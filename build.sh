#!/bin/bash

if [ ! -d "build" ]; then
  echo "Build directory does not exist. Creating it..."
  mkdir build
else
  echo "Build directory already exists."
fi

echo "Running cmake..."
cmake -S . -B build

echo "Done!"

cd build

make
#!/bin/bash
set -ev

mkdir build
cd build
cmake .. 
make
cd ../doc
mkdir build
cd build
cmake ..
make doc

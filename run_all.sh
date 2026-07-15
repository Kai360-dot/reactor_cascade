#!/bin/bash

mkdir -p ./logs/

echo "Running: global_sample"
./build/global_sample > ./logs/global_sample

echo "Running: inlet_box_sobol"
./build/inlet_box_sobol > ./logs/inlet_box_sobol

echo "Running: unit1_sample"
./build/unit1_sample > ./logs/unit1_sample

echo "Training ANNs"
uv run ./src/train_surrogates.py > ./logs/train_surrogates

echo "Running: unit2_sample"
./build/unit2_sample > ./logs/unit2_sample

echo "Running: mlp_check"
./build/mlp_check > ./logs/mlp_check

echo "Running: reconstruct"
./build/reconstruct > ./logs/reconstruct

echo "Running: ctest"
ctest --test-dir build/ > ./logs/ctest

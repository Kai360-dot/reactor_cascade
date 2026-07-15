#!/bin/bash
echo "Running: global_sample"
./build/global_sample

echo "Running: inlet_box_sobol"
./build/inlet_box_sobol

echo "Running: unit1_sample"
./build/unit1_sample

echo "Training ANNs"
uv run ./src/train_surrogates.py

echo "Running: unit2_sample"
./build/unit2_sample

echo "Running: mlp_check"
./build/mlp_check

echo "Running: reconstruct"
./build/reconstruct

echo "Running: ctest"
ctest --test-dir build/


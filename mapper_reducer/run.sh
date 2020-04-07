#!/bin/bash

make clean
make

echo "****MAPPER TEST START*****"
./mapper input.txt
echo "****MAPPER TEST DONE******"

echo "****COMBINER TEST START*****"
./combiner input.txt
echo "****COMBINER TEST DONE******"

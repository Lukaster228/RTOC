#!/bin/bash

base64 /dev/urandom | head -c 2000 > input.txt

g++ main.cpp -o otp

./otp -i input.txt -o output.txt -x 4212 -a 84589 -c 45989 -m 217728 
./otp -i output.txt -o input2.txt -x 4212 -a 84589 -c 45989 -m 217728

#diff input.txt input2.txt
if cmp -s input.txt input2.txt; then
    printf 'SUCCESS\n'
else
    printf 'FAIL\n'
fi


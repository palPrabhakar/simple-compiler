#!/usr/bin/bash

for f in `find . -name *.bril`; do
    result=$(bril2json < $f | ../build/sc | bril2json | ./is_ssa.py)
    if [[ $result = "yes" ]]; then
        echo -e "\e[32mPass: $f\e[0m"
    else
        echo -e "\e[31mFail: $f\e[0m"
    fi
done




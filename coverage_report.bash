#!/bin/bash
mkdir -p coverage
gcovr -r . --gcov-ignore-parse-errors --html --html-details --exclude tests/ -o coverage/coverage.html
gcovr -r . --gcov-ignore-parse-errors --xml --print-summary --exclude tests/ -o coverage/coverage.xml # This is used for the CI
bash <(curl -Ls https://coverage.codacy.com/get.sh) report -r coverage/coverage.xml

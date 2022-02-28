#!/bin/bash
mkdir -p coverage
gcovr -r . --html --html-details --exclude tests/ -o coverage/coverage.html
gcovr -r . --xml --print-summary --exclude tests/ -o coverage/coverage.xml # This is used for the CI

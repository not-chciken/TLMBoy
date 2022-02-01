#!/bin/bash
mkdir coverage
gcovr -r . --html --html-details -o coverage/coverage.html
gcovr -r . --xml --print-summary -o coverage/coverage.xml # This is used for the CI
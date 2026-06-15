#!/bin/bash
export SYSTEMC_PATH=/opt/systemc
export TLMBOY_ROOT=$(pwd)
export LD_LIBRARY_PATH=${SYSTEMC_PATH}/lib-linux64/
export SYSTEMC_DISABLE_COPYRIGHT_MESSAGE=1
if [ ! -d "./tests/golden_files" ]; then
  tar -xf ./tests/golden_files.tar.gz -C ./tests/
fi

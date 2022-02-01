#!/bin/bash
cpplint --linelength=120 --filter=-runtime/references,-build/include_subdir src/*

#!/bin/bash
CXX=${CXX:-g++}
$CXX -O2 -Wall -Iinclude src/xinfc-wsc.cpp -o xinfc-wsc

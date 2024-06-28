#!/bin/bash

# This file is intended to autoformat the program before committing the code
find include -iname *.hpp | xargs clang-format -i
find src -iname *.cpp | xargs clang-format -i
find tests -iname *.cpp | xargs clang-format -i
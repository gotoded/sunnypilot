#!/bin/bash
cd "$(dirname "$0")" || exit
git restore .
git pull
git submodule update --init --recursive
source .venv/bin/activate
scons -u -j$(nproc)

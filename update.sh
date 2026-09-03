#!/bin/bash
cd "$(dirname "$0")" || exit
git pull
git submodule update --init --recursive
source .venv/bin/activate
scons -u -j$(nproc)

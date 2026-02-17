#!/bin/bash
set -e

mkdir -p third_party
if [ ! -d third_party/rapidjson ]; then
  git clone --depth 1 https://github.com/Tencent/rapidjson.git third_party/rapidjson
fi
echo "Deps ready."

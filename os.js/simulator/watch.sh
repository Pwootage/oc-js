#!/usr/bin/env bash

set -e

trap 'trap - SIGTERM && kill 0' SIGINT SIGTERM EXIT

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NODE_BIN="${DIR}/node_modules/.bin"

cd "${DIR}/../bios"
"${NODE_BIN}/tsc" --watch --outDir "${DIR}/" &

cd "${DIR}/../os"
"${NODE_BIN}/tsc" --watch --outDir "${DIR}/root/" &

cd "${DIR}"
"${NODE_BIN}/tsc" --watch
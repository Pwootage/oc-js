#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd "${DIR}/../bios"
tsc

cd "${DIR}/../os"
tsc

cd "${DIR}"

cp "${DIR}/../../src/main/resources/assets/oc-js/bios/bios.js" "${DIR}/"
cp "${DIR}/../../src/main/resources/assets/oc-js/bios/bootloader.js" "${DIR}"
cp -r "${DIR}/../../src/main/resources/assets/oc-js/os/"/* "${DIR}/root/"
#!/usr/bin/env bash
set -eu
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
flags=(-std=c11 -pedantic -Wall -Wextra -Werror -Icommon -Irx-bridge/src -Itests/v1)
cc "${flags[@]}" tests/protocol_test.c -o "$build/vectors"
"$build/vectors"
cc "${flags[@]}" tests/console_test.c tests/v1/{transmitter,source}.c common/{console_pad,console_usb}.c \
  rx-bridge/src/{link,receiver}.c -o "$build/console"
"$build/console"

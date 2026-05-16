#!/bin/bash

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    PIO_BIN="$USERPROFILE/.platformio/penv/Scripts/pio.exe"
else
    PIO_BIN="$HOME/.platformio/penv/bin/pio"
fi

if [ ! -x "$PIO_BIN" ]; then
    echo "PlatformIO CLI not found at: $PIO_BIN" >&2
    exit 1
fi

echo "Running PlatformIO: $PIO_BIN run $@"
"$PIO_BIN" run "$@"

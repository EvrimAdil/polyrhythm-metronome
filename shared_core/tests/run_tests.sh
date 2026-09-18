#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORE_DIR="$SCRIPT_DIR/.."
BIN_DIR="$SCRIPT_DIR/bin"

mkdir -p "$BIN_DIR"

echo "=========================================================="
echo "Compiling and Running Polyrhythm C++20 Core Unit Tests"
echo "Compiler: $(clang++ --version | head -n 1)"
echo "=========================================================="

# Common sources
SOURCES="$CORE_DIR/src/bjorklund.cpp $CORE_DIR/src/sound_generator.cpp $CORE_DIR/src/trainers.cpp $CORE_DIR/src/rhythm_layer.cpp $CORE_DIR/src/audio_scheduler.cpp $CORE_DIR/src/polyrhythm_engine.cpp $CORE_DIR/src/c_bridge.cpp"
INCLUDES="-I$CORE_DIR/include"
CXXFLAGS="-std=c++20 -O3 -Wall -Wextra -pthread"

echo ""
echo "-> 1. Compiling & Running: test_scheduler_drift..."
clang++ $CXXFLAGS $INCLUDES $SOURCES "$SCRIPT_DIR/test_scheduler_drift.cpp" -o "$BIN_DIR/test_scheduler_drift"
"$BIN_DIR/test_scheduler_drift"

echo ""
echo "-> 2. Compiling & Running: test_bjorklund..."
clang++ $CXXFLAGS $INCLUDES $SOURCES "$SCRIPT_DIR/test_bjorklund.cpp" -o "$BIN_DIR/test_bjorklund"
"$BIN_DIR/test_bjorklund"

echo ""
echo "-> 3. Compiling & Running: test_polyrhythm..."
clang++ $CXXFLAGS $INCLUDES $SOURCES "$SCRIPT_DIR/test_polyrhythm.cpp" -o "$BIN_DIR/test_polyrhythm"
"$BIN_DIR/test_polyrhythm"

echo ""
echo "=========================================================="
echo "ALL TESTS PASSED! ZERO-DRIFT C++20 AUDIO CORE VALIDATED!"
echo "=========================================================="

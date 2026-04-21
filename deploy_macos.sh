#!/bin/bash

# Configuration
PLUGIN_NAME="gamepad-monitor"
OBS_PLUGINS_DIR="$HOME/Library/Application Support/obs-studio/plugins"
BUILD_DIR="build_macos/Release"

echo "🚀 Building Gamepad Monitor Plugin..."

# Initialize build directory if it doesn't exist
if [ ! -d "build_macos" ]; then
    mkdir -p build_macos
    cd build_macos
    cmake .. -G Xcode -Dlibobs_DIR=/Users/jonwood/Github_local_dev/mediaWarp/midi-server-plugin/.deps/libobs -Dobs-frontend-api_DIR=/Users/jonwood/Github_local_dev/mediaWarp/midi-server-plugin/.deps/obs-frontend-api -DQt6_DIR=/Users/jonwood/Github_local_dev/mediaWarp/midi-server-plugin/.deps/qt6/lib/cmake/Qt6
    cd ..
fi

cmake --build build_macos --config Release --target $PLUGIN_NAME

if [ $? -ne 0 ]; then
    echo "❌ Build failed!"
    exit 1
fi

echo "📦 Deploying to OBS..."
mkdir -p "$OBS_PLUGINS_DIR"
rm -rf "$OBS_PLUGINS_DIR/$PLUGIN_NAME.plugin"
cp -R "$BUILD_DIR/$PLUGIN_NAME.plugin" "$OBS_PLUGINS_DIR/"

echo "✍️  Ad-hoc signing..."
codesign --force --deep --sign - "$OBS_PLUGINS_DIR/$PLUGIN_NAME.plugin"

echo "✅ Done! Please restart OBS Studio."

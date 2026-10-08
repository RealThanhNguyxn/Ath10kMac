#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
app_dir="$PWD/build/Ath10k Connectivity.app"
mkdir -p "$app_dir/Contents/MacOS"
xcrun clang++ -std=c++17 -fobjc-arc -Wall -Wextra -Werror -Wno-deprecated-declarations \
  -arch x86_64 -mmacosx-version-min=11.0 app/Main.mm \
  -framework Cocoa -framework IOKit -framework Security -o "$app_dir/Contents/MacOS/Ath10kConnectivity"
cat > "$app_dir/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleIdentifier</key><string>br.com.jonasw8.Ath10kConnectivity</string>
<key>CFBundleName</key><string>Ath10k Connectivity</string>
<key>CFBundleExecutable</key><string>Ath10kConnectivity</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>0.1.2</string>
<key>CFBundleVersion</key><string>3</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
codesign --force --sign - "$app_dir"
printf 'App criado: %s\nAbra usando ./scripts/run-connectivity.sh\n' "$app_dir"

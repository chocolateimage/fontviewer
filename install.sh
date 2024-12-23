#!/usr/bin/env bash

set -e

echo "Installing dependencies..."
if command -v apt >/dev/null
then
    sudo apt install git g++ meson pkg-config libfontconfig-dev libgtkmm-3.0-dev libjson-glib-dev libcurl4-gnutls-dev
elif command -v dnf >/dev/null
then
    sudo dnf install git g++ meson pkg-config fontconfig-devel gtkmm3.0-devel json-glib-devel libcurl-devel
else
    echo "Could not find a package manager, try manually installing with the \"Building/installing from source\" section in the README"
    exit 1
fi

echo "Cloning repository..."
mkdir -p /tmp/fontviewerinstall
cd /tmp/fontviewerinstall

git clone https://github.com/chocolateimage/fontviewer.git
cd fontviewer

echo "Setting up build directory..."
meson setup builddir -Dbuildtype=release -Ddebug=false
cd builddir

echo "Building & installing, you may need to press \"y\" and enter to allow it to install"
meson install

cd
rm -rf /tmp/fontviewerinstall

echo "Done! You can find \"Fonts\" in your application launcher"
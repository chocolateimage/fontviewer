#!/usr/bin/env bash

set -e

echo "Installing dependencies..."
sudo apt install git g++ meson pkg-config libfontconfig-dev libgtkmm-3.0-dev libjson-glib-dev libcurl4-gnutls-dev

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
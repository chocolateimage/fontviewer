# Font Viewer
View and install fonts on a Linux system

![image](https://github.com/chocolateimage/fontviewer/assets/45315451/f3f4cfc3-c5d0-4eb1-84a3-82e0ec4d9d8d)


---


## Installation

To add the PPA (required for the package), run the following commands:
```bash
sudo add-apt-repository ppa:chocolateimage/ppa
sudo apt update
```
To install the package, run:
```bash
sudo apt install fontviewer
```


## Building from source
You need to have `meson` installed.

Install all dependencies:
```bash
sudo apt install meson pkg-config libfontconfig-dev libgtkmm-3.0-dev libjson-glib-dev libcurl4-gnutls-dev
```
You may want to remove `libcurl4-gnutls-dev` from the install list if you already have libcurl-dev installed.

Run this command to setup the build directory:
```bash
meson setup builddir
cd builddir
```
And to compile the program run:
```
meson compile
```

To run fontviewer run:
```bash
# Make sure to be in the build directory!
./fontviewer
```
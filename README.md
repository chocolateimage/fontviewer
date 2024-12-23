# Font Viewer
View and install fonts on a Linux system and install fonts from Google Fonts.

![image](https://github.com/user-attachments/assets/2f94ce10-39b7-46c3-bc9b-fecb54ee9207)


---


## Installation

To install the package, run:
```bash
bash <(curl -s https://raw.githubusercontent.com/chocolateimage/fontviewer/refs/heads/main/install.sh)
```

I no longer want to do PPA for now because of how shitty Launchpad is.


## Building/installing from source
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

If you want to **install**:
```bash
meson install
```
And press "y" and enter if asked for elevated privileges, the program should now appear in the application launcher as "Fonts".

---

If you want to **develop**:

To compile the program run:
```bash
meson compile
```

To run fontviewer run:
```bash
# Make sure to be in the build directory!
./fontviewer
```

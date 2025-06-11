#!/usr/bin/env bash

uninstalled=false

if command -v dpkg >/dev/null
then
    if dpkg -s fontviewer &>/dev/null; then
        sudo apt remove -y fontviewer
        uninstalled=true
    fi
fi

for prefix in "/usr/local" "/usr"
do
    if [ -e "$prefix/bin/fontviewer" ]
    then
        uninstalled=true
        echo "Uninstalling in $prefix"
        sudo rm -f "$prefix/bin/fontviewer"
        sudo rm -f "$prefix/share/applications/fontviewer.desktop"
        sudo rm -f "$prefix/share/icons/hicolor/scalable/actions/fontviewer-google-symbolic.svg"
        sudo rm -f "$prefix/share/icons/hicolor/scalable/actions/fontviewer-google-colorful.svg"
        sudo rm -f "$prefix/share/locale/de/LC_MESSAGES/fontviewer.mo"
    fi
done

if $uninstalled
then
    echo "Font Viewer is now uninstalled"
else
    echo "Font Viewer was already uninstalled"
fi

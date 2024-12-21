#pragma once
#include <string>
#include <gtkmm.h>

class GoogleFontsStyle;

class GoogleFontsFamily {
public:
    std::string family;
    std::string displayName;

    int sortPopularity;
    int sortTrending;

    Gtk::Button *button;

    std::vector<GoogleFontsStyle*> styles;
};

class GoogleFontsStyle {
public:
    int weight;
    int slant;
};
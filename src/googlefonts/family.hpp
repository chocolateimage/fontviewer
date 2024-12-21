#pragma once
#include <string>
#include <gtkmm.h>

class GoogleFontsStyle;

class GoogleFontsFamily {
public:
    std::string family;
    std::string displayName;
    std::string language;

    std::vector<std::string> subsets;

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
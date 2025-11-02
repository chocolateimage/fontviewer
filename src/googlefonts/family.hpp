#pragma once
#include <string>
#include <vector>
#include <gtkmm/button.h>

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

    std::vector<std::shared_ptr<GoogleFontsStyle>> styles;

    bool isInstalled;
    std::vector<std::string> paths;
};

class GoogleFontsStyle {
public:
    std::shared_ptr<GoogleFontsFamily> family;

    int weight;
    int slant;
};

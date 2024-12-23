#pragma once

#include <string>
#include <vector>
#include <sys/stat.h>
#include <gtkmm.h>
#include <glib/gi18n.h>
#include <fontconfig/fontconfig.h>
#include "sushi-font-widget.h"

struct FontStyleData;

struct FontFamilyData {
    std::string family;
    std::vector<std::string>* paths; // for the tooltip
    long fileCreationTime;
    std::vector<FontStyleData*>* styles;
    bool isModifiedGrouping;
};
struct FontStyleData {
    FontFamilyData* family;
    std::string path; // store path in FontStyleData because some fonts have their weights/slants/etc in different files
    int weight;
    int slant;
    int index;
    int sortingOrder;
};

struct FontListItem {
    Gtk::Button* button;
    Gtk::Label* preview;
    FontFamilyData* fontFamily;
    bool hasBeenViewed;
};
struct FontStyleListItem {
    Gtk::Button* button;
    SushiFontWidget* fontWidget;
    Gtk::Widget* fontWidgetMM;
};
struct FontStyleRow {
    std::string name;
    Gtk::Label* lblName;
    Gtk::Label* lblValue;
};
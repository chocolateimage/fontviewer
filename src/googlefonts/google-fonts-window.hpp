#pragma once
#include <gtkmm.h>
#include <vector>
#include "family.hpp"

struct GoogleFontsFamilyListItem;

class GoogleFontsWindow: public Gtk::Window {
    public:
        GoogleFontsWindow();

        void fontListScroll();

        Gtk::ScrolledWindow* scrolledWindow;
        Gtk::Box* familyListBox;

        std::vector<GoogleFontsFamilyListItem*>* fontListItems;
        std::vector<GoogleFontsFamily*>* families;
};

struct GoogleFontsFamilyListItem {
    Gtk::Button* button;
    Gtk::Box* buttonBox;
    Gtk::Label* placeholderText;
    GoogleFontsFamily* fontFamily;
    const char *temppath;
    bool hasBeenViewed;
};

struct GoogleFontsFamilyLoadData {
    GFile* tempFileG;
    Gtk::Label* placeholderText;
};
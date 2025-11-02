#pragma once

#include <string>
#include <vector>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/searchbar.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/box.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/stack.h>
#include "font.hpp"
// #include "googlefonts/google-fonts-window.hpp"

class MainWindow: public Gtk::ApplicationWindow {
    public:
        MainWindow(std::vector<FontFamilyData*>* fonts, std::string* defaultFileName);
        void loadFonts();
        bool queuedfontListScrollCallback();
        void fontListScroll();
        void switchToFontFamily(int);
        void switchToFontFile(std::string);
        void loadFont();
        void fontPreviewTextChanged();
        void switchToFontList();
        void addInfoText(std::string,std::string);
        void installFontClicked();
        bool windowKeyPressEvent(guint keyval, guint keycode, Gdk::ModifierType state);
        void searchUpdated();
        void openGoogleFonts();
        ~MainWindow();

        int currentFontIndex;
        std::string* currentFontPath;
        FontFamilyData* familyData;
        std::vector<FontListItem*>* fontListItems;
        Gtk::HeaderBar* headerBar;
        Gtk::SearchBar* searchBar;
        Gtk::SearchEntry* searchEntry;
        Gtk::Label* headerBarCustomText;
        Gtk::Button* backButton;
        Gtk::Button* installButton;
        Gtk::ToggleButton* searchButton;
        Gtk::Button* googleFontsButton;
        Gtk::Stack* stack;
        Gtk::ScrolledWindow* fontsListScrollWidget;
        Gtk::Box* fontsListWidget;
        Gtk::Box* fontViewWidget;
        Gtk::ScrolledWindow* fontFamilyScrollWidget;
        Gtk::Box* fontFamilyBoxWidget;
        Gtk::Label* fontFamilyLabelWidget;
        Gtk::ScrolledWindow* fontStyleScrollWidget;
        Gtk::Box* fontStyleRowsWidget;
        Gtk::Entry* fontFamilyEntryWidget;
        std::string* currentPreviewText;
        std::vector<FontStyleListItem*>* fontStyleListItems;
        std::vector<FontStyleRow*>* fontStyleRows;

        std::vector<FontFamilyData*>* fontFamilies = NULL;

        // GoogleFontsWindow* googleFontsWindow;
};

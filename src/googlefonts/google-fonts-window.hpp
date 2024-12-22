#pragma once
#include <gtkmm.h>
#include <vector>
#include "family.hpp"

struct GoogleFontsFamilyListItem;
struct GoogleFontsFamilyLoadData;

class GoogleFontsWindow: public Gtk::Window {
    public:
        GoogleFontsWindow();

        bool queuedFontListScroll();
        void fontListScroll();
        void searchUpdated();
        void switchToFontList();
        void switchToFontFamily(GoogleFontsFamilyListItem* fontListItem);

        Gtk::HeaderBar *headerBar;
        Gtk::Button *backButton;
        Gtk::SearchEntry *searchEntry;
        Gtk::Stack *stack;

        Gtk::Spinner *spinner;

        Gtk::ScrolledWindow* scrolledWindow;
        Gtk::Box* familyListBox;

        Gtk::Notebook *notebook;
        Gtk::Box *boxSpecimen;
        Gtk::Label *specimenTitle;
        Gtk::Box *specimenStyles;

        std::vector<GoogleFontsFamilyListItem*>* fontListItems;
        std::vector<GoogleFontsFamily*>* families;
};

void GoogleFontsWindow_loadFamilies(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void GoogleFontsWindow_loadFamilies_callback(GObject *source_object, GAsyncResult *res, gpointer user_data);

struct GoogleFontsFamilyListItem {
    Gtk::Button* button;
    Gtk::Box* buttonBox;
    Gtk::Label* placeholderText;
    GoogleFontsFamily* fontFamily;
    bool hasBeenViewed;

    GoogleFontsFamilyLoadData* loadData;
};

struct GoogleFontsFamilyLoadData {
    std::string family;
    const char *temppath;
};

struct GoogleFontsFontWidgetLoadData {
    GFile* tempFileG;
    Gtk::Label* placeholderText;
};
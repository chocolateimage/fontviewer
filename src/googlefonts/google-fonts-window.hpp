#pragma once
#include <gtkmm.h>
#include <vector>
#include "family.hpp"

struct GoogleFontsFamilyListItem;

class GoogleFontsWindow: public Gtk::Window {
    public:
        GoogleFontsWindow();

        bool queuedFontListScroll();
        void fontListScroll();
        void searchUpdated();

        Gtk::HeaderBar *headerBar;
        Gtk::SearchEntry *searchEntry;
        Gtk::Stack *stack;
        Gtk::Spinner *spinner;
        Gtk::ScrolledWindow* scrolledWindow;
        Gtk::Box* familyListBox;

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
    const char *temppath;
    bool hasBeenViewed;
};

struct GoogleFontsFamilyLoadData {
    GFile* tempFileG;
    Gtk::Label* placeholderText;
};
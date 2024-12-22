#pragma once
#include <gtkmm.h>
#include <vector>
#include "family.hpp"
#include "../sushi-font-widget.h"

struct GoogleFontsFamilyListItem;
struct GoogleFontsStyleListItem;
struct GoogleFontsFamilyLoadData;

class GoogleFontsWindow: public Gtk::Window {
    public:
        GoogleFontsWindow();

        bool queuedFontListScroll();
        void fontListScroll();
        void searchUpdated();
        void switchToFontList();
        void switchToFontFamily(GoogleFontsFamilyListItem* fontListItem);
        void updateStylePreview();
        void userOverridenStylePreviewTextChanged();

        std::string *getStylePreviewText();

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
        Gtk::Entry *specimenStylesCustomPreviewEntry;
        Gtk::Box *specimenStyles;

        std::string *stylePreviewText;
        std::string *userOverridenStylePreviewText;

        std::vector<GoogleFontsFamilyListItem*>* fontListItems;
        std::vector<GoogleFontsStyleListItem*>* styleListItems;
        std::vector<GoogleFontsFamily*>* families;
    private:
        std::string *_newSampleText;
};

void GoogleFontsWindow_loadFamilies(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void GoogleFontsWindow_loadFamilies_callback(GObject *source_object, GAsyncResult *res, gpointer user_data);
void GoogleFontsWindow_loadFontFamilyInList(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void GoogleFontsWindow_loadFontFamilyInList_callback_style(GObject *source_object, GAsyncResult *res, gpointer user_data);

struct GoogleFontsFamilyListItem {
    Gtk::Button* button;
    Gtk::Box* buttonBox;
    Gtk::Label* placeholderText;
    GoogleFontsFamily* fontFamily;
    bool hasBeenViewed;

    GoogleFontsFamilyLoadData* loadData;
};

struct GoogleFontsStyleListItem {
    GoogleFontsWindow* googleFontsWindow;
    Gtk::Box* box;
    Gtk::Label* placeholderText;
    GoogleFontsStyle* style;
    SushiFontWidget* fontWidget;

    GCancellable* loadCancellable;
    GoogleFontsFamilyLoadData* loadData;
};

struct GoogleFontsFamilyLoadData {
    std::string family;
    const char *temppath;
};

struct GoogleFontsFontWidgetLoadData {
    GFile* tempFileG;
    Gtk::Label* placeholderText;
    SushiFontWidget* fontWidget;
};
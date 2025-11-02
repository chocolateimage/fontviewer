#pragma once
#include <vector>
#include <gtkmm/widget.h>
#include <gtkmm/window.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/searchentry.h>
#include <gtkmm/spinner.h>
#include <gtkmm/stack.h>
#include <gtkmm/scrolledwindow.h>
#include "family.hpp"
#include "../font.hpp"
#include "../sushi-font-widget.h"

class MainWindow;
struct GoogleFontsFamilyListItem;
struct GoogleFontsStyleListItem;
struct GoogleFontsFamilyLoadData;

class GoogleFontsWindow: public Gtk::Window {
    public:
        GoogleFontsWindow(std::vector<FontFamilyData*>* fonts);
        ~GoogleFontsWindow();

        bool queuedFontListScroll();
        void fontListScroll();
        void searchUpdated();
        void switchToFontList();
        void switchToFontFamily(std::shared_ptr<GoogleFontsFamilyListItem> fontListItem);
        void updateStylePreview();
        void userOverridenStylePreviewTextChanged();
        void installButtonReload();
        void installButtonClick();
        bool windowKeyPressEvent(GdkEventKey* event);

        std::string getStylePreviewText();

        std::vector<FontFamilyData*>* fontFamilies = NULL;

        Gtk::HeaderBar headerBar;
        Gtk::Label headerBarCustomText;
        Gtk::Button backButton;
        Gtk::SearchEntry searchEntry;
        Gtk::Stack stack;

        Gtk::Spinner spinner;

        Gtk::ScrolledWindow scrolledWindow;
        Gtk::Box familyListBox;


        Gtk::Notebook notebook;

        Gtk::ScrolledWindow swSpecimen;
        Gtk::Box boxSpecimen;
        Gtk::Box specimenHeader;
        Gtk::Label specimenTitle;
        Gtk::Button specimenInstallButton;
        Gtk::Label specimenAuthors;
        Gtk::Entry specimenStylesCustomPreviewEntry;
        Gtk::Box specimenStyles;
        Gtk::Label specimenStylesLabel;

        Gtk::Box boxAbout;

        Gtk::ScrolledWindow swLicense;
        Gtk::Box boxLicense;
        Gtk::Label licenseLabel;
        Gtk::Label licenseTitleLabel;

        std::string stylePreviewText;
        std::string userOverridenStylePreviewText;

        std::vector<std::shared_ptr<GoogleFontsFamilyListItem>> fontListItems;
        std::vector<std::shared_ptr<GoogleFontsStyleListItem>> styleListItems;
        std::vector<std::shared_ptr<GoogleFontsFamily>> families;

        std::shared_ptr<GoogleFontsFamilyListItem> currentFontListItem;
    private:
        void loadLicense();
        void loadFamilyDetails();
        void loadSampleText();
};

void GoogleFontsWindow_loadFamilies(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void GoogleFontsWindow_loadFamilies_callback(GObject *source_object, GAsyncResult *res, gpointer user_data);
void GoogleFontsWindow_loadFontFamilyInList(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void GoogleFontsWindow_loadFontFamilyInList_callback_style(GObject *source_object, GAsyncResult *res, gpointer user_data);

struct GoogleFontsFamilyListItem {
    Gtk::Button* button;
    Gtk::Box* buttonBox;
    Gtk::Label* placeholderText;
    Gtk::Widget* installedIconWidget;
    Gtk::Widget* installedLabelWidget;
    std::shared_ptr<GoogleFontsFamily> fontFamily;
    bool hasBeenViewed;

    std::shared_ptr<GoogleFontsFamilyLoadData> loadData;
};

struct GoogleFontsStyleListItem {
    GoogleFontsWindow* googleFontsWindow;
    Gtk::Box* box;
    Gtk::Label* placeholderText;
    std::shared_ptr<GoogleFontsStyle> style;
    SushiFontWidget* fontWidget;

    GCancellable* loadCancellable;
    std::shared_ptr<GoogleFontsFamilyLoadData> loadData;
};

struct GoogleFontsFamilyLoadData {
    std::string family;
    std::string language;
};

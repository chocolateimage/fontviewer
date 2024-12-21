#include "google-fonts-window.hpp"
#include "src/googlefonts/family.hpp"
#include <json-glib/json-glib.h>
#include <gtkmm.h>
#include <giomm.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <regex>
#include <glib/gi18n.h>
#include <unistd.h>
#include "../pangram.hpp"
#include "../sushi-font-widget.h"

std::string loadStringFromURI(std::string uri) {
    auto file = Gio::File::create_for_uri(uri);

    auto stream = file->read();

    std::string response;

    char buffer[4096];
    gsize bytes_read;
    while (true) {
        bytes_read = stream->read(buffer, sizeof(buffer));
        if (bytes_read == 0) {
            break;
        }

        response.append(buffer, bytes_read);
    }

    return response;
}

GoogleFontsWindow::GoogleFontsWindow() {
    auto provider = Gtk::CssProvider::create();
    provider->load_from_data(
        ".window {background: #202124; color: #ffffff;}"
        );
    this->get_style_context()->add_provider_for_screen(Gdk::Screen::get_default(),provider,GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    this->get_style_context()->add_class("window");

    this->families = new std::vector<GoogleFontsFamily*>();
    this->fontListItems = new std::vector<GoogleFontsFamilyListItem*>();

    this->resize(1000, 700);
    this->set_title("Google Fonts");

    headerBar = new Gtk::HeaderBar();
    headerBar->set_title("Google Fonts");
    headerBar->set_show_close_button();
    this->set_titlebar(*headerBar);

    searchEntry = new Gtk::SearchEntry();
    searchEntry->set_placeholder_text(_("Search..."));
    searchEntry->signal_changed().connect(sigc::mem_fun(*this,&GoogleFontsWindow::searchUpdated));
    searchEntry->set_sensitive(false);
    headerBar->pack_end(*searchEntry);

    stack = new Gtk::Stack();
    stack->set_transition_duration(200);
    stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_CROSSFADE);
    this->add(*stack);

    spinner = new Gtk::Spinner();
    spinner->start();
    stack->add(*spinner, "loading");

    scrolledWindow = new Gtk::ScrolledWindow();
    scrolledWindow->get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this,&GoogleFontsWindow::fontListScroll));
    familyListBox = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    scrolledWindow->add(*familyListBox);
    stack->add(*scrolledWindow, "list");

    GCancellable* cancellable = g_cancellable_new();
    GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFamilies_callback,this);
    g_task_set_task_data(task,this,NULL);
    g_task_run_in_thread(task,GoogleFontsWindow_loadFamilies);
}

void GoogleFontsWindow_loadFamilies(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    GoogleFontsWindow *self = (GoogleFontsWindow*)task_data;
    std::string response = loadStringFromURI("https://fonts.google.com/metadata/fonts");

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    json_parser_load_from_data(parser, response.c_str(), response.length(), &error);

    if (error != NULL) {
        std::cout << "Unable to parse: " << error->message << "\n";
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    JsonObject* rootObject = json_node_get_object(root);
    JsonArray* familyMetadataList = json_object_get_array_member(rootObject, "familyMetadataList");

    int length = json_array_get_length(familyMetadataList);
    for (int i = 0; i < length; i++) {
        GoogleFontsFamily *family = new GoogleFontsFamily();
        JsonObject *fontMetadata = json_array_get_object_element(familyMetadataList, i);
        family->family = json_object_get_string_member(fontMetadata, "family");
        if (json_object_get_null_member(fontMetadata, "displayName")) {
            family->displayName = family->family;
        } else {
            family->displayName = json_object_get_string_member(fontMetadata, "displayName");
        }
        
        family->sortPopularity = json_object_get_int_member(fontMetadata, "popularity");
        family->sortTrending = json_object_get_int_member(fontMetadata, "defaultSort");

        family->language = json_object_get_string_member(fontMetadata, "primaryScript");
        if (family->language == "") {
            family->language = json_object_get_string_member(fontMetadata, "primaryLanguage");
        }
        if (family->language == "") {
            family->language = "en_Latn";
        }

        JsonArray *subsetsArray = json_object_get_array_member(fontMetadata, "subsets");
        int subsetsLength = json_array_get_length(subsetsArray);
        for (int j = 0; j < subsetsLength; j++) {
            std::string subset = json_array_get_string_element(subsetsArray, j);
            family->subsets.push_back(subset);
        }

        if (
            family->language == "en_Latn" &&
            std::find(family->subsets.begin(), family->subsets.end(), "emoji") != family->subsets.end() &&
            std::find(family->subsets.begin(), family->subsets.end(), "latin") == family->subsets.end()
        ) {
            family->language = "emoji";
        }

        JsonObject *styleListObject = json_object_get_object_member(fontMetadata, "fonts");
        GList *stylesList = json_object_get_members(styleListObject);
        for (GList *j = stylesList; j != NULL; j = j->next) {
            std::string styleName = std::string((const gchar*)j->data);

            GoogleFontsStyle *style = new GoogleFontsStyle();
            if (styleName.at(styleName.length() - 1) == 'i') {
                style->slant = 1;
                styleName.pop_back();
            }
            style->weight = std::stoi(styleName);
            family->styles.push_back(style);
        }
        g_list_free(stylesList);

        self->families->push_back(family);
    }

    g_object_unref(parser);

    g_task_return_boolean(task, true);
}

void GoogleFontsWindow_loadFamilies_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GoogleFontsWindow* self = (GoogleFontsWindow*)user_data;

    self->stack->set_visible_child("list");

    std::sort(self->families->begin(), self->families->end(), [](GoogleFontsFamily *a, GoogleFontsFamily *b) {
        return a->sortTrending < b->sortTrending;
    });

    for (auto i : *self->families) {

        GoogleFontsFamilyListItem* fontListItem = new GoogleFontsFamilyListItem();
        fontListItem->fontFamily = i;
        fontListItem->hasBeenViewed = false;

        Gtk::Button *btn = new Gtk::Button();
        fontListItem->button = btn;

        btn->set_relief(Gtk::RELIEF_NONE);

        Gtk::VBox* btnBox = new Gtk::VBox();
        fontListItem->buttonBox = btnBox;
        btnBox->set_margin_left(8);
        btnBox->set_margin_right(8);
        btnBox->set_margin_top(4);
        btnBox->set_margin_bottom(4);
        btnBox->set_spacing(8);

        Gtk::HBox* btnHeaderBox = new Gtk::HBox();
        btnHeaderBox->set_spacing(4);

        Gtk::Label* btnLabel = new Gtk::Label();
        btnLabel->set_alignment(Gtk::ALIGN_START);
        btnLabel->set_text(i->displayName);
        btnLabel->get_style_context()->add_class("display-name");
        btnHeaderBox->pack_start(*btnLabel,Gtk::PACK_SHRINK,0);
        
        if (i->styles.size() > 1) {
            Gtk::Label* btnStyleCount = new Gtk::Label();
            btnStyleCount->set_sensitive(false);
            btnStyleCount->set_alignment(Gtk::ALIGN_START);
            btnStyleCount->set_text(Glib::ustring::compose(_("%1 styles"),std::to_string(i->styles.size())));
            btnHeaderBox->pack_start(*btnStyleCount,Gtk::PACK_SHRINK,0);
        }

        btnBox->add(*btnHeaderBox);

        Gtk::Label* lblPlaceholder = new Gtk::Label();
        auto context = lblPlaceholder->get_pango_context();
        auto fontDescription = context->get_font_description();
        fontDescription.set_size(26 * PANGO_SCALE);
        context->set_font_description(fontDescription);
        lblPlaceholder->set_text("");
        lblPlaceholder->set_alignment(Gtk::ALIGN_START);
        btnBox->add(*lblPlaceholder);
        fontListItem->placeholderText = lblPlaceholder;

        btn->add(*btnBox);

        btn->show_all();

        self->familyListBox->add(*btn);

        i->button = btn;
        self->fontListItems->push_back(fontListItem);
    }
    self->signal_check_resize().connect(sigc::mem_fun(*self,&GoogleFontsWindow::fontListScroll));
    
    self->searchEntry->set_sensitive(true);
    self->searchEntry->grab_focus();
}

void GoogleFontsWindow_fontFamilyLoaded(SushiFontWidget* fontWidget, GoogleFontsFamilyLoadData* data) {
    g_file_delete(data->tempFileG, NULL, NULL);
    delete data->placeholderText;
}

void GoogleFontsWindow_fontFamilyError(SushiFontWidget* fontWidget, GError *error, GoogleFontsFamilyLoadData* data) {
    g_file_delete(data->tempFileG, NULL, NULL);
    data->placeholderText->set_text("Error loading font");
}

void GoogleFontsWindow_loadFontFamilyInList(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    GoogleFontsFamilyListItem *listItem = (GoogleFontsFamilyListItem*)task_data;
    std::regex sourceRegex(R"(src:\s*url\(([^)]+)\))");
    std::string css;
    std::smatch match;

    try {
        css = loadStringFromURI("https://fonts.googleapis.com/css2?family="+listItem->fontFamily->family+"%3Awght%40400&directory=3&display=block");
    } catch (Gio::Error &error) {
        std::cout << "An error has occured while loading the font " << listItem->fontFamily->family << ": " << error.what() << std::endl;
        g_task_return_boolean(task,false);
        return;
    }
    if (!std::regex_search(css, match, sourceRegex)) {
        std::cout << "Could not find font data in " << listItem->fontFamily->family << std::endl;
        g_task_return_boolean(task,false);
        return;
    }

    auto last = new std::string(match[match.size() - 1].str());

    std::string tempnameString = "/tmp/fontviewer_font_" + listItem->fontFamily->family + "_XXXXXX";

    char *tempname = new char[tempnameString.size() + 1];
    memcpy(tempname, tempnameString.c_str(), tempnameString.size() + 1);

    listItem->temppath = tempname;
    int tempFileDescriptor = mkstemp(tempname);

    FILE *tempFile = fdopen(tempFileDescriptor, "wb");
    auto onlineFile = Gio::File::create_for_uri(*last);

    auto stream = onlineFile->read();

    gsize total_size = 0;

    char buffer[4096];
    gsize bytes_read;
    while (true) {
        bytes_read = stream->read(buffer, sizeof(buffer));
        if (bytes_read == 0) {
            break;
        }

        fwrite(buffer, bytes_read, 1, tempFile);
        total_size += bytes_read;
    }
    if (total_size < 1000) {
        std::cout << "BROKEN: " << listItem->fontFamily->family << std::endl;
    }
    fflush(tempFile);
    fclose(tempFile);

    g_task_return_boolean(task,true);
}


void GoogleFontsWindow_loadFontFamilyInList_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GoogleFontsFamilyListItem* listItem = (GoogleFontsFamilyListItem*)user_data;
    GError* error = NULL;
    bool returnBool = g_task_propagate_boolean(G_TASK(res),&error);
    if (returnBool == false || error != NULL) {
        listItem->placeholderText->set_text(_("Error loading"));
        return;
    }

    GFile* tempFileG = g_file_new_for_path(listItem->temppath);
    char* tempFileURIG = g_file_get_uri(tempFileG);
    SushiFontWidget* fontWidget = sushi_font_widget_new(tempFileURIG, 0);

    sushi_font_widget_set_text(fontWidget, (gchar*)getPreviewTextForLanguage(listItem->fontFamily->language));
    
    GoogleFontsFamilyLoadData *loadData = new GoogleFontsFamilyLoadData();
    loadData->tempFileG = tempFileG;
    loadData->placeholderText = listItem->placeholderText;
    g_signal_connect(fontWidget,"loaded", G_CALLBACK(GoogleFontsWindow_fontFamilyLoaded), loadData);
    g_signal_connect(fontWidget,"error", G_CALLBACK(GoogleFontsWindow_fontFamilyError), loadData);
    
    gtk_widget_set_has_window(GTK_WIDGET(fontWidget), false); // Needed else the preview will take over focus/hover
    Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
    fontWidgetMM->show_all();
    listItem->buttonBox->add(*fontWidgetMM);
}


void GoogleFontsWindow::fontListScroll() {
    double scrollPosition = scrolledWindow->get_vadjustment()->get_value();
    for (GoogleFontsFamilyListItem* listItem : *fontListItems) {
        if (listItem->hasBeenViewed) continue;
        if (!listItem->button->get_visible()) continue;

        Gtk::Allocation allocation = listItem->button->get_allocation();
        int y = allocation.get_y();
        int windowHeight = this->scrolledWindow->get_height();
        if (y > scrollPosition - 256 && y < scrollPosition + windowHeight + 256) {
            listItem->hasBeenViewed = true;

            GCancellable* cancellable = g_cancellable_new();
            GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFontFamilyInList_callback,listItem);
            g_task_set_task_data(task,listItem,NULL);
            g_task_run_in_thread(task,GoogleFontsWindow_loadFontFamilyInList);
        }
    }
}

bool GoogleFontsWindow::queuedFontListScroll() {
    this->fontListScroll();
    return false;
}

void GoogleFontsWindow::searchUpdated() {
    std::string input = this->searchEntry->get_text().lowercase();
    for (GoogleFontsFamilyListItem* listItem : *fontListItems) {
        std::string family = listItem->fontFamily->displayName;
        std::transform(family.begin(), family.end(), family.begin(), [](unsigned char c){ return std::tolower(c); });
        if (family.find(input) != std::string::npos) {
            listItem->button->show();
        } else {
            listItem->button->hide();
        }
    }
    Glib::signal_idle().connect(sigc::mem_fun(*this, &GoogleFontsWindow::queuedFontListScroll));
}
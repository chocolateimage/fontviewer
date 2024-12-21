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
#include "../constants.hpp"
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

    this->resize(900, 500);
    this->set_title("Google Fonts");

    scrolledWindow = new Gtk::ScrolledWindow();
    scrolledWindow->get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this,&GoogleFontsWindow::fontListScroll));
    familyListBox = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    scrolledWindow->add(*familyListBox);
    this->add(*scrolledWindow);

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
        family->sortTrending = json_object_get_int_member(fontMetadata, "trending");

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

        this->families->push_back(family);
    }

    std::sort(this->families->begin(), this->families->end(), [](GoogleFontsFamily *a, GoogleFontsFamily *b) {
        return a->sortPopularity < b->sortPopularity;
    });

    int index = 0;

    for (auto i : *this->families) {

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
        lblPlaceholder->set_text("Loading...");
        lblPlaceholder->set_alignment(Gtk::ALIGN_START);
        btnBox->add(*lblPlaceholder);
        fontListItem->placeholderText = lblPlaceholder;

        btn->add(*btnBox);

        this->familyListBox->add(*btn);

        i->button = btn;
        fontListItems->push_back(fontListItem);

        index += 1;
        //if (index == 105) break;
    }
    Glib::signal_idle().connect(sigc::mem_fun(*this, &GoogleFontsWindow::queuedFontListScrollCallback));
}

bool GoogleFontsWindow::queuedFontListScrollCallback() {
    fontListScroll();
    return false;
}

void GoogleFontsWindow_fontFamilyLoaded(SushiFontWidget* fontWidget, GoogleFontsFamilyLoadData* data) {
    g_file_delete(data->tempFileG, NULL, NULL);
    delete data->placeholderText;
}

void GoogleFontsWindow_loadFontFamilyInList(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    GoogleFontsFamilyListItem *listItem = (GoogleFontsFamilyListItem*)task_data;
    std::cout << listItem->fontFamily->displayName << std::endl;
    std::regex sourceRegex(R"(src:\s*url\(([^)]+)\))");
    std::string css = loadStringFromURI("https://fonts.googleapis.com/css2?family="+listItem->fontFamily->family+"%3Awght%40400&directory=3&display=block");
    std::smatch match;
    if (std::regex_search(css, match, sourceRegex)) {
        auto last = new std::string(match[match.size() - 1].str());

        char tempname[] = "/tmp/fontviewer_font_XXXXXX";
        listItem->temppath = tempname;
        int tempFileDescriptor = mkstemp(tempname);

        FILE *tempFile = fdopen(tempFileDescriptor, "wb");
        auto onlineFile = Gio::File::create_for_uri(*last);

        auto stream = onlineFile->read();

        std::string response;

        char buffer[4096];
        gsize bytes_read;
        while (true) {
            bytes_read = stream->read(buffer, sizeof(buffer));
            if (bytes_read == 0) {
                break;
            }

            fwrite(buffer, bytes_read, 1, tempFile);
        }
        fclose(tempFile);

        g_task_return_boolean(task,true);
    } else {
        g_task_return_boolean(task,false);
    }
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

    sushi_font_widget_set_text(fontWidget, (gchar*)PANGRAM);
    
    GoogleFontsFamilyLoadData *loadData = new GoogleFontsFamilyLoadData();
    loadData->tempFileG = tempFileG;
    loadData->placeholderText = listItem->placeholderText;
    g_signal_connect(fontWidget,"loaded", G_CALLBACK(GoogleFontsWindow_fontFamilyLoaded), loadData);
    
    Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
    fontWidgetMM->show_all();
    listItem->buttonBox->add(*fontWidgetMM);
}


void GoogleFontsWindow::fontListScroll() {
    double scrollPosition = scrolledWindow->get_vadjustment()->get_value();
    for (GoogleFontsFamilyListItem* listItem : *fontListItems) {
        if (listItem->hasBeenViewed) continue;

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
#include "google-fonts-window.hpp"
#include "src/googlefonts/family.hpp"
#include <json-glib/json-glib.h>
#include <gtkmm.h>
#include <giomm.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <regex>
#include <thread>
#include <glib/gi18n.h>
#include <unistd.h>
#include <curl/curl.h>
#include "../pangram.hpp"
#include "../sushi-font-widget.h"
#include "../utils.hpp"

std::string loadStringFromURI(std::string uri) {
    std::string response;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallbackString);
    curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    return response;
}

std::string sendPOSTRequest(std::string uri, std::string json) {
    std::string response;

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json+protobuf");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallbackString);
    curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response;
}

GoogleFontsWindow::GoogleFontsWindow(std::vector<FontFamilyData*>* fonts) {
    this->fontFamilies = fonts;

    auto provider = Gtk::CssProvider::create();
    provider->load_from_data(
        "notebook tabs {padding-left: 60px; padding-right: 60px;} "
        ".disabled-icon { color: @insensitive_fg_color; }"
        );
    this->get_style_context()->add_provider_for_screen(Gdk::Screen::get_default(),provider,GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    this->families = new std::vector<GoogleFontsFamily*>();
    this->fontListItems = new std::vector<GoogleFontsFamilyListItem*>();
    this->styleListItems = NULL;
    this->stylePreviewText = new std::string("");
    this->userOverridenStylePreviewText = NULL;
    this->currentFontListItem = NULL;

    this->resize(1000, 700);
    this->set_title("Google Fonts");

    headerBar = new Gtk::HeaderBar();
    headerBar->set_title("Google Fonts");
    headerBar->set_show_close_button();
    headerBarCustomText = new Gtk::Label();
    headerBarCustomText->show();
    backButton = new Gtk::Button();
    backButton->set_image_from_icon_name("go-previous-symbolic");
    backButton->signal_clicked().connect(sigc::mem_fun(*this,&GoogleFontsWindow::switchToFontList));
    headerBar->add(*backButton);
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

    notebook = new Gtk::Notebook();

    swSpecimen = new Gtk::ScrolledWindow();
    boxSpecimen = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    boxSpecimen->set_margin_left(60);
    boxSpecimen->set_margin_right(60);
    boxSpecimen->set_margin_top(18);
    boxSpecimen->set_spacing(8);
    
    specimenHeader = new Gtk::Box();

    specimenTitle = new Gtk::Label();
    specimenTitle->set_alignment(Gtk::ALIGN_START);
    setFontSizeOfLabel(specimenTitle, 32);
    specimenHeader->add(*specimenTitle);

    specimenInstallButton = new Gtk::Button();
    specimenInstallButton->set_valign(Gtk::ALIGN_START);
    specimenInstallButton->signal_clicked().connect(sigc::mem_fun(*this, &GoogleFontsWindow::installButtonClick));
    specimenHeader->pack_end(*specimenInstallButton, Gtk::PACK_SHRINK);

    boxSpecimen->add(*specimenHeader);

    specimenAuthors = new Gtk::Label();
    specimenAuthors->set_alignment(Gtk::ALIGN_START);
    specimenAuthors->set_sensitive(false);
    specimenAuthors->set_margin_bottom(12);
    boxSpecimen->add(*specimenAuthors);

    Gtk::Label *specimenStylesLabel = new Gtk::Label();
    specimenStylesLabel->set_alignment(Gtk::ALIGN_START);
    setFontSizeOfLabel(specimenStylesLabel, 18);
    specimenStylesLabel->set_text(_("Styles"));
    boxSpecimen->add(*specimenStylesLabel);

    specimenStylesCustomPreviewEntry = new Gtk::Entry();
    specimenStylesCustomPreviewEntry->set_placeholder_text(_("Enter text to preview in the font"));
    specimenStylesCustomPreviewEntry->signal_changed().connect(sigc::mem_fun(*this, &GoogleFontsWindow::userOverridenStylePreviewTextChanged));
    boxSpecimen->add(*specimenStylesCustomPreviewEntry);

    specimenStyles = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    boxSpecimen->add(*specimenStyles);

    swSpecimen->add(*boxSpecimen);
    notebook->append_page(*swSpecimen, _("Specimen"));

    swLicense = new Gtk::ScrolledWindow();
    boxLicense = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    boxLicense->set_margin_left(60);
    boxLicense->set_margin_right(60);
    boxLicense->set_margin_top(18);
    boxLicense->set_spacing(12);

    Gtk::Label *licenseTitleLabel = new Gtk::Label();
    licenseTitleLabel->set_alignment(Gtk::ALIGN_START);
    setFontSizeOfLabel(licenseTitleLabel, 18);
    licenseTitleLabel->set_text(_("License"));
    boxLicense->add(*licenseTitleLabel);

    licenseLabel = new Gtk::Label();
    licenseLabel->set_alignment(Gtk::ALIGN_START);
    licenseLabel->set_selectable(true);
    boxLicense->add(*licenseLabel);

    swLicense->add(*boxLicense);
    notebook->append_page(*swLicense, _("License"));

    stack->add(*notebook, "view");

    GCancellable* cancellable = g_cancellable_new();
    GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFamilies_callback,this);
    g_task_set_task_data(task,this,NULL);
    g_task_run_in_thread(task,GoogleFontsWindow_loadFamilies);

    this->show_all();
    backButton->hide();
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
        } else if (family->family == "Noto Sans Symbols") {
            family->language = "symbols";
        } else if (family->family == "Noto Sans Symbols 2") {
            family->language = "symbols2";
        } else if (family->family == "Noto Music") {
            family->language = "music";
        }

        JsonObject *styleListObject = json_object_get_object_member(fontMetadata, "fonts");
        GList *stylesList = json_object_get_members(styleListObject);
        for (GList *j = stylesList; j != NULL; j = j->next) {
            std::string styleName = std::string((const gchar*)j->data);

            GoogleFontsStyle *style = new GoogleFontsStyle();
            style->family = family;
            if (styleName.at(styleName.length() - 1) == 'i') {
                style->slant = 2;
                styleName.pop_back();
            } else if (styleName.at(styleName.length() - 1) == 'o') {
                style->slant = 1;
                styleName.pop_back();
            }
            style->weight = std::stoi(styleName);
            family->styles.push_back(style);
        }
        g_list_free(stylesList);

        family->isInstalled = false;
        for (FontFamilyData* fontFamily : *self->fontFamilies) {
            if (fontFamily->family == family->family) {
                family->isInstalled = true;
                break;
            }
        }

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
        btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*self, &GoogleFontsWindow::switchToFontFamily), fontListItem));

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
        
        Gtk::Label* installedLabel = new Gtk::Label();
        installedLabel->set_valign(Gtk::ALIGN_CENTER);
        installedLabel->set_text(_("Installed"));
        installedLabel->set_sensitive(false);
        btnHeaderBox->pack_end(*installedLabel,Gtk::PACK_SHRINK,0);
        fontListItem->installedLabelWidget = installedLabel;

        Gtk::Image *installedIcon = new Gtk::Image();
        installedIcon->set_valign(Gtk::ALIGN_CENTER);
        installedIcon->set_from_icon_name("emblem-ok", Gtk::ICON_SIZE_BUTTON);
        installedIcon->get_style_context()->add_class("disabled-icon");
        btnHeaderBox->pack_end(*installedIcon,Gtk::PACK_SHRINK,0);
        fontListItem->installedIconWidget = installedIcon;

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

        if (!i->isInstalled) {
            fontListItem->installedLabelWidget->hide();
            fontListItem->installedIconWidget->hide();
        }

        self->familyListBox->add(*btn);

        i->button = btn;
        self->fontListItems->push_back(fontListItem);
    }
    self->signal_check_resize().connect(sigc::mem_fun(*self,&GoogleFontsWindow::fontListScroll));
    
    self->searchEntry->set_sensitive(true);
    self->searchEntry->grab_focus();
}

void GoogleFontsWindow::switchToFontList() {
    this->backButton->hide();
    this->stack->set_visible_child("list");
    this->currentFontListItem = NULL;

    gtk_header_bar_set_custom_title(headerBar->gobj(), NULL);
}

void GoogleFontsWindow::switchToFontFamily(GoogleFontsFamilyListItem* fontListItem) {
    this->currentFontListItem = fontListItem;
    this->backButton->show();
    this->stack->set_visible_child("view");
    this->notebook->set_current_page(0);
    this->swSpecimen->get_vadjustment()->set_value(0);

    headerBarCustomText->set_text(fontListItem->fontFamily->displayName);
    headerBar->set_custom_title(*headerBarCustomText);

    specimenTitle->set_text(fontListItem->fontFamily->displayName);

    this->installButtonReload();

    if (this->styleListItems != NULL) {
        for (auto styleListItem : *this->styleListItems) {
            g_cancellable_cancel(styleListItem->loadCancellable);
        }
        this->styleListItems->clear();
        delete this->styleListItems;
    }
    this->styleListItems = new std::vector<GoogleFontsStyleListItem*>();

    for (auto child : this->specimenStyles->get_children()) {
        delete child;
    }

    Glib::Dispatcher *dispatcher = new Glib::Dispatcher();
    this->stylePreviewText = new std::string("");
    this->licenseLabel->set_text("");
    this->specimenAuthors->set_text("");
    dispatcher->connect([this, dispatcher]() {
        if (this->stylePreviewText != NULL) {
            delete this->stylePreviewText;
        }
        this->stylePreviewText = this->_newSampleText;
        this->updateStylePreview();
        this->licenseLabel->set_markup(*this->_newLicense);
        this->specimenAuthors->set_text(Glib::ustring::compose(_("Designed by %1"), *this->_newAuthors));
        this->_newSampleText = NULL;
        delete this->_newLicense;
        this->_newLicense = NULL;
        delete this->_newAuthors;
        this->_newAuthors = NULL;
        delete dispatcher;
    });
    std::thread([this, fontListItem, dispatcher]() {
        gchar *twoLayerJSON;
        gchar *threeLayerJSON;

        {
            JsonArray *array = json_array_new();
            JsonArray *array2 = json_array_new();
            json_array_add_string_element(array2, fontListItem->fontFamily->family.c_str());
            json_array_add_array_element(array, array2);
            JsonNode *root = json_node_new(JSON_NODE_ARRAY);
            json_node_set_array(root, array);
            twoLayerJSON = json_to_string(root, false);
        }
        {
            JsonArray *array = json_array_new();
            JsonArray *array2 = json_array_new();
            JsonArray *array3 = json_array_new();
            json_array_add_string_element(array3, fontListItem->fontFamily->family.c_str());
            json_array_add_array_element(array2, array3);
            json_array_add_array_element(array, array2);
            JsonNode *root = json_node_new(JSON_NODE_ARRAY);
            json_node_set_array(root, array);
            threeLayerJSON = json_to_string(root, false);
        }

        {
            std::string response = sendPOSTRequest("https://fonts.google.com/$rpc/fonts.fe.catalog.actions.metadata.MetadataService/SampleText", twoLayerJSON);
            JsonParser *parser = json_parser_new();
            json_parser_load_from_data(parser, response.c_str(), response.size(), NULL);
            JsonNode *parseRoot = json_parser_get_root(parser);
            const gchar *sampleText = json_array_get_string_element(
                json_array_get_array_element(
                    json_node_get_array(parseRoot), 
                    2
                ), 
                2
            );
            this->_newSampleText = new std::string(sampleText);
            g_object_unref(parser);
        }
        {
            std::string response = sendPOSTRequest("https://fonts.google.com/$rpc/fonts.fe.catalog.actions.metadata.MetadataService/FamilyDetail", threeLayerJSON);
            JsonParser *parser = json_parser_new();
            json_parser_load_from_data(parser, response.c_str(), response.size(), NULL);
            JsonNode *parseRoot = json_parser_get_root(parser);
            JsonArray *familyDetail = json_array_get_array_element(
                json_array_get_array_element(
                    json_array_get_array_element(
                        json_node_get_array(parseRoot), 
                        0
                    ), 
                    0
                ),
                1
            );
            JsonArray *authorsArray = json_array_get_array_element(familyDetail, 1);
            int authorsLength = json_array_get_length(authorsArray);
            std::string authors = "";
            for (int i = 0; i < authorsLength; i++) {
                if (i > 0) {
                    authors += ", ";
                }
                JsonArray *authorInfo = json_array_get_array_element(authorsArray, i);
                authors += json_array_get_string_element(authorInfo, 0);
            }
            this->_newAuthors = new std::string(authors);
            g_object_unref(parser);
        }
        {
            std::string response = sendPOSTRequest("https://fonts.google.com/$rpc/fonts.fe.catalog.actions.metadata.MetadataService/License", threeLayerJSON);
            JsonParser *parser = json_parser_new();
            json_parser_load_from_data(parser, response.c_str(), response.size(), NULL);
            JsonNode *parseRoot = json_parser_get_root(parser);
            const gchar *license = json_array_get_string_element(
                json_array_get_array_element(
                    json_array_get_array_element(
                        json_node_get_array(parseRoot), 
                        0
                    ), 
                    0
                ),
                1
            );
            this->_newLicense = new std::string(license);
            replaceAllInString(this->_newLicense, "\r\n", "\n");
            replaceAllInString(this->_newLicense, "<p>", "<span>");
            replaceAllInString(this->_newLicense, "</p>", "</span>");
            replaceAllInString(this->_newLicense, "<h3>", "<big>");
            replaceAllInString(this->_newLicense, "</h3>", "</big>");
            replaceAllInString(this->_newLicense, "<ul>", "<span>");
            replaceAllInString(this->_newLicense, "</ul>", "</span>");
            replaceAllInString(this->_newLicense, "<li>\n    ", "<span>    - ");
            replaceAllInString(this->_newLicense, "<li>\n", "<span>    -");
            replaceAllInString(this->_newLicense, "<li>", "<span>    -");
            replaceAllInString(this->_newLicense, "</li>", "</span>");
            replaceAllInString(this->_newLicense, "&", "&amp;");
            g_object_unref(parser);
        }
        dispatcher->emit();
    }).detach();

    for (auto style : fontListItem->fontFamily->styles) {
        GoogleFontsStyleListItem *styleListItem = new GoogleFontsStyleListItem();
        styleListItem->googleFontsWindow = this;
        styleListItem->style = style;
        styleListItem->fontWidget = NULL;

        Gtk::Separator *separator = new Gtk::Separator();
        separator->show();
        this->specimenStyles->add(*separator);
        Gtk::Box *box = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
        styleListItem->box = box;
        box->set_margin_left(10);
        box->set_margin_right(10);
        box->set_margin_top(8);
        box->set_margin_bottom(8);

        box->set_spacing(8);


        Gtk::Label *styleText = new Gtk::Label();
        styleText->set_sensitive(false);
        styleText->set_text(
            std::string(weight_to_name(style->weight)) +
            " " +
            std::to_string(style->weight) +
            " " +
            slant_to_name(style->slant));
        styleText->set_alignment(Gtk::ALIGN_START);
        box->add(*styleText);

        Gtk::Label* lblPlaceholder = new Gtk::Label();
        auto context = lblPlaceholder->get_pango_context();
        auto fontDescription = context->get_font_description();
        fontDescription.set_size(26 * PANGO_SCALE);
        context->set_font_description(fontDescription);
        lblPlaceholder->set_text("");
        lblPlaceholder->set_alignment(Gtk::ALIGN_START);
        styleListItem->placeholderText = lblPlaceholder;
        box->add(*lblPlaceholder);

        box->show_all();
        this->specimenStyles->add(*box);

        std::string familyLoadText = style->family->family;
        familyLoadText += ":ital,wght@";
        if (style->slant == 0) {
            familyLoadText += "0";
        } else {
            familyLoadText += "1";
        }
        familyLoadText += ",";
        familyLoadText += std::to_string(style->weight);

        GoogleFontsFamilyLoadData *loadData = new GoogleFontsFamilyLoadData();
        loadData->family = familyLoadText;
        styleListItem->loadData = loadData;

        GCancellable* cancellable = g_cancellable_new();
        styleListItem->loadCancellable = cancellable;
        GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFontFamilyInList_callback_style, styleListItem);
        g_task_set_task_data(task,loadData,NULL);
        g_task_run_in_thread(task,GoogleFontsWindow_loadFontFamilyInList);

        this->styleListItems->push_back(styleListItem);
    }
    Gtk::Separator *separator = new Gtk::Separator();
    separator->show();
    this->specimenStyles->add(*separator);
}

void GoogleFontsWindow_fontFamilyLoaded(SushiFontWidget* fontWidget, GoogleFontsFontWidgetLoadData* data) {
    g_file_delete(data->tempFileG, NULL, NULL);
    gtk_widget_show(GTK_WIDGET(data->fontWidget));
    delete data->placeholderText;

    delete data;
}

void GoogleFontsWindow_fontFamilyError(SushiFontWidget* fontWidget, GError *error, GoogleFontsFontWidgetLoadData* data) {
    g_file_delete(data->tempFileG, NULL, NULL);
    data->placeholderText->set_text("Error loading font");

    delete data;
}

void GoogleFontsWindow_loadFontFamilyInList(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    GoogleFontsFamilyLoadData *loadData = (GoogleFontsFamilyLoadData*)task_data;
    std::regex sourceRegex(R"(src:\s*url\(([^)]+)\))");
    std::string css;
    std::smatch match;

    try {
        /*
            Returns TTF with no browser user agent

            Default: Roboto
            Normal: Roboto:ital,wght@0,100 AND Roboto:wght@100
            Italic: Roboto:ital,wght@1,100
        */
        css = loadStringFromURI("https://fonts.googleapis.com/css2?family="+Glib::uri_escape_string(loadData->family)+"&directory=3&display=block");
    } catch (Gio::Error &error) {
        std::cout << "An error has occured while loading the font " << loadData->family << ": " << error.what() << std::endl;
        g_task_return_boolean(task,false);
        return;
    }
    if (!std::regex_search(css, match, sourceRegex)) {
        std::cout << "Could not find font data in " << loadData->family << std::endl;
        g_task_return_boolean(task,false);
        return;
    }

    auto last = new std::string(match[match.size() - 1].str());

    std::string tempnameString = "/tmp/fontviewer_font_" + loadData->family + "_XXXXXX";

    char *tempname = new char[tempnameString.size() + 1];
    memcpy(tempname, tempnameString.c_str(), tempnameString.size() + 1);

    loadData->temppath = tempname;
    int tempFileDescriptor = mkstemp(tempname);

    FILE *tempFile = fdopen(tempFileDescriptor, "wb");
    
    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, last->c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, tempFile);
    curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    fflush(tempFile);
    fclose(tempFile);

    if (g_cancellable_is_cancelled(cancellable)) {
        remove(tempname);
    }

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

    GFile* tempFileG = g_file_new_for_path(listItem->loadData->temppath);
    char* tempFileURIG = g_file_get_uri(tempFileG);
    SushiFontWidget* fontWidget = sushi_font_widget_new(tempFileURIG, 0);

    sushi_font_widget_set_text(fontWidget, (gchar*)getPreviewTextForLanguage(listItem->fontFamily->language));
    
    GoogleFontsFontWidgetLoadData *loadData = new GoogleFontsFontWidgetLoadData();
    loadData->tempFileG = tempFileG;
    loadData->placeholderText = listItem->placeholderText;
    loadData->fontWidget = fontWidget;
    g_signal_connect(fontWidget,"loaded", G_CALLBACK(GoogleFontsWindow_fontFamilyLoaded), loadData);
    g_signal_connect(fontWidget,"error", G_CALLBACK(GoogleFontsWindow_fontFamilyError), loadData);
    
    gtk_widget_set_has_window(GTK_WIDGET(fontWidget), false); // Needed else the preview will take over focus/hover
    Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
    listItem->buttonBox->add(*fontWidgetMM);

    listItem->loadData = NULL;
    delete listItem->loadData;
}

void GoogleFontsWindow_loadFontFamilyInList_callback_style(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GoogleFontsStyleListItem* listItem = (GoogleFontsStyleListItem*)user_data;
    GError* error = NULL;
    bool returnBool = g_task_propagate_boolean(G_TASK(res),&error);
    if (returnBool == false || error != NULL) {
        listItem->placeholderText->set_text(_("Error loading"));
        return;
    }

    GFile* tempFileG = g_file_new_for_path(listItem->loadData->temppath);
    char* tempFileURIG = g_file_get_uri(tempFileG);
    SushiFontWidget* fontWidget = sushi_font_widget_new(tempFileURIG, 0);

    listItem->fontWidget = fontWidget;
    sushi_font_widget_set_text(fontWidget, listItem->googleFontsWindow->getStylePreviewText()->c_str());
    
    GoogleFontsFontWidgetLoadData *loadData = new GoogleFontsFontWidgetLoadData();
    loadData->tempFileG = tempFileG;
    loadData->placeholderText = listItem->placeholderText;
    loadData->fontWidget = fontWidget;
    g_signal_connect(fontWidget,"loaded", G_CALLBACK(GoogleFontsWindow_fontFamilyLoaded), loadData);
    g_signal_connect(fontWidget,"error", G_CALLBACK(GoogleFontsWindow_fontFamilyError), loadData);
    
    gtk_widget_set_has_window(GTK_WIDGET(fontWidget), false); // Needed else the preview will take over focus/hover
    Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
    listItem->box->add(*fontWidgetMM);

    listItem->loadData = NULL;
    delete listItem->loadData;
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

            GoogleFontsFamilyLoadData *loadData = new GoogleFontsFamilyLoadData();
            loadData->family = listItem->fontFamily->family;
            listItem->loadData = loadData;
            GCancellable* cancellable = g_cancellable_new();
            GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFontFamilyInList_callback,listItem);
            g_task_set_task_data(task,loadData,NULL);
            g_task_run_in_thread(task,GoogleFontsWindow_loadFontFamilyInList);
        }
    }
}

bool GoogleFontsWindow::queuedFontListScroll() {
    this->fontListScroll();
    return false;
}

void GoogleFontsWindow::searchUpdated() {
    if (this->stack->get_visible_child_name() != "list") {
        this->switchToFontList();
    }
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

std::string *GoogleFontsWindow::getStylePreviewText() {
    if (this->userOverridenStylePreviewText != NULL) {
        return this->userOverridenStylePreviewText;
    }
    return this->stylePreviewText;
}

void GoogleFontsWindow::updateStylePreview() {
    for (auto style : *this->styleListItems) {
        if (style->fontWidget != NULL) {
            sushi_font_widget_set_text(style->fontWidget, getStylePreviewText()->c_str());
            gtk_widget_queue_draw(GTK_WIDGET(style->fontWidget));
        }
    }
}

void GoogleFontsWindow::userOverridenStylePreviewTextChanged() {
    delete this->userOverridenStylePreviewText;
    std::string text = specimenStylesCustomPreviewEntry->get_text();
    if (text.empty()) {
        this->userOverridenStylePreviewText = NULL;
    } else {
        this->userOverridenStylePreviewText = new std::string(text);
    }

    this->updateStylePreview();
}

void GoogleFontsWindow::installButtonReload() {
    specimenInstallButton->get_style_context()->remove_class("suggested-action");
    specimenInstallButton->get_style_context()->remove_class("destructive-action");

    if (this->currentFontListItem == NULL) return;

    if (this->currentFontListItem->fontFamily->isInstalled) {
        specimenInstallButton->get_style_context()->add_class("destructive-action");
        specimenInstallButton->set_label(_("Uninstall"));
    } else {
        specimenInstallButton->get_style_context()->add_class("suggested-action");
        specimenInstallButton->set_label(_("Install"));
    }
}

void GoogleFontsWindow::installButtonClick() {
    this->specimenInstallButton->set_sensitive(false);
    GoogleFontsFamilyListItem *listItem = this->currentFontListItem;
    if (listItem->fontFamily->isInstalled) {
        this->specimenInstallButton->set_label(_("Uninstalling..."));
        Glib::Dispatcher *dispatcher = new Glib::Dispatcher();
        dispatcher->connect([this, listItem, dispatcher]() {
            this->specimenInstallButton->set_sensitive(true);
            this->installButtonReload();
            if (!listItem->fontFamily->isInstalled) {
                listItem->installedLabelWidget->hide();
                listItem->installedIconWidget->hide();
            }
            delete dispatcher;
        });

        std::vector<std::string> paths;
        for (FontFamilyData* fontFamily : *this->fontFamilies) {
            if (fontFamily->family == listItem->fontFamily->family) {
                for (auto path : *fontFamily->paths) {
                    paths.push_back(path);
                }
                break;
            }
        }
        for (auto path : listItem->fontFamily->paths) {
            paths.push_back(path);
        }
        try {
            for (auto path : paths) {
                auto file = Gio::File::create_for_path(path);
                file->remove();
            }
        } catch (const Gio::Error &error) {
            Gtk::MessageDialog* dialog = new Gtk::MessageDialog(*this,_("Error uninstalling font"),false,Gtk::MESSAGE_ERROR,Gtk::BUTTONS_OK,true);
            dialog->set_secondary_text(error.what());
            dialog->show_all();
            dialog->signal_response().connect_notify([dialog](int response){delete dialog;});
            dispatcher->emit();
            return;
        }
        listItem->fontFamily->paths.clear();
        if (paths.empty()) {
            Gtk::MessageDialog* dialog = new Gtk::MessageDialog(*this,_("Error uninstalling font"),false,Gtk::MESSAGE_ERROR,Gtk::BUTTONS_OK,true);
            dialog->set_secondary_text("No files for font found");
            dialog->show_all();
            dialog->signal_response().connect_notify([dialog](int response){delete dialog;});
            dispatcher->emit();
        } else {
            std::thread([dispatcher, listItem] () {
                listItem->fontFamily->isInstalled = false;
                int result = system("fc-cache -fv");
                if (result != 0) {
                    std::cerr << "Error updating cache" << std::endl;
                }
                dispatcher->emit();
            }).detach();
        }
    } else {
        this->specimenInstallButton->set_label(_("Installing..."));

        Glib::Dispatcher *dispatcher = new Glib::Dispatcher();
        dispatcher->connect([this, listItem, dispatcher]() {
            listItem->fontFamily->isInstalled = true;
            this->specimenInstallButton->set_sensitive(true);
            this->installButtonReload();
            listItem->installedLabelWidget->show();
            listItem->installedIconWidget->show();
            delete dispatcher;
        });

        std::thread([listItem, dispatcher] () {
            // Prepare font directory
            std::string userDataDir = Glib::get_user_data_dir();

            FcStrList* fontDirs = FcConfigGetFontDirs(NULL);
            FcChar8* fontDirPath;

            Glib::RefPtr<Gio::File> fontDirectory;


            while ((fontDirPath = FcStrListNext(fontDirs)) != NULL) {
                std::string fontDirPathStr((char*)fontDirPath);
                if (fontDirPathStr.rfind(userDataDir,0) == 0) {
                    fontDirectory = Gio::File::create_for_path(std::string((char*)fontDirPath));
                    break;
                }
            }

            if (!fontDirectory) {
                return;
            }

            if (!fontDirectory->query_exists()) {
                fontDirectory->make_directory_with_parents();
            }


            // Download fonts
            std::string family = listItem->fontFamily->family;

            std::string listResponse = loadStringFromURI("https://fonts.google.com/download/list?family=" + Glib::uri_escape_string(family));
            listResponse = listResponse.substr(5); // Remove ")]}'\n" that Google adds at the start

            JsonParser *parser = json_parser_new();
            json_parser_load_from_data(parser, listResponse.c_str(), listResponse.size(), NULL);

            JsonNode *root = json_parser_get_root(parser);
            JsonObject *rootObject = json_node_get_object(root);
            JsonObject *manifest = json_object_get_object_member(rootObject, "manifest");
            JsonArray *fileRefs = json_object_get_array_member(manifest, "fileRefs");
            int fileRefsLength = json_array_get_length(fileRefs);

            bool fontInstalled = false;

            for (int i = 0; i < fileRefsLength; i++) {
                JsonObject *fileRef = json_array_get_object_element(fileRefs, i);

                std::string url = json_object_get_string_member(fileRef, "url");
                std::string filename = json_object_get_string_member(fileRef, "filename");

                if (filename.find("/") != std::string::npos) {
                    continue;
                }
                
                auto finalFile = fontDirectory->get_child(filename);;
                if (finalFile->query_exists()) {
                    continue;
                }

                auto path = finalFile->get_path();

                FILE *file = fopen(path.c_str(), "w");

                CURL *curl = curl_easy_init();
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
                curl_easy_perform(curl);

                curl_easy_cleanup(curl);

                fflush(file);
                fclose(file);

                fontInstalled = true;
                listItem->fontFamily->paths.push_back(path);
            }

            g_object_unref(parser);

            if (fontInstalled) {
                int result = system("fc-cache -fv");
                if (result != 0) {
                    std::cerr << "Error updating cache" << std::endl;
                }
            }
            
            dispatcher->emit();
        }).detach();
    }
}
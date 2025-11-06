#include "google-fonts-window.hpp"
#include "src/googlefonts/family.hpp"
#include <json-glib/json-glib.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/separator.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/adjustment.h>
#include <glibmm/dispatcher.h>
#include <glibmm/uriutils.h>
#include <glibmm/miscutils.h>
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Firefox/50");
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
        ".disabled { color: @insensitive_fg_color; }"
        );
    this->get_style_context()->add_provider_for_display(Gdk::Display::get_default(), provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    this->stylePreviewText = "";
    this->userOverridenStylePreviewText = "";
    this->currentFontListItem = NULL;

    this->set_default_size(1000, 700);
    this->set_title("Google Fonts");

    // headerBar.set_show_close_button();
    headerBarCustomText.show();
    backButton.set_image_from_icon_name("go-previous-symbolic");
    backButton.signal_clicked().connect(sigc::mem_fun(*this,&GoogleFontsWindow::switchToFontList));
    headerBar.pack_start(backButton);
    this->set_titlebar(headerBar);

    searchEntry.set_placeholder_text(_("Search..."));
    searchEntry.signal_changed().connect(sigc::mem_fun(*this,&GoogleFontsWindow::searchUpdated));
    searchEntry.set_sensitive(false);
    headerBar.pack_end(searchEntry);
    // this->signal_key_press_event().connect(sigc::mem_fun(*this, &GoogleFontsWindow::windowKeyPressEvent));

    stack.set_transition_duration(200);
    stack.set_transition_type(Gtk::StackTransitionType::CROSSFADE);
    this->set_child(stack);

    spinner.start();
    stack.add(spinner, "loading");

    // scrolledWindow.get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this,&GoogleFontsWindow::fontListScroll));
    familyListBox.set_orientation(Gtk::Orientation::VERTICAL);
    scrolledWindow.set_child(familyListBox);
    stack.add(scrolledWindow, "list");

    boxSpecimen.set_orientation(Gtk::Orientation::VERTICAL);
    boxSpecimen.set_margin_start(60);
    boxSpecimen.set_margin_end(60);
    boxSpecimen.set_margin_top(18);
    boxSpecimen.set_spacing(8);

    // specimenTitle.set_alignment(Gtk::ALIGN_START);
    setFontSizeOfLabel(specimenTitle, 32);
    specimenHeader.append(specimenTitle);

    specimenInstallButton.set_valign(Gtk::Align::START);
    specimenInstallButton.signal_clicked().connect(sigc::mem_fun(*this, &GoogleFontsWindow::installButtonClick));
    specimenHeader.append(specimenInstallButton);

    boxSpecimen.append(specimenHeader);

    // specimenAuthors.set_alignment(Gtk::ALIGN_START);
    specimenAuthors.set_sensitive(false);
    specimenAuthors.set_margin_bottom(12);
    boxSpecimen.append(specimenAuthors);

    // specimenStylesLabel.set_alignment(Gtk::ALIGN_START);
    setFontSizeOfLabel(specimenStylesLabel, 18);
    specimenStylesLabel.set_text(_("Styles"));
    boxSpecimen.append(specimenStylesLabel);

    specimenStylesCustomPreviewEntry.set_placeholder_text(_("Enter text to preview in the font"));
    specimenStylesCustomPreviewEntry.signal_changed().connect(sigc::mem_fun(*this, &GoogleFontsWindow::userOverridenStylePreviewTextChanged));
    boxSpecimen.append(specimenStylesCustomPreviewEntry);

    specimenStyles.set_orientation(Gtk::Orientation::VERTICAL);
    boxSpecimen.append(specimenStyles);

    swSpecimen.set_child(boxSpecimen);
    notebook.append_page(swSpecimen, _("Specimen"));

    boxLicense.set_orientation(Gtk::Orientation::VERTICAL);
    boxLicense.set_margin_start(60);
    boxLicense.set_margin_end(60);
    boxLicense.set_margin_top(18);
    boxLicense.set_spacing(12);

    // licenseTitleLabel.set_alignment(Gtk::ALIGN_START);
    setFontSizeOfLabel(licenseTitleLabel, 18);
    licenseTitleLabel.set_text(_("License"));
    boxLicense.append(licenseTitleLabel);

    // licenseLabel.set_alignment(Gtk::ALIGN_START);
    licenseLabel.set_selectable(true);
    boxLicense.append(licenseLabel);

    swLicense.set_child(boxLicense);
    notebook.append_page(swLicense, _("License"));

    stack.add(notebook, "view");

    GCancellable* cancellable = g_cancellable_new();
    GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFamilies_callback,this);
    g_task_set_task_data(task,this,NULL);
    g_task_run_in_thread(task,GoogleFontsWindow_loadFamilies);

    this->show();
    backButton.hide();
}

void GoogleFontsWindow_loadFamilies(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    GoogleFontsWindow *self = (GoogleFontsWindow*)task_data;
    std::string response = loadStringFromURI("https://fonts.google.com/metadata/fonts");

    g_autoptr(JsonParser) parser = json_parser_new();
    g_autoptr(GError) error = NULL;

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
        auto family = std::make_shared<GoogleFontsFamily>();
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

            auto style = std::make_shared<GoogleFontsStyle>();
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

        self->families.push_back(family);
    }

    g_task_return_boolean(task, true);
}

void GoogleFontsWindow_loadFamilies_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GoogleFontsWindow* self = (GoogleFontsWindow*)user_data;

    self->stack.set_visible_child("list");

    std::sort(self->families.begin(), self->families.end(), [](std::shared_ptr<GoogleFontsFamily> a, std::shared_ptr<GoogleFontsFamily> b) {
        return a->sortTrending < b->sortTrending;
    });

    for (auto i : self->families) {
        auto fontListItem = std::make_shared<GoogleFontsFamilyListItem>();
        fontListItem->fontFamily = i;
        fontListItem->hasBeenViewed = false;

        Gtk::Button* btn = Gtk::make_managed<Gtk::Button>();
        fontListItem->button = btn;

        btn->set_has_frame(false);
        btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*self, &GoogleFontsWindow::switchToFontFamily), fontListItem));

        Gtk::Box *btnBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        fontListItem->buttonBox = btnBox;
        btnBox->set_margin_start(8);
        btnBox->set_margin_end(8);
        btnBox->set_margin_top(4);
        btnBox->set_margin_bottom(4);
        btnBox->set_spacing(8);

        Gtk::Box* btnHeaderBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL);
        btnHeaderBox->set_spacing(4);

        Gtk::Label* btnLabel = Gtk::make_managed<Gtk::Label>();
        btnLabel->set_halign(Gtk::Align::START);
        btnLabel->set_text(i->displayName);
        btnLabel->get_style_context()->add_class("display-name");
        btnHeaderBox->append(*btnLabel);
        
        if (i->styles.size() > 1) {
            Gtk::Label* btnStyleCount = Gtk::make_managed<Gtk::Label>();
            btnStyleCount->set_sensitive(false);
            btnStyleCount->get_style_context()->add_class("disabled");
            btnStyleCount->set_halign(Gtk::Align::START);
            btnStyleCount->set_text(Glib::ustring::compose(_("%1 styles"),std::to_string(i->styles.size())));
            btnHeaderBox->append(*btnStyleCount);
        }
        
        Gtk::Label* installedLabel = Gtk::make_managed<Gtk::Label>();
        installedLabel->set_valign(Gtk::Align::CENTER);
        installedLabel->set_text(_("Installed"));
        installedLabel->set_sensitive(false);
        installedLabel->get_style_context()->add_class("disabled");
        btnHeaderBox->append(*installedLabel);
        fontListItem->installedLabelWidget = installedLabel;

        Gtk::Image *installedIcon = Gtk::make_managed<Gtk::Image>();
        installedIcon->set_valign(Gtk::Align::CENTER);
        installedIcon->set_from_icon_name("emblem-ok");
        installedIcon->get_style_context()->add_class("disabled");
        btnHeaderBox->append(*installedIcon);
        fontListItem->installedIconWidget = installedIcon;

        btnBox->append(*btnHeaderBox);

        Gtk::Label* lblPlaceholder = Gtk::make_managed<Gtk::Label>();
        lblPlaceholder->set_size_request(0, 40);
        lblPlaceholder->set_text("");
        lblPlaceholder->set_halign(Gtk::Align::START);
        btnBox->append(*lblPlaceholder);
        fontListItem->placeholderText = lblPlaceholder;

        btn->set_child(*btnBox);

        btn->show();

        if (!i->isInstalled) {
            fontListItem->installedLabelWidget->hide();
            fontListItem->installedIconWidget->hide();
        }

        self->familyListBox.append(*btn);

        i->button = btn;
        self->fontListItems.push_back(fontListItem);
    }
    // self->signal_check_resize().connect(sigc::mem_fun(*self,&GoogleFontsWindow::fontListScroll));
    
    self->searchEntry.set_sensitive(true);
    self->searchEntry.grab_focus();
}

void GoogleFontsWindow::switchToFontList() {
    this->backButton.hide();
    this->stack.set_visible_child("list");
    this->currentFontListItem = NULL;

    // gtk_header_bar_set_custom_title(headerBar.gobj(), NULL);
}

void GoogleFontsWindow::loadLicense() {
    Glib::Dispatcher* dispatcher = new Glib::Dispatcher();
    std::string* licenseText = new std::string();

    this->licenseLabel.set_text("");

    JsonArray *array = json_array_new();
    JsonArray *array2 = json_array_new();
    JsonArray *array3 = json_array_new();
    json_array_add_string_element(array3, this->currentFontListItem->fontFamily->family.c_str());
    json_array_add_array_element(array2, array3);
    json_array_add_array_element(array, array2);
    JsonNode *root = json_node_new(JSON_NODE_ARRAY);
    json_node_set_array(root, array);
    gchar *json = json_to_string(root, false);
    g_free(array);
    g_free(array2);
    g_free(array3);

    dispatcher->connect([this, dispatcher, licenseText]() {
        this->licenseLabel.set_markup(*licenseText);

        delete dispatcher;
        delete licenseText;
    });

    std::thread([json, licenseText, dispatcher]() {
        std::string response = sendPOSTRequest("https://fonts.google.com/$rpc/fonts.fe.catalog.actions.metadata.MetadataService/License", json);
        JsonParser *parser = json_parser_new();
        json_parser_load_from_data(parser, response.c_str(), response.size(), NULL);
        JsonNode *parseRoot = json_parser_get_root(parser);
        *licenseText = json_array_get_string_element(
            json_array_get_array_element(
                json_array_get_array_element(
                    json_node_get_array(parseRoot),
                    0
                ),
                0
            ),
            1
        );
        replaceAllInString(*licenseText, "\r\n", "\n");
        replaceAllInString(*licenseText, "<p>", "<span>");
        replaceAllInString(*licenseText, "</p>", "</span>");
        replaceAllInString(*licenseText, "<h3>", "<big>");
        replaceAllInString(*licenseText, "</h3>", "</big>");
        replaceAllInString(*licenseText, "<ul>", "<span>");
        replaceAllInString(*licenseText, "</ul>", "</span>");
        replaceAllInString(*licenseText, "<li>\n    ", "<span>    - ");
        replaceAllInString(*licenseText, "<li>\n", "<span>    -");
        replaceAllInString(*licenseText, "<li>", "<span>    -");
        replaceAllInString(*licenseText, "</li>", "</span>");
        replaceAllInString(*licenseText, "&", "&amp;");
        g_object_unref(parser);

        dispatcher->emit();
    }).detach();
}

void GoogleFontsWindow::loadFamilyDetails() {
    Glib::Dispatcher* dispatcher = new Glib::Dispatcher();
    std::string* authors = new std::string();
    
    this->specimenAuthors.set_text("");
    
    JsonArray *array = json_array_new();
    JsonArray *array2 = json_array_new();
    JsonArray *array3 = json_array_new();
    json_array_add_string_element(array3, this->currentFontListItem->fontFamily->family.c_str());
    json_array_add_array_element(array2, array3);
    json_array_add_array_element(array, array2);
    JsonNode *root = json_node_new(JSON_NODE_ARRAY);
    json_node_set_array(root, array);
    gchar *json = json_to_string(root, false);
    g_free(array);
    g_free(array2);
    g_free(array3);

    dispatcher->connect([this, dispatcher, authors]() {
        this->specimenAuthors.set_text(Glib::ustring::compose(_("Designed by %1"), *authors));
        
        delete dispatcher;
        delete authors;
    });

    std::thread([json, authors, dispatcher]() {
        std::string response = sendPOSTRequest("https://fonts.google.com/$rpc/fonts.fe.catalog.actions.metadata.MetadataService/FamilyDetail", json);
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
        for (int i = 0; i < authorsLength; i++) {
            if (i > 0) {
                *authors += ", ";
            }
            JsonArray *authorInfo = json_array_get_array_element(authorsArray, i);
            *authors += json_array_get_string_element(authorInfo, 0);
        }
        g_object_unref(parser);
        dispatcher->emit();
    }).detach();
}

void GoogleFontsWindow::loadSampleText() {
    Glib::Dispatcher* dispatcher = new Glib::Dispatcher();
    std::string* sampleText = new std::string();

    JsonArray *array = json_array_new();
    JsonArray *array2 = json_array_new();
    json_array_add_string_element(array2, this->currentFontListItem->fontFamily->family.c_str());
    json_array_add_array_element(array, array2);
    JsonNode *root = json_node_new(JSON_NODE_ARRAY);
    json_node_set_array(root, array);
    gchar *json = json_to_string(root, false);
    g_free(array);
    g_free(array2);

    dispatcher->connect([this, dispatcher, sampleText]() {
        this->stylePreviewText = *sampleText;
        this->updateStylePreview();

        delete dispatcher;
    });

    std::thread([json, dispatcher, sampleText]() {
        std::string response = sendPOSTRequest("https://fonts.google.com/$rpc/fonts.fe.catalog.actions.metadata.MetadataService/SampleText", json);
        JsonParser *parser = json_parser_new();
        json_parser_load_from_data(parser, response.c_str(), response.size(), NULL);
        JsonNode *parseRoot = json_parser_get_root(parser);
        *sampleText = json_array_get_string_element(
            json_array_get_array_element(
                json_node_get_array(parseRoot), 
                2
            ), 
            2
        );
        g_object_unref(parser);

        dispatcher->emit();
    }).detach();
}

void GoogleFontsWindow::switchToFontFamily(std::shared_ptr<GoogleFontsFamilyListItem> fontListItem) {
    this->currentFontListItem = fontListItem;
    this->backButton.show();
    this->stack.set_visible_child("view");
    this->notebook.set_current_page(0);
    // this->swSpecimen.get_vadjustment()->set_value(0);

    headerBarCustomText.set_text(fontListItem->fontFamily->displayName);
    // headerBar.set_custom_title(headerBarCustomText);

    specimenTitle.set_text(fontListItem->fontFamily->displayName);

    this->installButtonReload();

    for (auto styleListItem : this->styleListItems) {
        g_cancellable_cancel(styleListItem->loadCancellable);
    }
    this->styleListItems.clear();

    Gtk::Widget* styleChild = this->specimenStyles.get_first_child();
    while (styleChild != NULL) {
        delete styleChild;
        styleChild = this->specimenStyles.get_first_child();
    }

    this->loadLicense();
    this->loadFamilyDetails();
    this->loadSampleText();

    for (auto style : fontListItem->fontFamily->styles) {
        auto styleListItem = std::make_shared<GoogleFontsStyleListItem>();
        styleListItem->googleFontsWindow = this;
        styleListItem->style = style;
        styleListItem->fontWidget = NULL;

        Gtk::Separator *separator = Gtk::make_managed<Gtk::Separator>();
        separator->show();
        this->specimenStyles.append(*separator);
        Gtk::Box *box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
        styleListItem->box = box;
        box->set_margin_start(10);
        box->set_margin_end(10);
        box->set_margin_top(8);
        box->set_margin_bottom(8);

        box->set_spacing(8);


        Gtk::Label *styleText = Gtk::make_managed<Gtk::Label>();
        styleText->set_sensitive(false);
        styleText->set_text(
            std::string(weight_to_name(style->weight)) +
            " " +
            std::to_string(style->weight) +
            " " +
            slant_to_name(style->slant));
        styleText->set_halign(Gtk::Align::START);
        box->append(*styleText);

        Gtk::Label* lblPlaceholder = Gtk::make_managed<Gtk::Label>();
        lblPlaceholder->set_size_request(0, 40);
        lblPlaceholder->set_text("");
        lblPlaceholder->set_halign(Gtk::Align::START);
        styleListItem->placeholderText = lblPlaceholder;
        box->append(*lblPlaceholder);

        box->show();
        this->specimenStyles.append(*box);

        std::string familyLoadText = style->family->family;
        familyLoadText += ":ital,wght@";
        if (style->slant == 0) {
            familyLoadText += "0";
        } else {
            familyLoadText += "1";
        }
        familyLoadText += ",";
        familyLoadText += std::to_string(style->weight);

        auto loadData = std::make_shared<GoogleFontsFamilyLoadData>();
        loadData->family = familyLoadText;
        loadData->language = style->family->language;
        styleListItem->loadData = loadData;

        GCancellable* cancellable = g_cancellable_new();
        styleListItem->loadCancellable = cancellable;
        GTask* task = g_task_new(
            this->gobj(),
            cancellable,
            GoogleFontsWindow_loadFontFamilyInList_callback_style,
            new std::shared_ptr<GoogleFontsStyleListItem>(styleListItem)
        );
        g_task_set_task_data(task,new std::shared_ptr<GoogleFontsFamilyLoadData>(loadData),NULL);
        g_task_run_in_thread(task,GoogleFontsWindow_loadFontFamilyInList);

        this->styleListItems.push_back(styleListItem);
    }
    Gtk::Separator *separator = new Gtk::Separator();
    separator->show();
    this->specimenStyles.append(*separator);
}

void GoogleFontsWindow_loadFontFamilyInList(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    auto loadData = *(std::shared_ptr<GoogleFontsFamilyLoadData>*)task_data;
    std::regex sourceRegex(R"(src:\s*url\(([^)]+)\))");
    std::string css;
    std::smatch match;
    std::string previewText = getPreviewTextForLanguage(loadData->language);
    std::string cssUri = "https://fonts.googleapis.com/css2?family="
                        + Glib::uri_escape_string(loadData->family, "", false)
                        + "&directory=3&display=block&text="
                        + Glib::uri_escape_string(previewText + "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnoprqtstuvwxyz", "", false);

    try {
        /*
            Returns WOFF2

            Default: Roboto
            Normal: Roboto:ital,wght@0,100 AND Roboto:wght@100
            Italic: Roboto:ital,wght@1,100
        */
        css = loadStringFromURI(cssUri);
    } catch (Gio::Error &error) {
        std::cerr << "An error has occured while loading the font " << loadData->family << ": " << error.what() << std::endl;
        g_task_return_new_error_literal(task, g_quark_from_static_string("Error"), 1, "Error");
        return;
    }
    if (!std::regex_search(css, match, sourceRegex)) {
        std::cerr << "Could not find font data in " << loadData->family << std::endl;
        g_task_return_new_error_literal(task, g_quark_from_static_string("Error"), 1, "Error");
        return;
    }

    std::string last = match[match.size() - 1].str();

    std::vector<uint8_t>* data = new std::vector<uint8_t>();

    CURL *curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, last.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallbackBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_perform(curl);

    curl_easy_cleanup(curl);

    g_task_return_pointer(task, data, NULL);

    delete (std::shared_ptr<GoogleFontsFamilyLoadData>*)task_data;
}


void GoogleFontsWindow_loadFontFamilyInList_callback(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    auto listItem = *(std::shared_ptr<GoogleFontsFamilyListItem>*)user_data;
    GError* error = NULL;
    std::vector<uint8_t>* data = (std::vector<uint8_t>*)g_task_propagate_pointer(G_TASK(res), &error);
    if (error != NULL) {
        listItem->placeholderText->set_text(_("Error loading"));
        return;
    }

    SushiFontWidget* fontWidget = sushi_font_widget_new_from_bytes((gchar*)data->data(), data->size(), 0);
    delete data;

    sushi_font_widget_set_text(fontWidget, (gchar*)getPreviewTextForLanguage(listItem->fontFamily->language));

    Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
    listItem->buttonBox->append(*fontWidgetMM);
    fontWidgetMM->show();

    delete listItem->placeholderText;
    delete (std::shared_ptr<GoogleFontsFamilyListItem>*)user_data;
}

void GoogleFontsWindow_loadFontFamilyInList_callback_style(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    auto listItem = *(std::shared_ptr<GoogleFontsStyleListItem>*)user_data;
    GError* error = NULL;
    std::vector<uint8_t>* data = (std::vector<uint8_t>*)g_task_propagate_pointer(G_TASK(res), &error);
    if (error != NULL) {
        listItem->placeholderText->set_text(_("Error loading"));
        return;
    }

    delete listItem->placeholderText;

    SushiFontWidget* fontWidget = sushi_font_widget_new_from_bytes((gchar*)data->data(), data->size(), 0);

    listItem->fontWidget = fontWidget;
    std::string *previewText = new std::string(listItem->googleFontsWindow->getStylePreviewText());
    sushi_font_widget_set_text(fontWidget, previewText->c_str());

    Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
    listItem->box->append(*fontWidgetMM);
    fontWidgetMM->show();

    delete (std::shared_ptr<GoogleFontsStyleListItem>*)user_data;
}


void GoogleFontsWindow::fontListScroll() {
    double scrollPosition = scrolledWindow.get_vadjustment()->get_value();
    for (auto listItem : fontListItems) {
        if (listItem->hasBeenViewed) continue;
        if (!listItem->button->get_visible()) continue;

        Gtk::Allocation allocation = listItem->button->get_allocation();
        int y = allocation.get_y();
        int windowHeight = this->scrolledWindow.get_height();
        if (y > scrollPosition - 256 && y < scrollPosition + windowHeight + 256) {
            listItem->hasBeenViewed = true;

            auto loadData = std::make_shared<GoogleFontsFamilyLoadData>();
            loadData->family = listItem->fontFamily->family;
            loadData->language = listItem->fontFamily->language;
            listItem->loadData = loadData;
            GCancellable* cancellable = g_cancellable_new();
            GTask* task = g_task_new(this->gobj(),cancellable,GoogleFontsWindow_loadFontFamilyInList_callback,new std::shared_ptr<GoogleFontsFamilyListItem>(listItem));
            g_task_set_task_data(task,new std::shared_ptr<GoogleFontsFamilyLoadData>(loadData),NULL);
            g_task_run_in_thread(task,GoogleFontsWindow_loadFontFamilyInList);
        }
    }
}

bool GoogleFontsWindow::queuedFontListScroll() {
    this->fontListScroll();
    return false;
}

void GoogleFontsWindow::searchUpdated() {
    if (this->stack.get_visible_child_name() != "list") {
        this->switchToFontList();
    }
    std::string input = this->searchEntry.get_text().lowercase();
    for (auto listItem : fontListItems) {
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

std::string GoogleFontsWindow::getStylePreviewText() {
    if (this->userOverridenStylePreviewText != "") {
        return this->userOverridenStylePreviewText;
    }
    return this->stylePreviewText;
}

void GoogleFontsWindow::updateStylePreview() {
    for (auto style : this->styleListItems) {
        if (style->fontWidget != NULL) {
            std::string *string = new std::string(getStylePreviewText());
            sushi_font_widget_set_text(style->fontWidget, string->c_str());
            gtk_widget_queue_draw(GTK_WIDGET(style->fontWidget));
        }
    }
}

void GoogleFontsWindow::userOverridenStylePreviewTextChanged() {
    std::string text = specimenStylesCustomPreviewEntry.get_text();
    this->userOverridenStylePreviewText = text;

    this->updateStylePreview();
}

void GoogleFontsWindow::installButtonReload() {
    specimenInstallButton.get_style_context()->remove_class("suggested-action");
    specimenInstallButton.get_style_context()->remove_class("destructive-action");

    if (this->currentFontListItem == NULL) return;

    if (this->currentFontListItem->fontFamily->isInstalled) {
        specimenInstallButton.get_style_context()->add_class("destructive-action");
        specimenInstallButton.set_label(_("Uninstall"));
    } else {
        specimenInstallButton.get_style_context()->add_class("suggested-action");
        specimenInstallButton.set_label(_("Install"));
    }
}

void GoogleFontsWindow::installButtonClick() {
    this->specimenInstallButton.set_sensitive(false);
    auto listItem = this->currentFontListItem;
    if (listItem->fontFamily->isInstalled) {
        this->specimenInstallButton.set_label(_("Uninstalling..."));
        Glib::Dispatcher *dispatcher = new Glib::Dispatcher();
        dispatcher->connect([this, listItem, dispatcher]() {
            this->specimenInstallButton.set_sensitive(true);
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
            Gtk::MessageDialog* dialog = new Gtk::MessageDialog(
                *this,
                _("Error uninstalling font"),
                false,
                Gtk::MessageType::ERROR,
                Gtk::ButtonsType::OK,
                true
            );
            dialog->set_secondary_text(error.what());
            dialog->show();
            dialog->signal_response().connect([dialog](int response){delete dialog;});
            dispatcher->emit();
            return;
        }
        listItem->fontFamily->paths.clear();
        if (paths.empty()) {
            Gtk::MessageDialog* dialog = new Gtk::MessageDialog(
                *this,
                _("Error uninstalling font"),
                false,
                Gtk::MessageType::ERROR,
                Gtk::ButtonsType::OK,
                true
            );
            dialog->set_secondary_text("No files for font found");
            dialog->show();
            dialog->signal_response().connect([dialog](int response){delete dialog;});
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
        this->specimenInstallButton.set_label(_("Installing..."));

        Glib::Dispatcher *dispatcher = new Glib::Dispatcher();
        dispatcher->connect([this, listItem, dispatcher]() {
            listItem->fontFamily->isInstalled = true;
            this->specimenInstallButton.set_sensitive(true);
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

bool GoogleFontsWindow::windowKeyPressEvent(guint keyval, guint keycode, Gdk::ModifierType state) {
    if (keyval == GDK_KEY_f && state == Gdk::ModifierType::CONTROL_MASK) {
        this->searchEntry.grab_focus();
        return true;
    }
    return false;
}

GoogleFontsWindow::~GoogleFontsWindow() {
    for (auto styleListItem : this->styleListItems) {
        g_cancellable_cancel(styleListItem->loadCancellable);
    }
    this->styleListItems.clear();

    for (auto family : this->families) {
        family->styles.clear();
    }
    this->families.clear();

    this->fontListItems.clear();
}

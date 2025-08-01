#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sys/stat.h>
#include <gtkmm.h>
#include <glib/gi18n.h>
#include <fontconfig/fontconfig.h>
#include <locale.h>
#include FT_FREETYPE_H
#include FT_TYPE1_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include FT_MULTIPLE_MASTERS_H
#include <freetype2/ft2build.h>
#include "sushi-font-widget.h"
#include "pangram.hpp"
#include "font.hpp"
#include "mainwindow.hpp"
#include "utils.hpp"

struct FontWidgetLoadData;
class MainWindow;
void MainWindow_fontWidgetLoaded(SushiFontWidget*, FontWidgetLoadData*);
void MainWindow_installFontTask(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
void MainWindow_installFontFinished(GObject *source_object, GAsyncResult *res, gpointer user_data);

struct FontWidgetLoadData {
    MainWindow* self;
    SushiFontWidget* fontWidget;
};

void loadFontFamiliesFromSet(FcFontSet* set, std::vector<FontFamilyData*>* list, bool grouped) {
    std::map<std::string,FontFamilyData*> familyMap{};

    for (int i = 0; i < set->nfont; i++) {
        FcPattern* font = set->fonts[i];
        FcChar8* familyChar;
        FcChar8* pathChar;
        FcChar8* styleChar;
        int fcWeight;
        int slant;
        int fontIndex;
        FcPatternGetString(font,FC_FAMILY,0,&familyChar);
        FcPatternGetString(font,FC_FILE,0,&pathChar);
        FcPatternGetString(font,FC_STYLE,0,&styleChar);
        FcPatternGetInteger(font,FC_WEIGHT,0,&fcWeight);
        FcPatternGetInteger(font,FC_SLANT,0,&slant);
        FcPatternGetInteger(font,FC_INDEX,0,&fontIndex);

        std::string family((char*)familyChar);
        std::string styleName((char*)styleChar);

        // TO SIMPLIFY LOADING
        bool isModifiedGrouping = false;
        if (grouped) {
            if (family.rfind("Noto Sans",0) == 0) {
                family = "Noto Sans";
                isModifiedGrouping = true;
            } else if (family.rfind("Noto Serif",0) == 0) {
                family = "Noto Serif";
                isModifiedGrouping = true;
            } else if (family.rfind("Noto",0) == 0) {
                family = "Noto (Other)";
                isModifiedGrouping = true;
            }
        }

        FontFamilyData* familyData = NULL;
        if (familyMap.count(family) == 1) {
            familyData = familyMap[family];
        } else {
            familyData = new FontFamilyData();
            familyData->family = family;
            struct stat fileStat;
            if (stat((char*)pathChar,&fileStat) == 0) {
                familyData->fileCreationTime = fileStat.st_mtim.tv_sec;
            } else {
                std::cout << "Error loading file stats for " << pathChar << " (" << familyData->family << ")" << std::endl;
            }
            familyData->styles = new std::vector<FontStyleData*>();
            familyData->paths = new std::vector<std::string>();
            familyData->isModifiedGrouping = isModifiedGrouping;
            familyMap[family] = familyData;
        }
        FontStyleData* styleData = new FontStyleData();
        styleData->path = std::string((char*)pathChar);
        if (std::find(familyData->paths->begin(),familyData->paths->end(),styleData->path) == familyData->paths->end()) {
            familyData->paths->push_back(styleData->path);
        }
        styleData->name = styleName;
        styleData->family = familyData;
        styleData->weight = fc_weight_to_weight(fcWeight);
        styleData->slant = fc_slant_to_slant(slant);
        styleData->index = fontIndex;
        styleData->sortingOrder = styleData->weight + styleData->slant;
        familyData->styles->push_back(styleData);
    }

    for (auto const& x: familyMap) {
        std::sort(x.second->styles->begin(),x.second->styles->end(),[](FontStyleData* a, FontStyleData* b) {
            return a->sortingOrder < b->sortingOrder;
        });
        list->push_back(x.second);
    }
    std::sort(list->begin(),list->end(),[](FontFamilyData* a, FontFamilyData* b) {
        return a->fileCreationTime > b->fileCreationTime;
    });
    
}

std::vector<FontFamilyData*>* loadFonts(bool grouped) {
    auto fontFamilies = new std::vector<FontFamilyData*>();
    FcPattern* p = FcPatternCreate();
    FcObjectSet* objectSet = FcObjectSetBuild(FC_FILE,FC_INDEX,FC_FAMILY,FC_WEIGHT,FC_SLANT,FC_STYLE,NULL);
    FcFontSet* fontSet = FcFontList(NULL,p,objectSet);
    FcPatternDestroy(p);
    FcObjectSetDestroy(objectSet);
    loadFontFamiliesFromSet(fontSet,fontFamilies,grouped);
    FcFontSetDestroy(fontSet);
    return fontFamilies;
}

MainWindow::MainWindow(std::vector<FontFamilyData*>* fonts, std::string* defaultFileName) {
    this->fontFamilies = fonts;
    auto provider = Gtk::CssProvider::create();
    provider->load_from_data(
        ".font-style-window {background-color: @insensitive_base_color;} "
        ".font-family-window {} "
        ".font-family-box {padding: 12px;} "
        ".font-family-label {font-size: 1.4em;font-weight:600;} "
        ".sushi-font-widget {background: @theme_bg_color;color: @theme_fg_color;} "
        ".font-info-name {font-weight: bold;} "
        ".font-style-button {background: none; border: none;} " // at the moment there are no plans to implement an action on button click, so just hide the button style
        );
    this->get_style_context()->add_provider_for_screen(Gdk::Screen::get_default(),provider,GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    fontListItems = new std::vector<FontListItem*>();
    fontStyleListItems = new std::vector<FontStyleListItem*>();
    fontStyleRows = new std::vector<FontStyleRow*>();
    familyData = NULL;
    currentPreviewText = NULL;
    currentFontPath = NULL;
    currentFontIndex = -1;
    googleFontsWindow = NULL;
    headerBar = new Gtk::HeaderBar();
    headerBar->set_title(_("All Fonts"));
    headerBar->set_show_close_button();
    headerBarCustomText = new Gtk::Label();
    backButton = new Gtk::Button();
    backButton->set_image_from_icon_name("go-previous-symbolic");
    backButton->signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::switchToFontList));
    headerBar->add(*backButton);

    installButton = new Gtk::Button();
    installButton->set_label("");
    installButton->signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::installFontClicked));
    headerBar->pack_end(*installButton);

    searchButton = new Gtk::ToggleButton();
    searchButton->set_image_from_icon_name("edit-find-symbolic");
    headerBar->pack_end(*searchButton);

    googleFontsButton = new Gtk::Button();
    googleFontsButton->set_tooltip_text("Google Fonts");
    googleFontsButton->set_image_from_icon_name("fontviewer-google-symbolic");
    googleFontsButton->signal_clicked().connect(sigc::mem_fun(*this,&MainWindow::openGoogleFonts));
    headerBar->pack_end(*googleFontsButton);
    
    this->set_size_request(100,100);
    this->set_default_size(950,600);
    this->set_title(_("Fonts"));
    this->set_titlebar(*headerBar);
    this->set_default_icon_name("fonts");

    Gtk::Box* mainBox = new Gtk::Box();
    mainBox->set_orientation(Gtk::ORIENTATION_VERTICAL);

    searchBar = new Gtk::SearchBar();
    searchEntry = new Gtk::SearchEntry();
    searchEntry->signal_changed().connect(sigc::mem_fun(*this,&MainWindow::searchUpdated));
    searchBar->add(*searchEntry);
    mainBox->add(*searchBar);
    this->signal_key_press_event().connect(sigc::mem_fun(*this,&MainWindow::windowKeyPressEvent));

    // had to do C workaround because weirdly Glib::Binding::bind_property wasn't working
    g_object_bind_property(searchBar->gobj(), "search-mode-enabled", searchButton->gobj(), "active", G_BINDING_BIDIRECTIONAL);


    stack = new Gtk::Stack();
    stack->set_transition_duration(200);
    stack->set_transition_type(Gtk::STACK_TRANSITION_TYPE_CROSSFADE);
    mainBox->pack_start(*stack,Gtk::PACK_EXPAND_WIDGET,0);

    this->add(*mainBox);
    
    fontsListScrollWidget = new Gtk::ScrolledWindow();
    fontsListScrollWidget->get_vadjustment()->signal_value_changed().connect(sigc::mem_fun(*this,&MainWindow::fontListScroll));
    stack->add(*fontsListScrollWidget,"list");
    fontsListWidget = new Gtk::Box();
    fontsListWidget->set_orientation(Gtk::ORIENTATION_VERTICAL);
    fontsListScrollWidget->add(*fontsListWidget);

    int fontIndex = 0;
    for (auto a : *fontFamilies) {
        FontListItem* fontListItem = new FontListItem();
        fontListItem->fontFamily = a;
        fontListItem->hasBeenViewed = false;

        Gtk::Button* btn = new Gtk::Button();
        fontListItem->button = btn;

        btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this,&MainWindow::switchToFontFamily),fontIndex));

        if (a->paths->size() > 1) {
            auto tooltip = Glib::ustring::compose(_("%1 different files"),std::to_string(a->paths->size()));
            if (a->isModifiedGrouping) {
                tooltip = tooltip + "\n" + _("Styles are combined into one single font.");
            }
            btn->set_tooltip_text(tooltip);
        } else {
            btn->set_tooltip_text(a->paths->at(0));
        }

        btn->set_relief(Gtk::RELIEF_NONE);

        Gtk::VBox* btnBox = new Gtk::VBox();
        btnBox->set_margin_left(8);
        btnBox->set_margin_right(8);
        btnBox->set_spacing(8);

        Gtk::HBox* btnHeaderBox = new Gtk::HBox();
        btnHeaderBox->set_spacing(4);

        Gtk::Label* btnLabel = new Gtk::Label();
        btnLabel->set_alignment(Gtk::ALIGN_START);
        btnLabel->set_text(a->family);
        btnHeaderBox->pack_start(*btnLabel,Gtk::PACK_SHRINK,0);
        
        if (a->styles->size() > 1) {
            Gtk::Label* btnStyleCount = new Gtk::Label();
            btnStyleCount->set_sensitive(false);
            btnStyleCount->set_alignment(Gtk::ALIGN_START);
            btnStyleCount->set_text(Glib::ustring::compose(_("%1 styles"),std::to_string(a->styles->size())));
            btnHeaderBox->pack_start(*btnStyleCount,Gtk::PACK_SHRINK,0);
        }

        btnBox->add(*btnHeaderBox);

        Gtk::Label* lblPreview = new Gtk::Label();
        fontListItem->preview = lblPreview;
        lblPreview->set_alignment(Gtk::ALIGN_START);
        lblPreview->set_text(getPreviewTextForLanguage(""));
        btnBox->add(*lblPreview);

        btn->add(*btnBox);

        fontsListWidget->add(*btn);
        fontListItems->push_back(fontListItem);
        fontIndex += 1;
    }
    Glib::signal_idle().connect(sigc::mem_fun(*this, &MainWindow::queuedfontListScrollCallback));
    fontViewWidget = new Gtk::HBox();
    fontFamilyScrollWidget = new Gtk::ScrolledWindow();
    fontFamilyScrollWidget->get_style_context()->add_class("font-family-window");
    fontViewWidget->add(*fontFamilyScrollWidget);
    fontStyleScrollWidget = new Gtk::ScrolledWindow();
    fontStyleScrollWidget->get_style_context()->add_class("font-style-window");
    fontViewWidget->add(*fontStyleScrollWidget);

    fontFamilyBoxWidget = new Gtk::Box();
    
    fontFamilyBoxWidget->set_orientation(Gtk::ORIENTATION_VERTICAL);
    fontFamilyBoxWidget->get_style_context()->add_class("font-family-box");
    fontFamilyBoxWidget->set_halign(Gtk::ALIGN_FILL);
    fontFamilyBoxWidget->set_valign(Gtk::ALIGN_START);
    fontFamilyLabelWidget = new Gtk::Label();
    fontFamilyLabelWidget->get_style_context()->add_class("font-family-label");
    fontFamilyLabelWidget->set_halign(Gtk::ALIGN_START);
    fontFamilyLabelWidget->set_margin_bottom(8);
    fontFamilyBoxWidget->pack_start(*fontFamilyLabelWidget,Gtk::PACK_SHRINK,0);

    fontFamilyEntryWidget = new Gtk::Entry();
    fontFamilyEntryWidget->set_placeholder_text(_("Enter text to preview in the font"));
    fontFamilyEntryWidget->set_margin_bottom(8);
    fontFamilyEntryWidget->signal_changed().connect(sigc::mem_fun(*this,&MainWindow::fontPreviewTextChanged));
    fontFamilyBoxWidget->pack_start(*fontFamilyEntryWidget,Gtk::PACK_EXPAND_WIDGET,0);

    fontFamilyScrollWidget->add(*fontFamilyBoxWidget);

    fontStyleRowsWidget = new Gtk::Box(Gtk::ORIENTATION_VERTICAL);
    
    fontStyleScrollWidget->add(*fontStyleRowsWidget);



    stack->add(*fontViewWidget,"view");
    this->show_all();
    backButton->hide();
    installButton->hide();
    if (defaultFileName != NULL) {
        stack->set_transition_duration(0);
        switchToFontFile(*defaultFileName);
        stack->set_transition_duration(200);
    }
}

void MainWindow::switchToFontFamily(int fontIndex) {
    currentFontIndex = fontIndex;
    currentFontPath = NULL;
    familyData = fontFamilies->at(fontIndex);
    loadFont();
}

void MainWindow::switchToFontFile(std::string fontPath) {
    currentFontIndex = -1;
    currentFontPath = new std::string(fontPath);
    std::vector<FontFamilyData*>* loadedFonts = new std::vector<FontFamilyData*>();
    int number = 0;
    FcFontSet* fontSet = FcFontSetCreate();
    FcFreeTypeQueryAll((const FcChar8*)currentFontPath->c_str(),-1,NULL,&number,fontSet);
    loadFontFamiliesFromSet(fontSet,loadedFonts,true);
    if (loadedFonts->empty()) {
        std::cout << "File doesn't contain any fonts" << std::endl;
        return;
    }
    familyData = loadedFonts->at(0);
    if (loadedFonts->size() > 1) {
        std::cout << "File contains multiple font families, selecting the first one" << std::endl;
        return;
    }
    loadFont();
}

bool familyContainsStyle(FontFamilyData* family, FontStyleData* style) {
    for (FontStyleData* familyStyle : *family->styles) {
        if (familyStyle->weight == style->weight && familyStyle->slant == style->slant) {
            return true;
        }
    }

    return false;
}

void MainWindow::loadFont() {
    stack->set_visible_child("view");
    backButton->show();
    searchBar->hide();
    searchButton->hide();
    googleFontsButton->hide();

    bool isInstalled = false;

    for (FontFamilyData* fontFamilyData : *fontFamilies) {
        if (fontFamilyData->family == familyData->family) {
            isInstalled = true;
            
            for (FontStyleData* fontStyle : *familyData->styles) {
                if (!familyContainsStyle(fontFamilyData, fontStyle)) {
                    isInstalled = false;
                    break;
                }
            }
            break;
        }
    }

    if (isInstalled) {
        installButton->set_label(_("Installed"));
        installButton->set_sensitive(false);
        installButton->get_style_context()->remove_class(GTK_STYLE_CLASS_SUGGESTED_ACTION);
    } else {
        installButton->set_label(_("Install"));
        installButton->set_sensitive(true);
        installButton->get_style_context()->add_class(GTK_STYLE_CLASS_SUGGESTED_ACTION);
    }



    installButton->show_all();
    headerBarCustomText->set_text(familyData->family);
    headerBar->set_custom_title(*headerBarCustomText);
    headerBarCustomText->show();
    fontFamilyLabelWidget->set_text(familyData->family);
    for (FontStyleListItem* fontStyleListItem : *fontStyleListItems) {
        fontFamilyBoxWidget->remove(*fontStyleListItem->button);
        delete fontStyleListItem->fontWidgetMM;
        delete fontStyleListItem->button;
        delete fontStyleListItem;
    }
    for (FontStyleRow* row : *fontStyleRows) {
        fontStyleRowsWidget->remove(*row->lblName);
        fontStyleRowsWidget->remove(*row->lblValue);
    }
    fontStyleListItems->clear();
    fontFamilyEntryWidget->set_text("");
    fontStyleRows->clear();
    int styleIndex = 0;
    for (std::string path : *familyData->paths) {
        this->addInfoText("Path",path);
    }
    for (FontStyleData* style : *familyData->styles) {
        FontStyleListItem* fontStyleListItem = new FontStyleListItem();

        Gtk::Button* btn = new Gtk::Button();
        fontStyleListItem->button = btn;

        btn->set_tooltip_text(style->path);

        btn->get_style_context()->add_class("font-style-button");
        btn->set_relief(Gtk::RELIEF_NONE);

        Gtk::VBox* btnBox = new Gtk::VBox();
        btnBox->set_spacing(8);

        Gtk::HBox* btnHeaderBox = new Gtk::HBox();
        btnHeaderBox->set_spacing(4);

        Gtk::Label* lblName = new Gtk::Label();
        lblName->set_alignment(Gtk::ALIGN_START);
        lblName->set_text(_(style->name.c_str()));
        btnHeaderBox->pack_start(*lblName,Gtk::PACK_SHRINK,0);
        
        Gtk::Label* lblWeight = new Gtk::Label();
        lblWeight->set_sensitive(false);
        lblWeight->set_alignment(Gtk::ALIGN_START);
        lblWeight->set_text(std::to_string(style->weight));
        btnHeaderBox->pack_start(*lblWeight,Gtk::PACK_SHRINK,0);

        btnBox->add(*btnHeaderBox);

        GFile* file = g_file_new_for_path(style->path.c_str());
        char* fileURI = g_file_get_uri(file);
        SushiFontWidget* fontWidget = sushi_font_widget_new(fileURI,style->index);
        FontWidgetLoadData* loadData = new FontWidgetLoadData();
        loadData->self = this;
        loadData->fontWidget = fontWidget;
        g_signal_connect(fontWidget,"loaded",G_CALLBACK(MainWindow_fontWidgetLoaded),loadData);
        sushi_font_widget_set_text(fontWidget,(gchar*)getPreviewTextForLanguage(""));
        
        Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontWidget));
        btnBox->add(*fontWidgetMM);

        fontStyleListItem->fontWidget = fontWidget;
        fontStyleListItem->fontWidgetMM = fontWidgetMM;

        btn->add(*btnBox);

        btn->show_all();
        fontFamilyBoxWidget->pack_start(*btn,Gtk::PACK_EXPAND_WIDGET,0);

        fontStyleListItems->push_back(fontStyleListItem);

        if (styleIndex == 0) btn->grab_focus(); // don't place focus on textbox automatically
        styleIndex += 1;
    }
}

void MainWindow::addInfoText(std::string name,std::string value) {
    if (name != "Path") {
        for (FontStyleRow* existingRow : *fontStyleRows) {
            if (existingRow->name == name) {
                return;
            }
        }
    }
    FontStyleRow* row = new FontStyleRow();
    row->name = name;
    row->lblName = new Gtk::Label(_(name.c_str()));
    row->lblValue = new Gtk::Label();

    std::string valueText = "";
    int overflowIndex = 0;
    for (int i = 0; i < (int)value.length(); i++) {
        char c = value.at(i);
        if (c == '\n') overflowIndex = 0; else overflowIndex += 1;
        valueText = valueText + c;
        if (overflowIndex > 70) {
            overflowIndex = 0;
            valueText = valueText + "\n";
        }
    }
    row->lblValue->set_text(valueText);
    
    row->lblName->set_margin_left(8);
    row->lblName->set_margin_right(8);
    row->lblValue->set_margin_left(8);
    row->lblValue->set_margin_right(8);

    row->lblName->set_margin_top(8);
    row->lblName->get_style_context()->add_class("font-info-name");

    row->lblName->set_halign(Gtk::ALIGN_START);
    row->lblValue->set_halign(Gtk::ALIGN_START);
    row->lblValue->set_selectable(true);

    

    fontStyleRowsWidget->add(*row->lblName);
    fontStyleRowsWidget->add(*row->lblValue);

    row->lblName->show_all();
    row->lblValue->show_all();
    
    fontStyleRows->push_back(row);
}

void MainWindow_fontWidgetLoaded(SushiFontWidget* fontWidget, FontWidgetLoadData* data) {
    FT_Face face = sushi_font_widget_get_ft_face(fontWidget);
    if (face == NULL) {
        return;
    }
    int len = FT_Get_Sfnt_Name_Count(face);
    for (int i = 0; i < len; i++) {
        FT_SfntName sinfo;
        if (FT_Get_Sfnt_Name(face,i,&sinfo) != 0) continue;
        if (sinfo.encoding_id != TT_MS_ID_UNICODE_CS) continue;
        std::string contents = g_convert((gchar*)sinfo.string, sinfo.string_len, "UTF-8", "UTF-16BE", NULL, NULL, NULL);
        if (sinfo.name_id == TT_NAME_ID_COPYRIGHT) {
            data->self->addInfoText("Copyright",contents);
        } else if (sinfo.name_id == TT_NAME_ID_VERSION_STRING) {
            data->self->addInfoText("Version",contents);
        } else if (sinfo.name_id == TT_NAME_ID_DESCRIPTION) {
            data->self->addInfoText("Description",contents);
        } else if (sinfo.name_id == TT_NAME_ID_DESIGNER) {
            data->self->addInfoText("Designer",contents);
        } else if (sinfo.name_id == TT_NAME_ID_MANUFACTURER) {
            data->self->addInfoText("Manufacturer",contents);
        } else if (sinfo.name_id == TT_NAME_ID_LICENSE) {
            data->self->addInfoText("License",contents);
        } else if (sinfo.name_id == TT_NAME_ID_TRADEMARK) {
            data->self->addInfoText("Trademark",contents);
        }
    }

}

void MainWindow::fontPreviewTextChanged() {
    std::string newPreviewText = fontFamilyEntryWidget->get_text();
    if (currentPreviewText != NULL) {
        free(currentPreviewText);
    }
    if (newPreviewText.empty()) {
        currentPreviewText = NULL;
    } else {
        currentPreviewText = new std::string(newPreviewText);
    }
    for (FontStyleListItem* fontStyleListItem : *fontStyleListItems) {
        if (currentPreviewText == NULL) {
            sushi_font_widget_set_text(fontStyleListItem->fontWidget,(gchar*)getPreviewTextForLanguage(""));
        } else {
            sushi_font_widget_set_text(fontStyleListItem->fontWidget,(gchar*)currentPreviewText->c_str());
        }
        Gtk::Widget* fontWidgetMM = Glib::wrap(GTK_WIDGET(fontStyleListItem->fontWidget));
        fontWidgetMM->queue_draw();
    }
}

void MainWindow::switchToFontList() {
    currentFontIndex = -1;
    currentFontPath = NULL;
    stack->set_visible_child("list");
    backButton->hide();
    installButton->hide();
    searchBar->show_all();
    searchButton->show_all();
    googleFontsButton->show_all();
    gtk_header_bar_set_custom_title(headerBar->gobj(),nullptr);
}

bool MainWindow::queuedfontListScrollCallback() {
    fontListScroll();
    return false;
}

void MainWindow::fontListScroll() {
    double scrollPosition = fontsListScrollWidget->get_vadjustment()->get_value();
    for (FontListItem* listItem : *fontListItems) {
        if (listItem->hasBeenViewed) continue;
        if (!listItem->button->get_visible()) continue;
        Gtk::Allocation allocation = listItem->button->get_allocation();
        int y = allocation.get_y();
        int windowHeight = this->get_height(); // good enough approximation
        if (y > scrollPosition - 256 && y < scrollPosition + windowHeight + 256) {
            listItem->hasBeenViewed = true;
            Pango::FontDescription* fontDescription = new Pango::FontDescription();
            fontDescription->set_family(listItem->fontFamily->family);
            fontDescription->set_size(20 * PANGO_SCALE);
            Pango::AttrList* fontAttributes = new Pango::AttrList();
            Pango::AttrFontDesc attrFontDesc = Pango::AttrFontDesc::create_attr_font_desc(*fontDescription);
            fontAttributes->insert(attrFontDesc);
            listItem->preview->set_attributes(*fontAttributes);
        }
    }
}

void MainWindow::installFontClicked() {
    this->installButton->set_sensitive(false);
    this->installButton->set_label(_("Installing..."));
    GCancellable* cancellable = g_cancellable_new();
    GTask* task = g_task_new(this->gobj(),cancellable,MainWindow_installFontFinished,this);
    g_task_set_task_data(task,this,NULL);
    g_task_run_in_thread(task,MainWindow_installFontTask);
}

void MainWindow_installFontTask(GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable) {
    MainWindow* self = (MainWindow*)task_data;

    std::string userDataDir = Glib::get_user_data_dir();
    std::string fontDirPath = userDataDir + "/fonts";

    auto fontDir = Gio::File::create_for_path(fontDirPath);

    if (!fontDir->query_exists()) {
        try {
            fontDir->make_directory_with_parents();
        } catch (Glib::Error& error) {
            g_task_return_error(task, g_error_copy(error.gobj()));
            return;
        }
    }

    auto originalFile = Gio::File::create_for_path(*self->currentFontPath);
    std::string basename = originalFile->get_basename();
    auto finalFile = fontDir->get_child(basename);

    if (finalFile->query_exists()) {
        g_task_return_boolean(task,false);
        return;
    }

    try {
        originalFile->copy(finalFile, Gio::File::SlotFileProgress(), Glib::wrap(cancellable), Gio::FILE_COPY_NONE);
    } catch (Glib::Error& error) {
        g_task_return_error(task, g_error_copy(error.gobj()));
        return;
    }

    g_task_return_boolean(task,true);
}

void MainWindow_installFontFinished(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    MainWindow* self = (MainWindow*)user_data;
    GError* error = NULL;
    bool returnBool = g_task_propagate_boolean(G_TASK(res),&error);
    if (returnBool == false || error != NULL) {
        std::string* errorMessage = NULL;
        if (error != NULL) errorMessage = new std::string(error->message);
        else errorMessage = new std::string(_("Unknown error occured"));
        
        std::cout << "Error installing font: " << *errorMessage << std::endl;
        Gtk::MessageDialog* dialog = new Gtk::MessageDialog(*self,_("Error installing font"),false,Gtk::MESSAGE_ERROR,Gtk::BUTTONS_OK,true);
        dialog->set_secondary_text(*errorMessage);
        dialog->show_all();
        dialog->signal_response().connect_notify([dialog](int response){delete dialog;});
        self->loadFont();
        return;
    }

    bool addedToExisting = false;
    for (FontFamilyData* family : *self->fontFamilies) {
        if (family->family == self->familyData->family) {
            addedToExisting = true;
            for (FontStyleData* style : *self->familyData->styles) {
                if (familyContainsStyle(family, style)) continue;

                family->styles->push_back(style);
            }
            break;
        }
    }

    if (!addedToExisting) {
        self->fontFamilies->push_back(self->familyData);
    }

    self->loadFont();
}

bool MainWindow::windowKeyPressEvent(GdkEventKey* event) {
    if (event->keyval == GDK_KEY_f && event->state & GDK_CONTROL_MASK) {
        this->searchBar->set_search_mode(!this->searchBar->get_search_mode());
    }
    return this->searchBar->handle_event(event);
}

void MainWindow::searchUpdated() {
    std::string input = this->searchEntry->get_text();
    std::transform(input.begin(), input.end(), input.begin(), [](unsigned char c){ return std::tolower(c); });
    for (FontListItem* listItem : *fontListItems) {
        std::string family = listItem->fontFamily->family;
        std::transform(family.begin(), family.end(), family.begin(), [](unsigned char c){ return std::tolower(c); });
        if (family.find(input) != std::string::npos) {
            listItem->button->show();
        } else {
            listItem->button->hide();
        }
    }
    Glib::signal_idle().connect(sigc::mem_fun(*this, &MainWindow::queuedfontListScrollCallback));
}

void MainWindow::openGoogleFonts() {
    if (googleFontsWindow != NULL) {
        googleFontsWindow->present();
        return;
    }

    googleFontsWindow = new GoogleFontsWindow(::loadFonts(false));
}

MainWindow::~MainWindow() {
    
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, DATADIR "/locale");
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    bool entryGoogleFontsValue = false;

    Glib::OptionEntry entryGoogleFonts;
    entryGoogleFonts.set_long_name("google-fonts");
    entryGoogleFonts.set_description("Open the Google Fonts window instead of the system fonts");

    Glib::OptionGroup optionGroup("options", "Options");
    optionGroup.add_entry(entryGoogleFonts, entryGoogleFontsValue);
    
    Glib::OptionContext optionContext("[Filename of font]");
    optionContext.set_main_group(optionGroup);

    try {
        optionContext.parse(argc, argv);
    } catch (const Glib::OptionError& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    Gtk::Main* app = new Gtk::Main();
    FcInit();
    std::string* defaultFileName = NULL;
    if (argc > 1) {
        defaultFileName = new std::string(argv[1]);
    }


    Gtk::Window* win = NULL;
    if (entryGoogleFontsValue) {
        auto fonts = loadFonts(false);
        win = new GoogleFontsWindow(fonts);
    } else {
        auto fonts = loadFonts(true);
        win = new MainWindow(fonts, defaultFileName);
    }
    app->run(*win);
    return 0;
}

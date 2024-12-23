#include <gtkmm.h>
#include "utils.hpp"

void setFontSizeOfWidget(Gtk::Widget *widget, double size) {
    auto context = widget->get_pango_context();
    auto description = context->get_font_description();
    description.set_size(size * Pango::SCALE);
    context->set_font_description(description);
}

const int fc_weight_to_weight(int weight) {
    switch (weight) {
    case FC_WEIGHT_THIN:
        return PANGO_WEIGHT_THIN;
    case FC_WEIGHT_EXTRALIGHT:
        return PANGO_WEIGHT_ULTRALIGHT;
    case FC_WEIGHT_LIGHT:
        return PANGO_WEIGHT_LIGHT;
    case FC_WEIGHT_SEMILIGHT:
        return PANGO_WEIGHT_SEMILIGHT;
    case FC_WEIGHT_BOOK:
        return PANGO_WEIGHT_BOOK;
    case FC_WEIGHT_REGULAR:
        return PANGO_WEIGHT_NORMAL;
    case FC_WEIGHT_MEDIUM:
        return PANGO_WEIGHT_MEDIUM;
    case FC_WEIGHT_SEMIBOLD:
        return PANGO_WEIGHT_SEMIBOLD;
    case FC_WEIGHT_BOLD:
        return PANGO_WEIGHT_BOLD;
    case FC_WEIGHT_EXTRABOLD:
        return PANGO_WEIGHT_ULTRABOLD;
    case FC_WEIGHT_HEAVY:
        return PANGO_WEIGHT_HEAVY;
    case FC_WEIGHT_EXTRABLACK:
        return PANGO_WEIGHT_ULTRAHEAVY;
    }

    return PANGO_WEIGHT_NORMAL;
}

const int fc_slant_to_slant(int slant) {
    switch (slant) {
    case FC_SLANT_ROMAN:
        return PANGO_STYLE_NORMAL;
    case FC_SLANT_OBLIQUE:
        return PANGO_STYLE_OBLIQUE;
    case FC_SLANT_ITALIC:
        return PANGO_STYLE_ITALIC;
    }

    return PANGO_STYLE_NORMAL;
}

const char* weight_to_name(int weight)
{
    switch (weight) {
    case PANGO_WEIGHT_THIN:
        return "Thin";
    case PANGO_WEIGHT_ULTRALIGHT:
        return "Extralight";
    case PANGO_WEIGHT_LIGHT:
        return "Light";
    case PANGO_WEIGHT_SEMILIGHT:
        return "Semilight";
    case PANGO_WEIGHT_BOOK:
        return "Book";
    case PANGO_WEIGHT_NORMAL:
        return "Regular";
    case PANGO_WEIGHT_MEDIUM:
        return "Medium";
    case PANGO_WEIGHT_SEMIBOLD:
        return "Semibold";
    case PANGO_WEIGHT_BOLD:
        return "Bold";
    case PANGO_WEIGHT_ULTRABOLD:
        return "Extrabold";
    case PANGO_WEIGHT_HEAVY:
        return "Heavy";
    case PANGO_WEIGHT_ULTRAHEAVY:
        return "Extrablack";
    }

    return "Unknown";
}

const char* slant_to_name(int slant)
{
    switch (slant) {
    case PANGO_STYLE_NORMAL:
        return "";
    case PANGO_STYLE_OBLIQUE:
        return "Oblique";
    case PANGO_STYLE_ITALIC:
        return "Italic";
    }

    return "";
}

size_t curlWriteCallbackString(void* ptr, size_t size, size_t nmemb, std::string *data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

void replaceAllInString(std::string *str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str->find(from, start_pos)) != std::string::npos) {
        str->replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}
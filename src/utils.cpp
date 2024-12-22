#include <gtkmm.h>
#include "utils.hpp"

void setFontSizeOfWidget(Gtk::Widget *widget, double size) {
    auto context = widget->get_pango_context();
    auto description = context->get_font_description();
    description.set_size(size * Pango::SCALE);
    context->set_font_description(description);
}
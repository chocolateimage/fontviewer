#pragma once

#include <gtkmm.h>

void setFontSizeOfWidget(Gtk::Widget *widget, double size);

const int fc_weight_to_weight(int weight);
const int fc_slant_to_slant(int slant);
const char* weight_to_name(int weight);
const char* slant_to_name(int slant);
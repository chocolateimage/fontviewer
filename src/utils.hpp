#pragma once

#include <gtkmm.h>

void setFontSizeOfLabel(Gtk::Label *label, double size);

const int fc_weight_to_weight(int weight);
const int fc_slant_to_slant(int slant);
const char* weight_to_name(int weight);
const char* slant_to_name(int slant);

size_t curlWriteCallbackString(void* ptr, size_t size, size_t nmemb, std::string *data);

void replaceAllInString(std::string *str, const std::string& from, const std::string& to);
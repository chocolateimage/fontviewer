#include "google-fonts-window.hpp"
#include <json-glib/json-glib.h>
#include <iostream>
#include <giomm.h>

GoogleFontsWindow::GoogleFontsWindow() {
    this->resize(900, 500);
    this->set_title("Google Fonts");

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    auto file = Gio::File::create_for_uri("https://fonts.google.com/metadata/fonts");

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

    json_parser_load_from_data(parser, response.c_str(), response.length(), &error);

    if (error != NULL) {
        std::cout << "Unable to parse: " << error->message << "\n";
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    JsonObject* rootObject = json_node_get_object(root);
    JsonArray* familyMetadataList = json_object_get_array_member(rootObject, "familyMetadataList");

    int length = json_array_get_length(familyMetadataList);
    std::cout << length << " families" << "\n";
    for (int i = 0; i < length; i++) {
        JsonObject* fontMetadata = json_array_get_object_element(familyMetadataList, i);
        const gchar *family = json_object_get_string_member(fontMetadata, "family");
        std::cout << family << "\n";
    }
}
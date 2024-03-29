/**
 * Copyright 2023 Simon Edwards <simon@simonzone.com>
 *
 * This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
 */
#include <stdlib.h>

const char *EXTRATERM_INTRO = "\x1b&";
/* const char *EXTRATERM_INTRO = ""; */

char *get_extratern_cookie() {
    return getenv("LC_EXTRATERM_COOKIE");
}

bool is_extraterm() {
    return get_extratern_cookie() != NULL;
}

void extraterm_start_file_transfer(const char* mimetype, const char* charset, const char* filename, size_t filesize,
        bool downloadFlag) {

    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    if (mimetype != NULL) {
        json_object_set_string(root_object, "mimeType", mimetype);
    }
    if (filename != NULL) {
        json_object_set_string(root_object, "filename", filename);
    }
    if (charset != NULL) {
        json_object_set_string(root_object, "charset", charset);
    }
    if (filesize != -1) {
        json_object_set_number(root_object, "filesize", filesize);
    }

    if (downloadFlag) {
        json_object_set_string(root_object, "download", "true");
    }

    serialized_string = json_serialize_to_string(root_value);

    fputs(EXTRATERM_INTRO, stdout);
    fputs(get_extratern_cookie(), stdout);
    fputs(";5;", stdout);
    printf("%li", strlen(serialized_string));
    fputs("\x07", stdout);
    fputs(serialized_string, stdout);

    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
}

void extraterm_end_file_transfer() {
    fputc(0, stdout);
}

void extraterm_client_request_frame(const char *frame_name) {
    fputs(EXTRATERM_INTRO, stderr);
    fputs(get_extratern_cookie(), stderr);
    fputs(";4\x07", stderr);
    fputs(frame_name, stderr);
    fputc(0, stderr);
    fflush(stderr);
}
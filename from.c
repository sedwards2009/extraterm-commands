/**
 * Copyright 2023 Simon Edwards <simon@simonzone.com>
 *
 * This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "arena.h"

#include "libs/adopt.h"
#include "libs/adopt.c"
#include "libs/parson.c"
#include "libs/base64.c"
#include "libs/sha256.c"

#include "tty_utils.c"
#include "utils.c"
#include "extraterm_client.c"


bool read_stdin_line(char *buffer, size_t buffer_size) {
    bool ok = fgets(buffer, buffer_size, stdin) != NULL;
    string_strip(buffer);
    return ok;
}

Arena *request_frame_arena = NULL;

void *request_frame_alloc(size_t size) {
    return arena_alloc(request_frame_arena, size);
}

void request_frame_free(void *) {
}

bool request_frame(Arena *arena, const char *frame_name, FILE *fhandle, JSON_Value **metadata) {
    const size_t HASH_LENGTH = 20;
    request_frame_arena = arena;

    turn_off_echo();

    extraterm_client_request_frame(frame_name);

    const int LINE_LENGTH = 1024;
    const int COMMAND_PREFIX_LENGTH = 3;

    char line[LINE_LENGTH];
    char contents[LINE_LENGTH];

    read_stdin_line(line, LINE_LENGTH);

    if (!string_starts_with(line, "#M:")) {
        fputs("[Error] When reading in frame data, expected '#M:...', but didn't receive it.\n", stderr);
        fflush(stderr);
        return false;
    }

    if (strlen(line) < COMMAND_PREFIX_LENGTH + 1 + HASH_LENGTH) {
        fputs("[Error] When reading in metadata line was too short.\n", stderr);
        fflush(stderr);
        return false;
    }

    char line_hash[HASH_LENGTH + 1];
    line_hash[HASH_LENGTH] = '\0';
    strncpy(line_hash, line + (strlen(line) - HASH_LENGTH), HASH_LENGTH);

    size_t b64data_length = strlen(line) - COMMAND_PREFIX_LENGTH - HASH_LENGTH -1;
    size_t contents_length = b64_decode(line + COMMAND_PREFIX_LENGTH, b64data_length, contents);

    sha256_context hash;
    sha256_init(&hash);
    sha256_hash(&hash, contents, contents_length);

    unsigned char previous_hash[SHA256_SIZE_BYTES];
    sha256_done(&hash, previous_hash);

    char hash_hex[SHA256_SIZE_BYTES * 2 + 1];
    sha256_hash_to_hex(previous_hash, hash_hex);
    hash_hex[HASH_LENGTH] = '\0';

    // Check the hash
    convert_to_lowercase(line_hash);
    if (strcmp(line_hash, hash_hex) != 0) {
        fputs("[Error] Hash didn't match for metadata line. Expected '", stderr);
        fputs(line_hash, stderr);
        fputs("' got '", stderr);
        fputs(hash_hex, stderr);
        fputs("'\n", stderr);
        fflush(stderr);
        return false;
    }

    json_set_allocation_functions(request_frame_alloc, request_frame_free);
    *metadata = json_parse_string(contents);

    while (true) {
        read_stdin_line(line, LINE_LENGTH);
        if (strlen(line) < COMMAND_PREFIX_LENGTH + 1 + HASH_LENGTH) {
            fputs("[Error] When reading frame body data a line is too short.", stderr);
            fflush(stderr);
            return false;
        }

        if (string_starts_with(line, "#D:") || string_starts_with(line, "#E:") || string_starts_with(line, "#A:")) {
            size_t b64data_length = strlen(line) - COMMAND_PREFIX_LENGTH - HASH_LENGTH -1;
            size_t contents_length = b64_decode(line + COMMAND_PREFIX_LENGTH, b64data_length, contents);

            strncpy(line_hash, line + (strlen(line) - HASH_LENGTH), HASH_LENGTH);
            convert_to_lowercase(line_hash);

            sha256_init(&hash);
            sha256_hash(&hash, previous_hash, SHA256_SIZE_BYTES);
            sha256_hash(&hash, contents, contents_length);
            sha256_done(&hash, previous_hash);

            // Check the hash
            sha256_hash_to_hex(previous_hash, hash_hex);
            hash_hex[HASH_LENGTH] = '\0';
            if (strcmp(line_hash, hash_hex) != 0) {
                fputs("[Error] Upload failed. (Hash didn't match for data line. Expected ", stderr);
                fputs(hash_hex, stderr);
                fputs(" got ", stderr);
                fputs(line_hash, stderr);
                fputs(")\n", stderr);
                fflush(stderr);
                return false;
            }

            if (string_starts_with(line, "#E:")) {
                // EOF
                break;
            }

            if (string_starts_with(line, "#A:")) {
                fputs("Upload aborted\n", stderr);
                fflush(stderr);
                return false;
            }

            // Send the input to stdout.
            fwrite(contents, sizeof(char), contents_length, fhandle);

        } else {
            fputs("[Error] When reading frame body data, line didn't start with '#D:' or '#E:'.", stderr);
            fflush(stderr);
        }
    }
    return true;
}

char *write_frame_to_disk(Arena *arena, const char *frame_name) {
    JSON_Value *metadata = NULL;
    char *tmp_filename = NULL;
    char *final_filename = NULL;
    FILE *tmp_fhandle = NULL;
    char *filename = NULL;
    int tmp_fd = -1;

    tmp_filename = arena_strdup(arena, "extraterm_from_XXXXXX");
    tmp_fd = mkstemp(tmp_filename);
    if (tmp_fd == -1) {
        fputs("[Error] Unable to open temp file.\n", stderr);
        goto clean_up;
    }

    tmp_fhandle = fdopen(tmp_fd, "wb");
    if (!tmp_fhandle) {
        perror("[Error] Unable to open temp file.");
        goto clean_up;
    }


    if (!request_frame(arena, frame_name, tmp_fhandle, &metadata)) {
        goto clean_up;
    }

    if (json_type(metadata) != JSONObject) {

    }

    JSON_Object *metadata_object = json_value_get_object(metadata);
    const char *json_filename = json_object_get_string(metadata_object, "filename");
    if (json_filename != NULL) {
        filename = arena_strdup(arena, json_filename);
    }
    if (filename == NULL) {
        const char *mimetype = json_object_get_string(metadata_object, "mimeType");
        if (mimetype != NULL) {
            filename = arena_strdup(arena, mimetype);
            replace_char(filename, '/', '-');
        } else {
            goto clean_up;
        }
    }

    if (access(filename, F_OK) == 0) {
        rename(tmp_filename, filename);
    } else {
        filename = find_suitable_filename(filename);
        if (filename != NULL) {
            rename(tmp_filename, filename);
        }
    }

clean_up:
    if (tmp_fhandle) {
        fclose(tmp_fhandle);
    }
    if (tmp_fd != -1) {
        close(tmp_fd);
    }
    unlink(tmp_filename);
    return filename;
}

bool output_frame(Arena *arena, const char *frame_name) {
    JSON_Value *metadata = NULL;
    return request_frame(arena, frame_name, stdout, &metadata);
}

void show_version() {
}

int main(int argc, char *argv[]) {
    char **frames_array = NULL;
    char *xargs = NULL;
    int help_flag = 0;
    int save_flag = 0;
    int version_flag = 0;

    adopt_spec opt_specs[] = {
        { .type=ADOPT_TYPE_SWITCH, .name="help", .alias='h', .value=&help_flag, .switch_value=1, .help="show this help message and exit" },
        { .type=ADOPT_TYPE_SWITCH, .name="version", .alias='v', .value=&version_flag, .switch_value=1 },
        { .type=ADOPT_TYPE_SWITCH, .name="save", .alias='s', .value=&save_flag, .switch_value=1 },
        { .type=ADOPT_TYPE_LITERAL },
        { .type=ADOPT_TYPE_ARGS, .value=&frames_array, .value_name="frames", .help="Frame IDs" },
    };

    adopt_opt result;
    if (adopt_parse(&result, opt_specs, argv + 1, argc - 1, ADOPT_PARSE_DEFAULT) != 0) {
        adopt_status_fprint(stderr, argv[0], &result);
        adopt_usage_fprint(stderr, argv[0], opt_specs);
        return EXIT_FAILURE;
    }

    if (help_flag) {
        adopt_usage_fprint(stderr, argv[0], opt_specs);
        return EXIT_SUCCESS;
    }
    if (version_flag) {
        show_version();
        return EXIT_SUCCESS;
    }

    if (!is_extraterm()) {
        fprintf(stderr, "[Error] Sorry, you're not using Extraterm as your terminal.\n");
        return EXIT_FAILURE;
    }

    // make sure that stdin is a tty
    if (!isatty(fileno(stdin))) {
        fprintf(stderr, "[Error] 'from' command must be connected to tty on stdin.\n");
        return EXIT_FAILURE;
    }

    if (frames_array != NULL) {
        // Normal execution. Output the frames
        for (int i=0; i<result.args_len; i++) {
            int rc;
            const char *frame_name = frames_array[i];
            if (save_flag) {
                Arena arena = {0};
                char *filename = write_frame_to_disk(&arena, frame_name);
                if (filename != NULL) {
                    printf("Wrote %s\n", filename);
                } else {
                    rc = EXIT_FAILURE;
                }
                arena_free(&arena);

            } else {
                Arena arena = {0};
                rc = output_frame(&arena, frame_name) ? EXIT_SUCCESS : EXIT_FAILURE;
                arena_free(&arena);
            }

            if (rc != 0) {
                return rc;
            }
        }
    }
    return EXIT_SUCCESS;
}

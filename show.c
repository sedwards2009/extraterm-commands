/**
 *
 * This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
 */
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "libs/adopt.h"
#include "libs/adopt.c"
#include "libs/sha256.c"
#include "libs/base64.c"
#include "libs/parson.c"

#include "extraterm_client.c"
#include "tty_utils.c"
#include "utils.c"

/* This is kept a multiple of 3 to avoid padding in the base64 representation. */
const size_t MAX_CHUNK_BYTES = 3 * 1024;

int send_mimetype_data(FILE* fhandle, const char* filename, const char* mimetype, const char* charset,
                        size_t filesize, bool download_flag) {
    turn_off_echo();

    extraterm_start_file_transfer(mimetype, charset, filename, filesize, download_flag);
    unsigned char buffer[MAX_CHUNK_BYTES];

    size_t read_count;
    unsigned char b64buffer[b64d_size(MAX_CHUNK_BYTES)];

    sha256_context hash;

    bool is_previous_hash = false;
    unsigned char previous_hash[SHA256_SIZE_BYTES];
    while (true) {
        read_count = fread(buffer, 1, MAX_CHUNK_BYTES, fhandle);
        if (read_count == 0) {
            if (!feof(fhandle)) {
                 return EXIT_FAILURE;
            }
            break;
        }

        sha256_init(&hash);
        if (is_previous_hash) {
            sha256_hash(&hash, previous_hash, SHA256_SIZE_BYTES);
        }
        sha256_hash(&hash, buffer, read_count);

        fputs("D:", stdout);
        b64_encode(buffer, read_count, b64buffer);
        fputs(b64buffer, stdout);

        fputs(":", stdout);

        sha256_done(&hash, previous_hash);
        is_previous_hash = true;

        print_hex(previous_hash, SHA256_SIZE_BYTES);

        puts("");
    }

    fputs("E::", stdout);
    sha256_init(&hash);
    sha256_hash(&hash, previous_hash, SHA256_SIZE_BYTES);
    sha256_done(&hash, previous_hash);
    print_hex(previous_hash, SHA256_SIZE_BYTES);
    puts("");

    fflush(stdout);

    extraterm_end_file_transfer();
    return EXIT_SUCCESS;
}

int show_file(const char* filename, const char* mimetype, const char* charset, const char* filepath, bool download_flag) {
    FILE* fhandle = fopen(filepath, "rb");
    if (fhandle == NULL) {
        fprintf(stderr, "Unable to open file '%s'. %s\n", filepath, strerror(errno));
        return EXIT_FAILURE;
    }

    struct stat st;
    if (fstat(fileno(fhandle), &st) != 0) {
        perror("Error getting file size");
        return EXIT_FAILURE;
    }

    int result = send_mimetype_data(fhandle, filename ? filename : filepath, mimetype, charset, st.st_size, download_flag);

    fclose(fhandle);
    return result;
}

int show_stdin(const char* mimetype, const char* charset, const char* filename, bool download_flag) {
    return send_mimetype_data(stdin, filename, mimetype, charset, -1, download_flag);
}

void show_version() {

}

int main(int argc, char *argv[]) {
    char **filename_array = NULL;
    char *charset = NULL;
    char *mimetype = NULL;
    char *filename = NULL;
    int download_flag = 0;
    int help_flag = 0;
    int text_flag = 0;
    int version_flag = 0;

    adopt_spec opt_specs[] = {
        { .type=ADOPT_TYPE_SWITCH, .name="help", .alias='h', .value=&help_flag, .switch_value=1, .help="show this help message and exit" },
        { .type=ADOPT_TYPE_SWITCH, .name="version", .alias='v', .value=&version_flag, .switch_value=1 },
        { .type=ADOPT_TYPE_SWITCH, .name="download", .alias='d', .value=&download_flag, .switch_value=1 },
        { .type=ADOPT_TYPE_SWITCH, .name="text", .alias='t', .value=&text_flag, .switch_value=1, .help="treat the file as plain text" },
        { .type=ADOPT_TYPE_VALUE, .name="charset", .value=&charset, .help="the character set of the input file (default: UTF8)" },
        { .type=ADOPT_TYPE_VALUE, .name="mimetype", .value=&mimetype, .help="the mime-type of the input file (default: auto-detect)" },
        { .type=ADOPT_TYPE_VALUE, .name="filename", .value=&filename, .help="sets the file name in the metadata sent to the terminal (useful when reading from stdin)" },
        { .type=ADOPT_TYPE_LITERAL },
        { .type=ADOPT_TYPE_ARGS, .value=&filename_array, .value_name="filename", .help="Filenames to read from" },
        { 0 },
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

    if (text_flag) {
        mimetype = "text/plain";
    }

    if (filename_array) {
        for (int i = 0; i < result.args_len; i++) {
            int result = show_file(filename, mimetype, charset, filename_array[i], download_flag);
            if (result != EXIT_SUCCESS) {
                return result;
            }
        }
    } else {
        return show_stdin(mimetype, charset, filename, download_flag);
    }
    return EXIT_SUCCESS;
}

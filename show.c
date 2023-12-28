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

#include "libs/parg.c"
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
    unsigned char b64buffer[b64d_size(SHA256_SIZE_BYTES)];

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

void show_help() {
    printf("usage: show [-h] [--charset CHARSET] [-d] [--mimetype MIMETYPE] [--filename FILENAME] [-t] [file ...]\n"
        "\n"
        "Show a file inside Extraterm.\n"
        "\n"
        "positional arguments:\n"
        "file                 file name. The file data is read from stdin if no files are specified.\n"
        "\n"
        "options:\n"
        "-h, --help           show this help message and exit\n"
        "--charset CHARSET    the character set of the input file (default: UTF8)\n"
        "-d, --download       download the file and don't show it\n"
        "--mimetype MIMETYPE  the mime-type of the input file (default: auto-detect)\n"
        "--filename FILENAME  sets the file name in the metadata sent to the terminal (useful when reading from stdin).\n"
        "  -t, --text         Treat the file as plain text. \n");
}

void show_version() {

}

static const struct parg_option longopts[] = {
#define CHARSET_INDEX 0
    { "charset", PARG_REQARG, NULL, 0 },
#define DOWNLOAD_INDEX 1
    { "download", PARG_NOARG, NULL, 'd' },
#define MIMETYPE_INDEX 2
    { "mimetype", PARG_REQARG, NULL, 0 },
#define FILENAME_INDEX 3
    { "filename", PARG_REQARG, NULL, 0 },
#define TEXT_INDEX 4
    { "text", PARG_NOARG, NULL, 't' },
#define HELP_INDEX 5
    { "help", PARG_NOARG, NULL, 'h' },
    { 0, 0, 0, 0 }
};

int main(int argc, char *argv[]) {
    struct parg_state ps;
    parg_init(&ps);

    const char *file_options[argc];
    for (int i=0; i<argc; i++) {
        file_options[i] = NULL;
    }
    int file_count = 0;

    int c;
    const char *optstring = "dthv";
    int longindex = -1;

    bool text_flag = false;
    bool download_flag = false;
    const char *filename = NULL;
    const char *charset = NULL;
    const char *mimetype = NULL;

    int optend = parg_reorder(argc, argv, optstring, NULL);

    while ((c = parg_getopt_long(&ps, argc, argv, optstring, longopts, &longindex)) != -1) {
        switch (c) {

        case 'd':
            download_flag = true;
            break;

        case 't':
            text_flag = true;
            break;

        case 'h':
            show_help();
            return EXIT_SUCCESS;
            break;

        case 'v':
            show_version();
            return EXIT_SUCCESS;
            break;

        case 1:
            file_options[file_count] = ps.optarg;
            file_count++;
            break;

        case 0:
            // printf("flag option index %i, '%s'\n", longindex, ps.optarg);
            switch (longindex) {
            case CHARSET_INDEX:
                charset = ps.optarg;
                break;

            case MIMETYPE_INDEX:
                mimetype = ps.optarg;
                break;

            case FILENAME_INDEX:
                filename = ps.optarg;
                break;
            }
            longindex = -1;
            break;

        case '?':
            // printf("unknown option -%i\n", (int) ps.optopt);
            // printf("ps.optind %i\n", ps.optind);
            // printf("-> %s\n", argv[ps.optind-1]);
            // printf("longindex: %d\n", longindex);
            switch (longindex) {
                case MIMETYPE_INDEX:
                    fprintf(stderr, "option --mimetype requires a value\n");
                    break;

                case FILENAME_INDEX:
                    fprintf(stderr, "option --filename requires a value\n");
                    break;

                case CHARSET_INDEX:
                    fprintf(stderr, "option --charset requires a value\n");
                    break;

                default:
                    fprintf(stderr, "unknown option %s\n", argv[ps.optind-1]);
                    break;

            }
            return EXIT_FAILURE;
            break;

        default:
            fprintf(stderr, "error: unhandled option -%c'\n", c);
            return EXIT_FAILURE;
            break;
        }
    }

    if (!is_extraterm()) {
        fprintf(stderr, "Sorry, you're not using Extraterm as your terminal.\n");
        return EXIT_FAILURE;
    }

    if (text_flag) {
        mimetype = "text/plain";
    }

    if (file_count != 0) {
        for (int i=0; i<file_count; i++) {
            int result = show_file(filename, mimetype, charset, file_options[i], download_flag);
            if (result != EXIT_SUCCESS) {
                return result;
            }
        }
        return EXIT_SUCCESS;

    } else {
        return show_stdin(mimetype, charset, filename, download_flag);
    }
    return EXIT_SUCCESS;
}

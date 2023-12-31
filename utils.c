/**
 * Copyright 2023 Simon Edwards <simon@simonzone.com>
 *
 * This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#include "libs/sha256.h"
#include "arena.h"


void replace_char(char *str, char oldChar, char newChar) {
    size_t len = strlen(str);
    for (int i=0; i<len; i++) {
        if (str[i] == oldChar) {
            str[i] = newChar;
        }
    }
}

/**
 * Find the index of the file extension.
 *
 * @param str String holding the file name.
 *
 * @return the index of the start of the file extension at '.' char of -1
 * if there was no file extension.
 */
int file_extension_index(char *str) {
    size_t len = strlen(str);
    for (int i=len-1; i>=0; i--) {
        if (str[i] == '/') {
            return -1;
        }
        if (str[i] == '.') {
            if (i > 0 && str[i-1] != '/') {
              return i;
            }
        }
    }
    return -1;
}

char *find_suitable_filename(char *filename) {
  const int COUNTER_WIDTH = 3;

  if (access(filename, F_OK) != 0) {
      return strdup(filename);
  }

  int ext_index = file_extension_index(filename);

  char *tmp_filename = strdup(filename);
  char *template = NULL;
  char *result = NULL;
  if (ext_index != -1) {
      tmp_filename[ext_index] = '\0';
      template = malloc(strlen(filename) + strlen("(%u)") + 1);
      sprintf(template, "%s(%%u).%s", tmp_filename, tmp_filename + ext_index + 1);
      free(tmp_filename);
  } else {
      template = malloc(strlen(filename) + strlen("(%u)") + 1);
      sprintf(template, "%s(%%u)", filename);
  }
  result = malloc(strlen(filename) + 2 + COUNTER_WIDTH + 1);

  const unsigned int max_counter = pow(10, COUNTER_WIDTH);

  for (unsigned int i=1; i<max_counter; i++) {
      sprintf(result, template, i);
      if (access(result, F_OK) != 0) {
          free(template);
          return result;
      }
  }

  free(template);
  free(result);
  return NULL;
}

void string_strip(char *buffer) {
    int start_index = 0;
    int len = strlen(buffer);
    if (len == 0) {
        return;
    }

    /* Chop any whitespace off the end. */
    int end_index = len-1;
    while (end_index >= 0 && isspace(buffer[end_index])) {
        end_index--;
    }
    buffer[end_index + 1] = 0;

    /* Find the first non-zero char. */
    while (start_index < len && isspace(buffer[start_index])) {
        start_index++;
    }
    if (start_index == 0) {
        return;
    }

    int i = 0;
    for (int i=0; (start_index+i)<=len; i++) {
        buffer[i] = buffer[start_index+i];
    }
}

bool string_starts_with(const char *base, const char *prefix) {
    return strncmp(base, prefix, strlen(prefix)) == 0;
}

char nibble_to_char(unsigned char nibble) {
    return nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
}

void print_hex(unsigned char *buffer, size_t count) {
    for (size_t i=0; i<count; i++) {
        unsigned char byte = buffer[i];
        unsigned char nibble = byte & 0xf;
        unsigned char top_nibble = (byte >> 4) & 0xf;
        char c1 = nibble_to_char(top_nibble);
        char c2 = nibble_to_char(nibble);
        printf("%c%c", c1, c2);
    }
}

void sha256_hash_to_hex(unsigned char *bytes, char *hex_buffer) {
    for (int i=0; i<SHA256_SIZE_BYTES; i++) {
        unsigned char b = bytes[i];
        hex_buffer[i*2] = nibble_to_char((b & 0xf0) >> 4);
        hex_buffer[i*2+1] = nibble_to_char(b & 0x0f);
    }
    hex_buffer[SHA256_SIZE_BYTES * 2] = '\0';
}

void convert_to_lowercase(char *line_hash) {
    for (int i = 0; line_hash[i] != '\0'; i++) {
        line_hash[i] = tolower(line_hash[i]);
    }
}

char *arena_strdup(Arena *arena, const char *str) {
    const int len = strlen(str) + 1;

    char *new_str = arena_alloc(arena, len);
    strncpy(new_str, str, len);
    return new_str;
}

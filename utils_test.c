/**
 * Copyright 2023 Simon Edwards <simon@simonzone.com>
 *
 * This source code is licensed under the MIT license which is detailed in the LICENSE.txt file.
 */
#include "utils.c"

#include "libs/munit/munit.c"


MunitResult test_replace_char(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "foo/bar/smeg.txt";
    replace_char(test_string, '/', '_');
    munit_assert_string_equal(test_string, "foo_bar_smeg.txt");
    return MUNIT_OK;
}

MunitResult test_file_extension_index_hit(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "foo/bar/smeg.txt";
    munit_assert_int(12, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_file_extension_index_miss(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "foo/bar/smeg_txt";
    munit_assert_int(-1, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_file_extension_index_miss_2(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "foo/bar.txt/smeg_txt";
    munit_assert_int(-1, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_file_extension_index_miss_3(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "smeg.txt";
    munit_assert_int(4, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_file_extension_index_miss_4(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "smeg_txt";
    munit_assert_int(-1, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_file_extension_index_miss_5(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = ".smeg";
    munit_assert_int(-1, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_file_extension_index_miss_6(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "foo/.smeg";
    munit_assert_int(-1, ==, file_extension_index(test_string));
    return MUNIT_OK;
}

MunitResult test_find_suitable_filename(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_filename[] = "test_fixtures/find_suitable_filename/bar.txt";
    char *new_filename = find_suitable_filename(test_filename);
    munit_assert_string_equal(new_filename, "test_fixtures/find_suitable_filename/bar.txt");
    free(new_filename);
    return MUNIT_OK;
}

MunitResult test_find_suitable_filename_2(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_filename[] = "test_fixtures/find_suitable_filename/foo.txt";
    char *new_filename = find_suitable_filename(test_filename);
    munit_assert_string_equal(new_filename, "test_fixtures/find_suitable_filename/foo(1).txt");
    free(new_filename);
    return MUNIT_OK;
}

MunitResult test_find_suitable_filename_3(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_filename[] = "test_fixtures/find_suitable_filename/bar";
    char *new_filename = find_suitable_filename(test_filename);
    munit_assert_string_equal(new_filename, "test_fixtures/find_suitable_filename/bar(1)");
    free(new_filename);
    return MUNIT_OK;
}

MunitResult test_string_strip(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "foo bar";
    string_strip(test_string);
    munit_assert_string_equal(test_string, "foo bar");
    return MUNIT_OK;
}

MunitResult test_string_strip_2(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "   foo bar";
    string_strip(test_string);
    munit_assert_string_equal(test_string, "foo bar");
    return MUNIT_OK;
}

MunitResult test_string_strip_3(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "   foo bar   ";
    string_strip(test_string);
    munit_assert_string_equal(test_string, "foo bar");
    return MUNIT_OK;
}

MunitResult test_string_strip_4(const MunitParameter params[], void* user_data_or_fixture) {
    static char test_string[] = "   foo bar   \r\n";
    string_strip(test_string);
    munit_assert_string_equal(test_string, "foo bar");
    return MUNIT_OK;
}

MunitResult test_sha256_hash_to_hex(const MunitParameter params[], void* user_data_or_fixture) {
    unsigned char bytes[SHA256_SIZE_BYTES] = "\x12\x34\x56\x78\x9a\xbc\xde\xf0"
        "\x12\x34\x56\x78\x9a\xbc\xde\xf0"
        "\x12\x34\x56\x78\x9a\xbc\xde\xf0"
        "\x12\x34\x56\x78\x9a\xbc\xde\xf0";
    char hex_buffer[SHA256_SIZE_BYTES*2+1];

    sha256_hash_to_hex(bytes, hex_buffer);

    munit_assert_string_equal(hex_buffer, "123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0");

    return MUNIT_OK;
}

MunitResult test_convert_to_lowercase(const MunitParameter params[], void* user_data_or_fixture) {
    static char input[] = "MiX of Upper & Lower case";
    convert_to_lowercase(input);
    munit_assert_string_equal(input, "mix of upper & lower case");
    return MUNIT_OK;
}

MunitTest tests[] = {
    /*name                                 test                              setup tear_down  options                 parameters */
    { "/test_replace_char",                test_replace_char,                NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_hit",    test_file_extension_index_hit,    NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_miss",   test_file_extension_index_miss,   NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_miss_2", test_file_extension_index_miss_2, NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_miss_3", test_file_extension_index_miss_3, NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_miss_4", test_file_extension_index_miss_4, NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_miss_5", test_file_extension_index_miss_5, NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_file_extension_index_miss_6", test_file_extension_index_miss_6, NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_find_suitable_filename",      test_find_suitable_filename,      NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_find_suitable_filename_2",    test_find_suitable_filename_2,    NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_find_suitable_filename_3",    test_find_suitable_filename_3,    NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_string_strip",                test_string_strip,                NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_string_strip_2",              test_string_strip_2,              NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_string_strip_3",              test_string_strip_3,              NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_string_strip_4",              test_string_strip_4,              NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_sha256_hash_to_hex",          test_sha256_hash_to_hex,          NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },
    { "/test_convert_to_lowercase",        test_convert_to_lowercase,        NULL, NULL,      MUNIT_TEST_OPTION_NONE, NULL },

    { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite suite = {
    "tests", /* name */
    tests, /* tests */
    NULL, /* suites */
    1, /* iterations */
    MUNIT_SUITE_OPTION_NONE /* options */
};

int main (int argc, char** argv) {
    return munit_suite_main(&suite, NULL, argc, argv);
}

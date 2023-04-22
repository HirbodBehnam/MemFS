#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crowfs.h"

int test_create_file();

int test_create_folder();

int test_get_entry();

int test_read_write_file();

int test_read_write_file_inflate();

int main(int argc, char **argv) {
    if (argc != 2) {
        puts("Enter the test number as argument");
        return 1;
    }
    int test_number = atoi(argv[1]);
    switch (test_number) {
        case 1:
            return test_create_file();
        case 2:
            return test_create_folder();
        case 3:
            return test_get_entry();
        case 4:
            return test_read_write_file();
        case 5:
            return test_read_write_file_inflate();
        default:
            puts("invalid test number");
            return 1;
    }
}

int test_create_file() {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    assert(crow_fs_create_file(&root, "/hello", 10) == 0);
    assert(crow_fs_create_file(&root, "/my file", 69) == 0);
    assert(crow_fs_create_file(&root, "/rng", 5) == 0);
    assert(crow_fs_create_file(&root, "/rng", 0) == EEXIST);
    assert(crow_fs_create_file(&root, "/non existing folder/file", 0) == ENOENT);
    crow_fs_tree(&root);
    return 0;
}

int test_create_folder() {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    assert(crow_fs_create_folder(&root, "/hello") == 0);
    assert(crow_fs_create_folder(&root, "/hello/world") == 0);
    assert(crow_fs_create_folder(&root, "/hello/world/sup bro") == 0);
    assert(crow_fs_create_folder(&root, "/another dir") == 0);
    assert(crow_fs_create_folder(&root, "/another dir") == EEXIST);
    assert(crow_fs_create_folder(&root, "/not found/directory/welp") == ENOENT);
    assert(crow_fs_create_file(&root, "/another dir/file", 0) == 0);
    assert(crow_fs_create_file(&root, "/hello/file", 0) == 0);
    assert(crow_fs_create_file(&root, "/hello/world/file", 0) == 0);
    assert(crow_fs_create_file(&root, "/hello/world/sup bro/file", 0) == 0);
    crow_fs_tree(&root);
    return 0;
}

int test_get_entry() {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    // Create basic entries
    assert(crow_fs_create_folder(&root, "/hello") == 0);
    assert(crow_fs_create_folder(&root, "/hello/world") == 0);
    assert(crow_fs_create_file(&root, "/hello/file", 10) == 0);
    assert(crow_fs_create_file(&root, "/hello/world/file", 20) == 0);
    // Get each entry in file system
    // Hello folder
    struct crow_fs_entry entry;
    assert(crow_fs_get_entry(&root, "/hello", &entry) == 0);
    assert(entry.type == CROW_FS_FOLDER);
    assert(strcmp(entry.name, "hello") == 0);
    assert(
            strcmp(entry.data.directory->entries->name, "world") == 0 ||
            strcmp(entry.data.directory->entries->name, "file") == 0
    );
    assert(
            strcmp(entry.data.directory->entries->next->name, "world") == 0 ||
            strcmp(entry.data.directory->entries->next->name, "file") == 0
    );
    assert(entry.data.directory->entries->next->next == NULL);
    // File
    assert(crow_fs_get_entry(&root, "/hello/file", &entry) == 0);
    assert(entry.type == CROW_FS_FILE);
    assert(strcmp(entry.name, "file") == 0);
    assert(entry.data.file->size == 10);
    const char empty_buffer[10] = {0};
    assert(memcmp(entry.data.file->data, empty_buffer, 10) == 0);
    // Non existent file and folder
    assert(crow_fs_get_entry(&root, "/hello world/", &entry) == ENOENT);
    assert(crow_fs_get_entry(&root, "/hello/no", &entry) == ENOENT);
    return 0;
}

int test_read_write_file() {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    const char to_write_buffer[] = "Hello world!";
    assert(crow_fs_create_file(&root, "/file", 1024) == 0);
    assert(crow_fs_create_folder(&root, "/folder") == 0);
    assert(crow_fs_write(&root, "/file", sizeof(to_write_buffer), to_write_buffer, 0) == sizeof(to_write_buffer));
    assert(crow_fs_write(&root, "/file", sizeof(to_write_buffer), to_write_buffer, 100) == sizeof(to_write_buffer));
    // This should have not inflated the file
    struct crow_fs_entry entry;
    assert(crow_fs_get_entry(&root, "/file", &entry) == 0);
    assert(entry.data.file->size == 1024);
    // Create the file content
    char expected_buffer[1024] = {0};
    strcpy(expected_buffer, to_write_buffer);
    strcpy(expected_buffer + 100, to_write_buffer);
    // Read the file
    char read_buffer[1024] = {0};
    assert(crow_fs_read(&root, "/file", 1024, read_buffer, 0) == sizeof(read_buffer));
    assert(memcmp(read_buffer, expected_buffer, 1024) == 0);
    // Read from offset
    memset(read_buffer, 0, sizeof(read_buffer));
    memset(expected_buffer, 0, sizeof(read_buffer));
    strcpy(expected_buffer, to_write_buffer);
    assert(crow_fs_read(&root, "/file", 1024, read_buffer, 100) == 1024 - 100);
    assert(memcmp(read_buffer, expected_buffer, 1024) == 0);
    // Out of bound read
    assert(crow_fs_read(&root, "/file", 1024, read_buffer, 1024) == 0);
    assert(crow_fs_read(&root, "/file", 1024, read_buffer, 1024 + 1) == 0);
    // Check errors
    assert(crow_fs_read(&root, "/folder", 1024, read_buffer, 0) == -EISDIR);
    assert(crow_fs_read(&root, "/nope", 1024, read_buffer, 0) == -ENOENT);
    return 0;
}

int test_read_write_file_inflate() {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    const char to_write_buffer[] = "Hello world!";
    assert(crow_fs_create_file(&root, "/file", 0) == 0);
    assert(crow_fs_create_folder(&root, "/folder") == 0);
    assert(crow_fs_write(&root, "/file", sizeof(to_write_buffer), to_write_buffer, 0) == sizeof(to_write_buffer));
    assert(crow_fs_write(&root, "/file", sizeof(to_write_buffer), to_write_buffer, 100) == sizeof(to_write_buffer));
    // This should have not inflated the file
    struct crow_fs_entry entry;
    assert(crow_fs_get_entry(&root, "/file", &entry) == 0);
    assert(entry.data.file->size == 100 + sizeof(to_write_buffer));
    // Create the file content
    char expected_buffer[1024] = {0};
    strcpy(expected_buffer, to_write_buffer);
    strcpy(expected_buffer + 100, to_write_buffer);
    // Read the file
    char read_buffer[1024] = {0};
    assert(crow_fs_read(&root, "/file", 1024, read_buffer, 0) == 100 + sizeof(to_write_buffer));
    assert(memcmp(read_buffer, expected_buffer, 1024) == 0);
    return 0;
}
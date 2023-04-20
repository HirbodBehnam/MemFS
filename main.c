#include <stdio.h>
#include <assert.h>
#include "crowfs.h"

int main(int argc, char *argv[]) {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    crow_fs_create_file(&root, "/hello", 10);
    crow_fs_create_file(&root, "/my file", 69);
    crow_fs_create_folder(&root, "/folder");
    crow_fs_create_file(&root, "/folder/file in folder", 0);
    crow_fs_create_file(&root, "/rng", 5);
    crow_fs_tree(&root);
    {
        const char buffer_to_write[] = "Hello world!";
        char read_buffer[1024 * 32];
        int write_bytes = crow_fs_write(&root, "/folder/file in folder", sizeof(buffer_to_write), buffer_to_write, 0);
        assert(sizeof(buffer_to_write) == write_bytes);
        int read_bytes = crow_fs_read(&root, "/folder/file in folder", sizeof(read_buffer), read_buffer, 0);
        assert(sizeof(buffer_to_write) == read_bytes);
        puts(read_buffer);
        const int offset = 2;
        read_bytes = crow_fs_read(&root, "/folder/file in folder", sizeof(read_buffer), read_buffer, offset);
        assert(sizeof(buffer_to_write) == (read_bytes + offset));
        puts(read_buffer);
    }
    {
        const char buffer_to_write[] = "Sup bro";
        char read_buffer[1024 * 32];
        const int offset = 3;
        int write_bytes = crow_fs_write(&root, "/rng", sizeof(buffer_to_write), buffer_to_write, offset);
        assert(sizeof(buffer_to_write) == write_bytes);
        int read_bytes = crow_fs_read(&root, "/rng", sizeof(read_buffer), read_buffer, 0);
        assert(sizeof(buffer_to_write) + offset == read_bytes);
        puts(read_buffer + offset);
        for (int i = 0; i < offset; i++)
            assert(read_buffer[i] == 0);
    }
    crow_fs_tree(&root);
    return 0;
}
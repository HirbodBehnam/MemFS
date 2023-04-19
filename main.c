#include "crowfs.h"

int main(int argc, char *argv[]) {
    struct crow_fs_directory root;
    crow_fs_new(&root);
    crow_fs_create_file(&root, "/hello", 10);
    crow_fs_create_file(&root, "/my file", 69);
    crow_fs_create_folder(&root, "/folder");
    crow_fs_create_file(&root, "/folder/file in folder", 0);
    crow_fs_create_file(&root, "/rng", 1);
    crow_fs_tree(&root);
    return 0;
}
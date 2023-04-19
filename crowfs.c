#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "crowfs.h"

/**
 * Checks if a string is an empty string or not.
 * @param string The string to check.
 * @return True if empty otherwise false.
 */
static bool is_empty_string(const char *string) {
    return string[0] == '\0';
}

static void indent_tree(int depth) {
    for (int i = 0; i < depth; i++)
        printf("|   ");
}

static void crow_fs_tree_internal(const struct crow_fs_directory *root, int depth) {
    for (struct crow_fs_entry *current_entry = root->entries;
         current_entry != NULL;
         current_entry = current_entry->next) {
        indent_tree(depth);
        printf("+-- ");
        switch (current_entry->type) {
            case CROW_FS_FOLDER:
                printf("%s\n", current_entry->name);
                crow_fs_tree_internal(current_entry->data.directory, depth + 1);
                break;
            case CROW_FS_FILE:
                printf("%s (%zu bytes)\n", current_entry->name, current_entry->data.file->size);
                break;
            case CROW_FS_LINK:
                // TODO: later...
                break;
        }
    }
}

void crow_fs_new(struct crow_fs_directory *root) {
    root->entries = NULL; // we only set the root to null. (no files in this folder)
}

void crow_fs_tree(const struct crow_fs_directory *root) {
    printf("+-- /\n");
    crow_fs_tree_internal(root, 1);
}

int crow_fs_get_entry(const struct crow_fs_directory *root, const char *path, struct crow_fs_entry *entry) {
    // Check literal root folder
    if (strcmp("/", path) == 0) {
        strcpy(entry->name, "/");
        entry->type = CROW_FS_FOLDER;
        entry->data.directory = root;
        entry->next = NULL;
    }
    // Traverse the file system
    int result = 0;
    char *path_copy = strdup(path);
    char *token;
    char *rest = path_copy;
    while ((token = strtok_r(rest, "/", &rest))) {
        bool found = false;
        bool last_part = is_empty_string(rest);
        // Check each entry in file this directory
        for (struct crow_fs_entry *current_entry = root->entries;
             current_entry != NULL;
             current_entry = current_entry->next) {
            if (strcmp(current_entry->name, token) == 0) { // same name
                if (last_part) { // On last part just set the results
                    *entry = *current_entry; // copy all fields
                    entry->next = NULL; // except the next value
                    found = true;
                } else {
                    // Allow folders only if we are not at last part
                    if (current_entry->type == CROW_FS_FOLDER) {
                        root = current_entry->data.directory;
                        found = true;
                    }
                }
                break;
            }
        }
        if (!found) { // cannot find the file. Just break out.
            result = ENOENT;
            break;
        }
    }
    // Clean up
    free(path_copy);
    return result;
}

int crow_fs_create_file(struct crow_fs_directory *root, const char *path, size_t file_size) {
    int result = 0;
    char filename[MAX_FILE_NAME + 1] = {'\0'};
    // Traverse the file system
    char *path_copy = strdup(path);
    char *token;
    char *rest = path_copy;
    while ((token = strtok_r(rest, "/", &rest))) {
        bool found = false;
        bool last_part = is_empty_string(rest);
        // Check each entry in file this directory
        for (struct crow_fs_entry *current_entry = root->entries;
             current_entry != NULL;
             current_entry = current_entry->next) {
            // TODO: at the last step, we shall truncate the token.
            if (strcmp(current_entry->name, token) == 0) { // same name
                found = true;
                /**
                 * If this is not the last part of the path, we are allowed to go to new folders.
                 * Otherwise (this is the last part) we shall give user error that the file already exists.
                 */
                if (!last_part) {
                    if (current_entry->type == CROW_FS_FOLDER) { // we can only enter folders
                        root = current_entry->data.directory;
                    } else { // we cant enter other files. NOT FOUND!
                        found = false;
                    }
                }
                break;
            }
        }
        if (found && last_part) { // file already exists
            result = EEXIST;
            break;
        } else if (!found && !last_part) { // not found part of path
            result = ENOENT;
            break;
        }
        // Copy the filename if this is the last part of path
        if (last_part)
            strncpy(filename, token, sizeof(filename) - 1);
    }
    free(path_copy); // clean up. We have a backup of our filename in filename array
    // Check for errors
    if (result != 0)
        return result;
    // Create the file
    struct crow_fs_entry *new_entry = malloc(sizeof(struct crow_fs_entry));
    new_entry->type = CROW_FS_FILE;
    strcpy(new_entry->name, filename); // this is safe. I already truncated the length in iteration over path.
    new_entry->data.file = malloc(sizeof(struct crow_fs_file));
    new_entry->data.file->data = calloc(file_size, sizeof(char));
    new_entry->data.file->size = file_size;
    // Add it to directory
    new_entry->next = root->entries;
    root->entries = new_entry;
    return 0;
}

int crow_fs_create_folder(struct crow_fs_directory *root, const char *path) {
    int result = 0;
    char filename[MAX_FILE_NAME + 1] = {'\0'};
    // Traverse the file system
    char *path_copy = strdup(path);
    char *token;
    char *rest = path_copy;
    while ((token = strtok_r(rest, "/", &rest))) {
        bool found = false;
        bool last_part = is_empty_string(rest);
        // Check each entry in file this directory
        for (struct crow_fs_entry *current_entry = root->entries;
             current_entry != NULL;
             current_entry = current_entry->next) {
            // TODO: at the last step, we shall truncate the token.
            if (strcmp(current_entry->name, token) == 0) { // same name
                found = true;
                /**
                 * If this is not the last part of the path, we are allowed to go to new folders.
                 * Otherwise (this is the last part) we shall give user error that the file already exists.
                 */
                if (!last_part) {
                    if (current_entry->type == CROW_FS_FOLDER) { // we can only enter folders
                        root = current_entry->data.directory;
                    } else { // we cant enter other files. NOT FOUND!
                        found = false;
                    }
                }
                break;
            }
        }
        if (found && last_part) { // file already exists
            result = EEXIST;
            break;
        } else if (!found && !last_part) { // not found part of path
            result = ENOENT;
            break;
        }
        // Copy the filename if this is the last part of path
        if (last_part)
            strncpy(filename, token, sizeof(filename) - 1);
    }
    free(path_copy); // clean up. We have a backup of our filename in filename array
    // Check for errors
    if (result != 0)
        return result;
    // Create the file
    struct crow_fs_entry *new_entry = malloc(sizeof(struct crow_fs_entry));
    new_entry->type = CROW_FS_FOLDER;
    strcpy(new_entry->name, filename); // this is safe. I already truncated the length in iteration over path.
    new_entry->data.directory = malloc(sizeof(struct crow_fs_directory));
    new_entry->data.directory->entries = NULL;
    // Add it to directory
    new_entry->next = root->entries;
    root->entries = new_entry;
    return 0;
}
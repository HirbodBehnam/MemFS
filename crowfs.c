#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "crowfs.h"

#define MIN(x, y) ((x < y) ? (x) : (y))

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

/**
 * Write a buffer to file, inflating the buffer if needed
 * @param file The file to write to
 * @param buffer_size Size of buffer to write
 * @param buffer The buffer itself
 * @param offset Offset to write to
 * @return Bytes written or negative value on error
 */
static int write_to_file(struct crow_fs_file *file, size_t buffer_size, const char *buffer, off_t offset) {
    // Check size of buffer
    if (offset + buffer_size > file->size) {
        // Try to make the buffer bigger
        char *new_data = realloc(file->data, offset + buffer_size);
        if (new_data == NULL)
            return -ENOSPC;
        file->size = offset + buffer_size;
        file->data = new_data;
    }
    // Just copy to buffer
    memcpy(file->data + offset, buffer, buffer_size);
    return (int) buffer_size;
}

/**
 * Reads a buffer from a file
 * @param file The file to read from
 * @param buffer_size The buffer size to read to
 * @param buffer The buffer to read to
 * @param offset The offset of the file to read to
 * @return Bytes read. This function is error free
 */
static int read_from_file(const struct crow_fs_file *file, size_t buffer_size, char *buffer, off_t offset) {
    // Bound check
    if (offset > file->size)
        return 0;
    // Get the size to copy
    size_t to_copy_size = MIN(buffer_size, file->size - offset);
    memcpy(buffer, file->data + offset, to_copy_size);
    return (int) to_copy_size;
}

void crow_fs_new(struct crow_fs_directory *root) {
    root->entries = NULL; // we only set the root to null. (no files in this folder)
}

void crow_fs_tree(const struct crow_fs_directory *root) {
    printf("+-- /\n");
    crow_fs_tree_internal(root, 1);
}

int crow_fs_get_entry(struct crow_fs_directory *root, const char *path, struct crow_fs_entry *entry) {
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

int
crow_fs_write(struct crow_fs_directory *root, const char *path, size_t buffer_size, const char *buffer, off_t offset) {
    // Get the file
    struct crow_fs_entry entry;
    int get_entry_status = crow_fs_get_entry(root, path, &entry);
    if (get_entry_status != 0)
        return -get_entry_status;
    // TODO: read link if needed?
    // Check if this is a file
    if (entry.type == CROW_FS_FOLDER)
        return -EISDIR;
    // Write to file
    return write_to_file(entry.data.file, buffer_size, buffer, offset);
}

int
crow_fs_read(struct crow_fs_directory *root, const char *path, size_t buffer_size, char *buffer, off_t offset) {
    // Get the file
    struct crow_fs_entry entry;
    int get_entry_status = crow_fs_get_entry(root, path, &entry);
    if (get_entry_status != 0)
        return -get_entry_status;
    // TODO: read link if needed?
    // Check if this is a file
    if (entry.type == CROW_FS_FOLDER)
        return -EISDIR;
    // Read
    return read_from_file(entry.data.file, buffer_size, buffer, offset);
}
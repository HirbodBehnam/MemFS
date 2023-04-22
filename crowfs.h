#include <stddef.h>
#include <sys/types.h>

#ifndef CROWFS_CROWFS_H
#define CROWFS_CROWFS_H

#endif //CROWFS_CROWFS_H

#define MAX_FILE_NAME 63

enum crow_fs_entry_type {
    CROW_FS_FOLDER,
    CROW_FS_FILE,
    CROW_FS_LINK,
};

struct crow_fs_entry {
    /**
     * What kind of entry is this?
     */
    enum crow_fs_entry_type type;
    /**
     * The data which this entry holds. The type of data depends on type.
     */
    union {
        struct crow_fs_directory *directory;
        struct crow_fs_file *file;
        struct crow_fs_link *link;
    } data;
    /**
     * The name of this file/folder/link
     */
    char name[MAX_FILE_NAME + 1];
    /**
     * Next element in linked list. Can be NULL.
     */
    struct crow_fs_entry *next;
};

struct crow_fs_directory {
    /**
     * List of files/folder/links this folder has. This is a linked list.
     */
    struct crow_fs_entry *entries;
};

struct crow_fs_file {
    /**
     * The size of this file
     */
    size_t size;
    /**
     * The data which this file holds. Note that this field is allocated with malloc and must be freed with free.
     */
    char *data;
};

struct crow_fs_link {
    // TODO: implement
};

/**
 * Creates a new file system.
 * @param root The root of file system to initiate the file system in it.
 */
void crow_fs_new(struct crow_fs_directory *root);

/**
 * Prints the tree of the file system in stdout
 * @param root The file directory to start printing from
 */
void crow_fs_tree(const struct crow_fs_directory *root);

/**
 * Gets the entry if it exists
 * @param root The root of file system
 * @param path The path of the file to get its info.
 * @param entry The entry to fill the info of file in it.
 * @return 0 if everything is ok. Otherwise the error value.
 */
int crow_fs_get_entry(struct crow_fs_directory *root, const char *path, struct crow_fs_entry *entry);

/**
 * Create a file in specified path.
 * @param root The root of file system
 * @param path The path to create the file. The last part of this path is the filename.
 * @param file_size Size of file in bytes.
 * @return 0 if everything is ok.
 */
int crow_fs_create_file(struct crow_fs_directory *root, const char *path, size_t file_size);

/**
 * Creates a new folder in a path
 * @param root The root of file system
 * @param path The path to create the folder in
 * @return 0 if everything is ok.
 */
int crow_fs_create_folder(struct crow_fs_directory *root, const char *path);

/**
 * Writes to a file, inflates it if needed
 * @param root The root of file system
 * @param path The file to write to
 * @param buffer_size Buffer size to write to
 * @param buffer The buffer to write to file
 * @param offset The offset to write the buffer in file
 * @return Negative value on error or bytes written to disk
 */
int
crow_fs_write(struct crow_fs_directory *root, const char *path, size_t buffer_size, const char *buffer, off_t offset);

/**
 * Reads from a file system
 * @param root The root of file system
 * @param path The file to read from
 * @param buffer_size Buffer size to read to
 * @param buffer The buffer to read into
 * @param offset The offset to read the buffer from file
 * @return On error, returns the negative value of errno (just like fuse). On Success, returns the bytes read.
 */
int
crow_fs_read(struct crow_fs_directory *root, const char *path, size_t buffer_size, char *buffer, off_t offset);

/**
 * Resizes a file to a new size. Fills added bytes with zero.
 * @param root The root of file system
 * @param path The path to create the file. The last part of this path is the filename.
 * @param new_size New size of file in bytes.
 * @return 0 if everything is ok.
 */
int crow_fs_resize_file(struct crow_fs_directory *root, const char *path, size_t new_size);
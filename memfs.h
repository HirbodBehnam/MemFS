#include <stddef.h>
#include <sys/types.h>

#ifndef CROWFS_CROWFS_H
#define CROWFS_CROWFS_H

#endif //CROWFS_CROWFS_H

#define MAX_FILE_NAME 63

enum mem_fs_entry_type {
    CROW_FS_FOLDER,
    CROW_FS_FILE,
    CROW_FS_LINK,
};

struct mem_fs_entry {
    /**
     * What kind of entry is this?
     */
    enum mem_fs_entry_type type;
    /**
     * The data which this entry holds. The type of data depends on type.
     */
    union {
        struct mem_fs_directory *directory;
        struct mem_fs_file *file;
        struct mem_fs_link *link;
    } data;
    /**
     * The name of this file/folder/link
     */
    char name[MAX_FILE_NAME + 1];
    /**
     * Next element in linked list. Can be NULL.
     */
    struct mem_fs_entry *next;
};

struct mem_fs_directory {
    /**
     * List of files/folder/links this folder has. This is a linked list.
     */
    struct mem_fs_entry *entries;
};

struct mem_fs_file {
    /**
     * The size of this file
     */
    size_t size;
    /**
     * The data which this file holds. Note that this field is allocated with malloc and must be freed with free.
     */
    char *data;
};

struct mem_fs_link {
    // TODO: implement
};

/**
 * Creates a new file system.
 * @param root The root of file system to initiate the file system in it.
 */
void mem_fs_new(struct mem_fs_directory *root);

/**
 * Prints the tree of the file system in stdout
 * @param root The file directory to start printing from
 */
void mem_fs_tree(const struct mem_fs_directory *root);

/**
 * Gets the entry if it exists
 * @param root The root of file system
 * @param path The path of the file to get its info.
 * @param entry The entry to fill the info of file in it.
 * @return 0 if everything is ok. Otherwise the error value.
 */
int mem_fs_get_entry(struct mem_fs_directory *root, const char *path, struct mem_fs_entry *entry);

/**
 * Create a file in specified path.
 * @param root The root of file system
 * @param path The path to create the file. The last part of this path is the filename.
 * @param file_size Size of file in bytes.
 * @return 0 if everything is ok.
 */
int mem_fs_create_file(struct mem_fs_directory *root, const char *path, size_t file_size);

/**
 * Creates a new folder in a path
 * @param root The root of file system
 * @param path The path to create the folder in
 * @return 0 if everything is ok.
 */
int mem_fs_create_folder(struct mem_fs_directory *root, const char *path);

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
mem_fs_write(struct mem_fs_directory *root, const char *path, size_t buffer_size, const char *buffer, off_t offset);

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
mem_fs_read(struct mem_fs_directory *root, const char *path, size_t buffer_size, char *buffer, off_t offset);

/**
 * Resizes a file to a new size. Fills added bytes with zero.
 * @param root The root of file system
 * @param path The path to create the file. The last part of this path is the filename.
 * @param new_size New size of file in bytes.
 * @return 0 if everything is ok.
 */
int mem_fs_resize_file(struct mem_fs_directory *root, const char *path, size_t new_size);

/**
 * Removes a single file
 * @param root The root of file system
 * @param path Path of file to delete. This must be a file or link. Not a folder
 * @return 0 if deletion was ok.
 */
int mem_fs_rm_file(struct mem_fs_directory *root, const char *path);

/**
 * Removes an empty directory
 * @param root The root of file system
 * @param path Folder to delete. Must be an empty folder
 * @return 0 if deletion was ok.
 */
int mem_fs_rm_dir(struct mem_fs_directory *root, const char *path);
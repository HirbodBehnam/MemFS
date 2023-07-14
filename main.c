#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse.h>
#include <string.h>
#include <pthread.h>
#include "memfs.h"

/**
 * The root of file system
 */
static struct mem_fs_directory fs_root;
static pthread_rwlock_t fs_mutex = PTHREAD_RWLOCK_INITIALIZER;

static struct options {
} options;

static const struct fuse_opt option_spec[] = {
        FUSE_OPT_END
};

static void *mem_fuse_init(struct fuse_conn_info *conn,
                           struct fuse_config *cfg) {
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int mem_fuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
    memset(stbuf, 0, sizeof(struct stat));
    // Get the entry from file list
    struct mem_fs_entry entry;
    pthread_rwlock_rdlock(&fs_mutex);
    int result = mem_fs_get_entry(&fs_root, path, &entry);
    if (result != 0) {
        result = -result;
        goto end;
    }
    // Check the type
    switch (entry.type) {
        case CROW_FS_FOLDER:
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            break;
        case CROW_FS_FILE:
            stbuf->st_mode = S_IFREG | 0777;
            stbuf->st_nlink = 1;
            stbuf->st_size = (long) entry.data.file->size;
            break;
        case CROW_FS_LINK:
            // TODO: later
            break;
    }
    end:
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi,
                            enum fuse_readdir_flags flags) {
    (void) fi;
    (void) flags;
    (void) offset;
    // Get the folder
    struct mem_fs_entry entry;
    pthread_rwlock_rdlock(&fs_mutex);
    int result = mem_fs_get_entry(&fs_root, path, &entry);
    if (result != 0) {
        result = -result;
        goto end;
    }
    if (entry.type != CROW_FS_FOLDER) { // we are looking for folders
        result = -ENOENT;
        goto end;
    }
    // Check offset for up folder
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    // Skip the offset dirs
    struct mem_fs_entry *folder_content = entry.data.directory->entries;
    // Put them in?
    while (folder_content != NULL) {
        struct stat stbuf = {};
        switch (folder_content->type) {
            case CROW_FS_FOLDER:
                stbuf.st_mode = S_IFDIR | 0755;
                stbuf.st_nlink = 2;
                break;
            case CROW_FS_FILE:
                stbuf.st_mode = S_IFREG | 0777;
                stbuf.st_nlink = 1;
                stbuf.st_size = (long) folder_content->data.file->size;
                break;
            case CROW_FS_LINK:
                // TODO: later
                break;
        }
        // Put the entry in stuff
        int filler_result = filler(buf, folder_content->name, &stbuf, 0, 0);
        if (filler_result == 1) // buffer full
            break;
        // Iterate
        folder_content = folder_content->next;
    }
    end:
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_open(const char *path, struct fuse_file_info *fi) {
    // Check if it exists
    struct mem_fs_entry entry;
    pthread_rwlock_wrlock(&fs_mutex);
    int result = mem_fs_get_entry(&fs_root, path, &entry);
    if (result == ENOENT && (fi->flags & O_CREAT) != 0) { // if specified, create the file
        result = -mem_fs_create_file(&fs_root, path, 0);
        goto end;
    }
    if (result != 0) {
        result = -result;
        goto end;
    }
    // We don't allow dirs
    if (entry.type == CROW_FS_FOLDER) {
        result = -EISDIR;
        goto end;
    }
    // Truncate the file if needed
    if ((fi->flags & O_TRUNC) != 0) {
        result = -mem_fs_resize_file(&fs_root, path, 0);
        goto end;
    }
    end:
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_read(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi) {
    (void) fi;
    pthread_rwlock_rdlock(&fs_mutex);
    int result = mem_fs_read(&fs_root, path, size, buf, offset);
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_write(const char *path, const char *buf, size_t size, off_t offset,
                          struct fuse_file_info *fi) {
    (void) fi;
    pthread_rwlock_wrlock(&fs_mutex);
    int result = mem_fs_write(&fs_root, path, size, buf, offset);
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_rmdir(const char *path) {
    pthread_rwlock_wrlock(&fs_mutex);
    int result = -mem_fs_rm_dir(&fs_root, path);
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_rmfile(const char *path) {
    pthread_rwlock_wrlock(&fs_mutex);
    int result = -mem_fs_rm_file(&fs_root, path);
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_create_file(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) mode;
    (void) fi;
    pthread_rwlock_wrlock(&fs_mutex);
    int result = -mem_fs_create_file(&fs_root, path, 0);
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static int mem_fuse_create_directory(const char *path, mode_t mode) {
    (void) mode;
    pthread_rwlock_wrlock(&fs_mutex);
    int result = -mem_fs_create_folder(&fs_root, path);
    pthread_rwlock_unlock(&fs_mutex);
    return result;
}

static const struct fuse_operations mem_fuse_operations = {
        .init = mem_fuse_init,
        .getattr = mem_fuse_getattr,
        .readdir = mem_fuse_readdir,
        .open = mem_fuse_open,
        .read = mem_fuse_read,
        .write = mem_fuse_write,
        .rmdir = mem_fuse_rmdir,
        .unlink = mem_fuse_rmfile,
        .create = mem_fuse_create_file,
        .mkdir = mem_fuse_create_directory,
};

int main(int argc, char *argv[]) {
    // Initiate the file system
    mem_fs_new(&fs_root);
    // Initiate fuse
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
        return 1;
    int ret = fuse_main(args.argc, args.argv, &mem_fuse_operations, NULL);
    fuse_opt_free_args(&args);
    return ret;
}

# MemFS

Simple in memory file system with directory support written using libFUSE.

## Features

* Directory structure without depth limit
* Unlimited file size as long as you have RAM
* 63 characters for each file/directory name
* (TODO) Link/shortcut support

## Building

To build the app you need libFUSE and cmake. To install these components on Ubuntu, try
this command:

```bash
apt install cmake libfuse-dev build-essentials
```

Now to build the app, follow these steps. In nutshell, these steps
will create a makefile to compile the app with full optimizations.

```bash
git clone https://github.com/HirbodBehnam/MemFS
cd MemFS
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

The driver will be created with the filename of `MemFS`.

### Running tests

This app contains some unit tests. To run them, you need to at first build the app using the steps above; Then
run `make test` to test the app. The result should look like this:

```
Running tests...
Test project /home/hirbod/Desktop/Project/C/MemFS/cmake-build-debug
    Start 1: memfs_internal_create_file
1/8 Test #1: memfs_internal_create_file ...............   Passed    0.00 sec
    Start 2: memfs_internal_create_folder
2/8 Test #2: memfs_internal_create_folder .............   Passed    0.00 sec
    Start 3: memfs_internal_get_entry
3/8 Test #3: memfs_internal_get_entry .................   Passed    0.00 sec
    Start 4: memfs_internal_read_write_file
4/8 Test #4: memfs_internal_read_write_file ...........   Passed    0.00 sec
    Start 5: memfs_internal_read_write_file_inflate
5/8 Test #5: memfs_internal_read_write_file_inflate ...   Passed    0.00 sec
    Start 6: memfs_internal_resize_file
6/8 Test #6: memfs_internal_resize_file ...............   Passed    0.00 sec
    Start 7: memfs_internal_delete_file
7/8 Test #7: memfs_internal_delete_file ...............   Passed    0.00 sec
    Start 8: memfs_internal_delete_folder
8/8 Test #8: memfs_internal_delete_folder .............   Passed    0.00 sec

100% tests passed, 0 tests failed out of 8

Total Test time (real) =   0.01 sec
```

## Running

To run the app, at first you need a mount point to mount the filesystem.
For this example, I create a mount point using this commands:

```bash
sudo mkdir /media/hirbod/memfs
sudo chown hirbod:hirbod /media/hirbod/memfs
sudo chmod 777 /media/hirbod/memfs
```

Next, we need to run the FUSE application in order to mount the filesystem in the specified path. To mount the
filesystem, use the following command:

```bash
./MemFS -f /media/hirbod/memfs
```

This will run the driver at foreground and thus can be killed with a simple SIGINT. If you wish to run the app in
background (without the `-f` argument),
you need to unmount the mount point after you are done. To do so, you can use the following command:

```bash
fusermount -u /media/hirbod/memfs
```

This will unmount the partition.

## Internals

### Directories

Directories are basically linked lists of entries. Each entry can either be a folder, a file or a link. This enables us
to create arbitrary large and deep directory structures.

### File

A file is simply a buffer which contains the content of file + the size of the file. The content is allocated
using `malloc` so files can be as large as your RAM.

### Links

NOT YET IMPLEMENTED

### Entry

All files, directories and links are stored in a "parent"-like structure called entry. The entry is defined like this:

```c
struct mem_fs_entry {
    enum mem_fs_entry_type type;
    union {
        struct mem_fs_directory *directory;
        struct mem_fs_file *file;
        struct mem_fs_link *link;
    } data;
    char name[64];
    struct mem_fs_entry *next;
};
```

The `type` field contains the type of this entry. This can be either a link, directory or file. Based on this value, the
element which shall be accessed in union is determined.
`name` is the name of the file or folder. We have to manually check to see if this is unique in each folder.
`next` is the pointer to next entry in current folder. This is `NULL` if current entry is the last entry in folder.

### TODOs

* Efficient move: Currently, moving a file acts like a copy + delete
* Links
* More fuse method implementations
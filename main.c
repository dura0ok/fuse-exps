#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>

static const char *mountpoint = NULL;
static const char *data_directory = "data";

double random_probability() {
    return (double)rand() / RAND_MAX;
}

static int do_noop_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // 50% probability of failing with a no-op read
    if (random_probability() < 0.5) {
        return -EIO; // Simulate an I/O error
    }

    // Otherwise, perform a normal read
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    int res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }

    close(fd);
    return res;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    char full_path[PATH_MAX];
    sprintf(full_path, "%s%s", mountpoint, path);

    DIR *dir = opendir(full_path);
    if (dir == NULL) {
        return -errno;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        filler(buf, entry->d_name, NULL, 0);
    }

    closedir(dir);
    return 0;
}


static int do_getattr(const char *path, struct stat *stbuf) {
    char full_path[PATH_MAX];
    sprintf(full_path, "%s%s", mountpoint, path);

    // Check if the requested path is the data directory
    if (strcmp(path, data_directory) == 0) {
        // If it is the data directory, set appropriate attributes
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;  // Return success
    }

    // Otherwise, retrieve file attributes and populate stbuf
    int res = lstat(full_path, stbuf);
    if (res == -1) {
        return -errno;
    }
    return 0;  // Return success
}

static int do_open(const char *path, struct fuse_file_info *fi) {
    char full_path[PATH_MAX];
    sprintf(full_path, "%s%s", mountpoint, path);

    // Check if the requested path is the data directory
    if (strcmp(path, data_directory) == 0) {
        return -EISDIR;  // Cannot open a directory
    }

    int fd = open(full_path, fi->flags);
    if (fd == -1) {
        return -errno;
    }

    fi->fh = fd;
    return 0;  // Return success
}

static int do_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0; // Return success
}

static int do_readlink(const char *path, char *buf, size_t size) {
    char full_path[PATH_MAX];
    sprintf(full_path, "%s%s", mountpoint, path);

    ssize_t res = readlink(full_path, buf, size - 1);
    if (res == -1) {
        return -errno;
    }

    buf[res] = '\0'; // Null-terminate the string
    return 0; // Return success
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    ssize_t res = pread(fi->fh, buf, size, offset);
    if (res == -1) {
        return -errno;
    }
    return res; // Return the number of bytes read
}

static struct fuse_operations myfuse_operations = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .open = do_open,
    .release = do_release,
    .readlink = do_readlink,
    .read = do_read,
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <mountpoint>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    mountpoint = argv[1];

    srand(time(NULL));

    return fuse_main(argc, argv, &myfuse_operations, NULL);
}

#include <fuse/fuse_opt.h>
#include <stddef.h>
#define FUSE_USE_VERSION 26

#define _GNU_SOURCE

#define _FILE_OFFSET_BITS 64
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char mount_point[4096];
static size_t mountp_index;

char *app_path(const char *path) {
	size_t plen = strlen(path);
	memcpy(mount_point + mountp_index, path, plen - 1);
	puts(path);
	mount_point[mountp_index + plen] = '\0';
	puts(mount_point);
	return mount_point;
}

static int pfs_getattr(const char *path, struct stat *st) {
    int res;

	st->st_uid = getuid(); // The owner of the file/directory is the user who mounted the filesystem
	st->st_gid = getgid(); // The group of the file/directory is the same as the group of the user who mounted the filesystem
	st->st_atime = time( NULL ); // The last "a"ccess of the file/directory is right now
	st->st_mtime = time( NULL ); // The last "m"odification of the file/directory is right now
	
	if ( strcmp( path, "/" ) == 0 )
	{
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
	}
	else
	{
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
	}

    return 0;
}

static int pfs_access(const char *path, int mask) {
    int res;

    res = access(app_path(path), mask);
    if (res == -1)
        return -errno;

    return 0;
}

static int pfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    DIR *dp;
    struct dirent *de;

    (void)offset;
    (void)fi;

    dp = opendir(app_path(path));
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0))
            break;
    }

    closedir(dp);
    return 0;
}

static int pfs_mkdir(const char *path, mode_t mode) {
    int res;

    res = mkdir(app_path(path), mode);
    if (res == -1)
        return -errno;

    return 0;
}

static int pfs_rmdir(const char *path) {
    int res;

    res = rmdir(app_path(path));
    if (res == -1)
        return -errno;

    return 0;
}

static int pfs_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
    int res;

    res = open(app_path(path), fi->flags, mode);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int pfs_open(const char *path, struct fuse_file_info *fi) {
    int res;

    res = open(app_path(path), fi->flags);
    if (res == -1)
        return -errno;

    fi->fh = res;
    return 0;
}

static int pfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    int fd;
    int res;

    if (fi == NULL)
        fd = open(app_path(path), O_RDONLY);
    else
        fd = fi->fh;

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    if (fi == NULL)
        close(fd);
    return res;
}

static const struct fuse_operations pfs_oper = {
    .getattr = pfs_getattr,
    .mkdir = pfs_mkdir,
    .readdir = pfs_readdir,
    .rmdir = pfs_rmdir,
    .open = pfs_open,
    .create = pfs_create,
    .read = pfs_read,
};

int main(int argc, char *argv[]) {
	strcpy(mount_point, argv[4]);
	mountp_index = strlen(argv[4]);
	puts(mount_point);
	printf("some: %zu\n", mountp_index);
    return fuse_main(argc, argv, &pfs_oper, NULL);
}

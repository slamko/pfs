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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char mount_point[4096];
static size_t mountp_index;

struct file {
	char *name;
	char *contents;
	struct file *next;
};

struct file first = {.name = "first", .contents = "first file contents"};

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
    int res = 1;

    if (res == -1)
        return -errno;

    return 0;
}

static int pfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi) {
    (void)offset;
    (void)fi;

	if (strcmp(path, "/") == 0) {
		for (struct file *f = &first; f; f = f->next) {
			filler(buf, f->name, NULL, 0);
		}
	} else {
		return 1;
	}
	
	return 0;
}

int pfs_utimens(const char *path, const struct timespec tv[2]) {
	return 0;
}

static int pfs_create(const char *path, mode_t mode,
                      struct fuse_file_info *fi) {
    int res = 0;
	char p_delim[] = "/";
	struct file *f = NULL;
	char *fname = NULL;
	char *parse_tok = NULL;
	
	char *parse_path = strdup(path);
	for (f = &first; f->next; f = f->next);

	fname = strtok(parse_path, p_delim);
	parse_tok = fname;
	while (parse_tok) {
		fname = parse_tok;
		parse_tok = strtok(NULL, p_delim);
	}
	
	f->next = calloc(1, sizeof(*f->next));
	f->next->name = strdup(fname);

	free(parse_path);
    fi->fh = res;
    return 0;
}

static int pfs_open(const char *path, struct fuse_file_info *fi) {
    return pfs_create(path, 0, fi);
}

static int pfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    int fd;
    int res;

    if (fi == NULL)
        fd = open(path, O_RDONLY);
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
    .readdir = pfs_readdir,
    .open = pfs_open,
    .create = pfs_create,
	.utimens = pfs_utimens,
    .read = pfs_read,
};

int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &pfs_oper, NULL);
}

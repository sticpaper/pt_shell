#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/syscall.h>
#include <sys/xattr.h>
#include <getopt.h>
#include <linux/fs.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <linux/fiemap.h>

extern int errno;
extern char *optarg;
extern int optind, opterr, optopt;

typedef int fdesc;

int main (int argc, char *argv[])
{
	int ret = 0;
	if (argc < 2) {
		printf ("Usage: %s <file>\n", argv[0]);
		return -2;
	}
	char *filename = argv[argc - 1];
	fdesc fd = open (filename, O_RDONLY | O_NONBLOCK); //nonblock is advized by ioctl man
	if (fd == -1) {
		perror (filename);
		return -2;
	}
	struct stat st = {0};
	if (fstat (fd, &st)) {
		perror (filename);
		return -2;
	}
	struct statfs st_fs = {0};
	if (fstatfs (fd, &st_fs)) {
		perror (NULL);
		return -2;
	}
	printf ("fs type code is: %lx\n", (unsigned long) st_fs.f_type);
	blksize_t blksize = 0;
	printf ("stat blksize is: %lu\n", (unsigned long) st.st_blksize);
	printf ("statfs blksize is: %lu\n", (unsigned long) st_fs.f_bsize);
	if (!ioctl (fd, FIGETBSZ, &blksize))
			printf ("ioctl blksize is: %lu\n", (unsigned long) blksize);
	//will use fiemap for extracting extents info
	char buf[2048] = {0};
	struct fiemap *fiemap = (struct fiemap *) buf; // why that unobvious? becaue fm_extemts are not fiemap_extent * but fiemap_extent [0]! WHY? Bacause!
	fiemap->fm_extent_count = (sizeof (buf) - sizeof (*fiemap)) / sizeof (struct fiemap_extent);
	fiemap->fm_length = ~ (uint64_t) 0;
	fiemap->fm_start = 0;
	char file_not_empty = 0, header_printed = 0;
	printf ("file '%s':\n", filename);
	while (1) {
		ret = ioctl (fd, FS_IOC_FIEMAP, fiemap);
		if (ret < 0) {
			perror ("ioctl failed to get fiemap");
			close (fd);
			return -1;
		}
		if (fiemap->fm_mapped_extents)
			file_not_empty = 1;
		else {
			if (!file_not_empty)
				printf ("is empty\n");
			break;
		}
		if (!header_printed) {
			printf ("%-4s%-24s%-24s%-24s\n", "N", "logical offset, blk", "physical offset, blk", "extent length, blk");
			header_printed = 1;
		}
		unsigned long i = 0;
		for (i = 0; i < fiemap->fm_mapped_extents; i++) {
			static unsigned long ext_num = 0;
			printf ("%-4lu%-24llu%-24llu%-24llu\n", ext_num++, \
				fiemap->fm_extents[i].fe_logical / blksize,    \
				fiemap->fm_extents[i].fe_physical / blksize,   \
				fiemap->fm_extents[i].fe_length / blksize);
		}
		fiemap->fm_start = fiemap->fm_extents[i-1].fe_logical + fiemap->fm_extents[i-1].fe_length;
	}
	close (fd);
	return 0;
}
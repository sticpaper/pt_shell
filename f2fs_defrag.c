#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

struct f2fs_defragment {
    uint64_t start;
    uint64_t len;
};

#define F2FS_IOCTL_MAGIC 0xf5
#define F2FS_IOC_DEFRAGMENT _IOWR(F2FS_IOCTL_MAGIC, 8, struct f2fs_defragment)
#define ALIGNMENT_RULE 4096

int main(int argc, char *argv[])
{
    struct f2fs_defragment defrag = {
        .start = 0,
        .len = 0,
    };

    if (argc != 2) {
        printf("Usage: %s [filepath]\n", argv[0]);
        goto out;
    }

    const char *path = argv[1];
    int fd = open(path, O_RDWR | O_NOATIME);
    if (fd < 0)
        goto out;

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        goto out;
    }

    if (st.st_size % ALIGNMENT_RULE) {
        defrag.len = (uint64_t) st.st_size + (ALIGNMENT_RULE - (st.st_size % ALIGNMENT_RULE));
    } else {
        defrag.len = (uint64_t) st.st_size;
    }

    printf("[PAPER] F2FS File DEFRAG Tool\n");
    printf("[PAPER] Filename: [%s]\n", path);
    printf("[PAPER] Filerange: [%lu] Bytes\n", defrag.len);
    printf("[PAPER] Start DEFRAG Target File\n");

    if (ioctl(fd, F2FS_IOC_DEFRAGMENT, &defrag) < 0) {
        printf("[PAPER] DEFRAG Target File Error\n");
        close(fd);
        goto out;
    }

    printf("[PAPER] Target File DEFRAG Done\n");
    close(fd);
out:
    return 0;
}

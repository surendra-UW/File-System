#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "wfs.h"

int main(int argc, char *argv[])
{
    if(argc == 1) {
        printf("disk path is required argument\n");
        exit(1);
    } 

    char* disk_path = argv[1];
    // Open the file for read-write access
    int fd = open(disk_path, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    // Get the file size
    size_t file_size = lseek(fd, 0, SEEK_END);

    // Map the file into memory
    void *data = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    struct wfs_sb *super_block = (struct wfs_sb *)data;
    super_block->magic = WFS_MAGIC;
    super_block->head = (uint32_t)(uintptr_t)((char *)data + sizeof(struct wfs_sb));

    struct wfs_log_entry *root_dir = (struct wfs_log_entry *)((char *)data + sizeof(struct wfs_sb));
    root_dir->inode.inode_number = 0;
    root_dir->inode.size = 0;
    root_dir->inode.mode = S_IFDIR;
    super_block->head = (uint32_t)(uintptr_t)((char *)root_dir + INODE_SIZE);

    // Sync changes to the underlying file
    msync(data, file_size, MS_SYNC);

    // Unmap the memory
    munmap(data, file_size);

    // Close the file
    close(fd);

    printf("Super block and root directory created!\n");

    return 0;
}
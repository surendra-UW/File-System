#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "wfs.h"

char *disk_ptr;
size_t file_size;
char *inode_maps[MAX_INODES];

void build_inode_map1()
{
    struct wfs_sb *super_block = (struct wfs_sb *)disk_ptr;
    char *tail = (char *)disk_ptr + super_block->head;
    char *start = (char *)disk_ptr + sizeof(super_block);
    while (start < tail)
    {
        struct wfs_log_entry *log_entry = (struct wfs_log_entry *)start;
        struct wfs_inode in = log_entry->inode;
        inode_maps[in.inode_number] = (char *)start;
        start = start + INODE_SIZE + in.size;
    }
}

void perform_consistency_check() {
     struct wfs_sb *super_block = (struct wfs_sb *)disk_ptr;

     char *curr_node = disk_ptr + sizeof(struct wfs_sb);
     char *end_node = disk_ptr + super_block->head;
     char *tail = curr_node;

     while(curr_node<end_node) {
        struct wfs_log_entry *log_entry = (struct wfs_log_entry *)curr_node;
        int log_size = log_entry->inode.size+INODE_SIZE;
        if(inode_maps[log_entry->inode.inode_number] == (char *)log_entry) {
            if(tail != curr_node) {
                strncpy(tail, curr_node, log_size);
                inode_maps[log_entry->inode.inode_number] = tail;
                tail = tail + log_size;
            }
        }

        curr_node = curr_node + log_size;

        memset(tail, 0, file_size-(tail-disk_ptr));
     }

     super_block->head = (uint32_t) (tail-disk_ptr);


}

int main(int argc, char *argv[])
{

    // initailze inode maps
    for (int i = 0; i < MAX_INODES; i++)
    {
        inode_maps[i] = NULL;
    }

    if (argc < 2)
    {
        exit(1);
    }

    char diskpath[MAX_PATH_LEN];

    strcpy(diskpath, argv[1]);

    // if (realpath(argv[argc - 2], diskpath) == NULL) {
    //     perror("error extarcting disk absolute path");
    //     return -1;
    // }

    int fd = open(diskpath, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    // Get the file size
     file_size = lseek(fd, 0, SEEK_END);

    disk_ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    build_inode_map1();

    perform_consistency_check();

        // Sync changes to the underlying file
    msync(disk_ptr, file_size, MS_SYNC);

    // Unmap the memory
    munmap(disk_ptr, file_size);

    return 0;
}
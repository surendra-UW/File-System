#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "wfs.h"


static int my_getattr(const char *path, struct stat *stbuf) {
    // Implementation of getattr function to retrieve file attributes
    // Fill stbuf structure with the attributes of the file/directory indicated by path
    // ...

    return 0; // Return 0 on success
}

static struct fuse_operations my_operations = {
    .getattr = my_getattr,
    // Add other functions (read, write, mkdir, etc.) here as needed
};

int main(int argc, char *argv[])
{

    if (argc <= 3)
    {
        printf("disk path and mount are required\n");
        exit(1);
    }

    char disk_path[32]; 
    strcpy(disk_path, argv[argc - 2]);
    char mount_point[32];
    strcpy(mount_point, argv[argc - 1]);

    //deleting disk path from arguments to be passed to fuse
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    // Initialize FUSE with specified operations
    // Filter argc and argv here and then pass it to fuse_main
    return fuse_main(argc-1, argv, &my_operations, NULL);
}

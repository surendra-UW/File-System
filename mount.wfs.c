#define FUSE_USE_VERSION 30
#include <sys/mman.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "wfs.h"

char *inode_maps[MAX_INODES];

void* disk_ptr;
char root_dir[MAX_PATH_LEN];

static int my_getattr(const char *path, struct stat *stbuf) {
    // Implementation of getattr function to retrieve file attributes
    // Fill stbuf structure with the attributes of the file/directory indicated by path
    // ...

    return 0; // Return 0 on success
}


// static int my_open(const char *path){

//     char * path_name = malloc(strlen(path) + 1);
//     strcpy(path_name, path);

//     if(path_name[0] == '/'){
// 		path_name++;
// 	}
//     if(path_name[strlen(path_name)-1] == '/'){
// 		path_name[strlen(path_name)-1] = '\0';
// 	}

//     char* index;
//     int i = 0;
//     char curr_dir[MAX_PATH_LEN];
//     while(strlen(path_name) != 0){
// 		index = strchr(path_name, '/');
		
// 		if(index != NULL){
// 			strncpy(curr_dir, path_name, index - path_name);
// 			curr_dir[index-path_name] = '\0';

//             if (i==0){
//                 if (strcmp(curr_dir,root_dir)!=0)
//                 return -1;
//             }
			
// 			int flag = 0;
			
// 			return 0;
// 		}
// 		path_name = index+1;
//         i += 1; 
// 	}
// }


static struct fuse_operations my_operations = {
    .getattr = my_getattr,
    // .open =  my_open,
    // Add other functions (read, write, mkdir, etc.) here as needed
};

int main(int argc, char *argv[])
{

    //initailze inode maps 
    for (int i =0;i<MAX_INODES;i++) {
        inode_maps[i] = NULL;
    }

    if (argc <= 3)
    {
        printf("disk path and mount are required\n");
        exit(1);
    }

    char disk_path[MAX_PATH_LEN]; 
    strcpy(disk_path, argv[argc - 2]);
    strcpy(root_dir, argv[argc - 1]);

    int fd = open(disk_path, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    // Get the file size
    size_t file_size = lseek(fd, 0, SEEK_END);

    disk_ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    //deleting disk path from arguments to be passed to fuse
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;

    struct wfs_sb *super_block = (struct wfs_sb *) disk_ptr;
    printf("super block offset %d ", super_block->head);

    char *tail = (char *)disk_ptr + super_block->head ;
    char *start = (char *)disk_ptr+ sizeof(super_block);
    printf("start pointer %p end pointer %p\n", start , tail);

    while (start < tail)
    {
        struct wfs_log_entry *log_entry = (struct wfs_log_entry *) start;
        struct wfs_inode in = log_entry->inode;

        inode_maps[in.inode_number] = (char *)start;
        printf("inode %d inode offset is: %p mode is %u inode size %u\n", in.inode_number, inode_maps[in.inode_number], in.mode, in.size);
        // if (in.mode == S_IFDIR)
        // {
            int dire = in.size/sizeof(struct wfs_dentry);
            for(int i=0;i<dire ; i++) {
                struct wfs_dentry *dentry = (struct wfs_dentry *)(start+i*sizeof(struct wfs_dentry));
                printf("inode data %s inode numebr %lu parent inode %d\n", dentry->name,
                dentry->inode_number, in.inode_number);

            }

        // } else {  //if (in.mode == S_IFREG) 
             printf("inode data %s \n", log_entry->data);
        // }
        start = start + INODE_SIZE + in.size;
    }
    // Initialize FUSE with specified operations
    // Filter argc and argv here and then pass it to fuse_main
    return fuse_main(argc-1, argv, &my_operations, NULL);
}

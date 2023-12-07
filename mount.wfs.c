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

// void build_inode_map()
// {
//     struct wfs_sb *super_block = (struct wfs_sb *) disk_ptr;
//     printf("super block offset %d ", super_block->head);

//     char *tail = (char *)disk_ptr + super_block->head ;
//     char *start = (char *)disk_ptr+ sizeof(super_block);
//     printf("start pointer %p end pointer %p\n", start , tail);
//     printf("Sizeof log entry %u\n",(uint32_t)sizeof(struct wfs_log_entry));
//     printf("Sizeof wfs_inode %u\n",(uint32_t)sizeof(struct wfs_inode));
//     printf("Sizeof wfs_dentry %u\n",(uint32_t)sizeof(struct wfs_dentry));

//     while (start < tail)
//     {
//         struct wfs_log_entry *log_entry = (struct wfs_log_entry *) start;
//         struct wfs_inode in = log_entry->inode;

//         inode_maps[in.inode_number] = (char *)start;
//         printf("inode %d mode is %u inode size %u size of data %u\n", in.inode_number, in.mode, in.size, (uint32_t)strlen(log_entry->data));
//         if (in.mode & S_IFDIR)
//         {
//             printf("directory content is\n");
//             int dire = in.size/sizeof(struct wfs_dentry);
//             for(int i=0;i<dire ; i++) {
//                 struct wfs_dentry *dentry = (struct wfs_dentry *)(start+INODE_SIZE+i*sizeof(struct wfs_dentry));
//                 printf("inode data %s inode numebr %lu parent inode %d\n", dentry->name,
//                 dentry->inode_number, in.inode_number);

//             }

//         } else {
//              printf("file content is %s\n", log_entry->data);
//         }
//         start = start + INODE_SIZE + in.size;
//     }
// }

void build_inode_map()
{
    struct wfs_sb *super_block = (struct wfs_sb *) disk_ptr;
    char *tail = (char *)disk_ptr + super_block->head ;
    char *start = (char *)disk_ptr+ sizeof(super_block);
    while (start < tail)
    {
        struct wfs_log_entry *log_entry = (struct wfs_log_entry *) start;
        struct wfs_inode in = log_entry->inode;
        inode_maps[in.inode_number] = (char *)start;
        start = start + INODE_SIZE + in.size;
    }
}

int find_inode(const char *path)
{
    char * path_name = malloc(strlen(path) + 1);
    strcpy(path_name, path);

    if(path_name[0] == '/'){
		path_name++;
	}
    if(path_name[strlen(path_name)-1] == '/'){
		path_name[strlen(path_name)-1] = '\0';
	}

    char* index;
    int i = 0;
    char curr_dir[MAX_PATH_LEN];
    int inode = 0;

    while(strlen(path_name) != 0){
		index = strchr(path_name, '/');
		
		if(index != NULL){
			strncpy(curr_dir, path_name, index - path_name);
			curr_dir[index-path_name] = '\0';
             // i=0 mount dir skip it
		}
        else{
            strcpy(curr_dir, path_name);
        }
        if (i==0){
            
            if (strcmp(curr_dir,root_dir)!=0)
            return -1;
            path_name = index+1;
            i = 1;
            continue;
        }
        if(inode_maps[inode]==NULL)
        return -1;
        struct wfs_log_entry *log_entry = (struct wfs_log_entry *) inode_maps[inode];
        struct wfs_inode in = log_entry->inode;

        // log entry is of type dir?
        if (in.mode & S_IFREG)
        return -1;

        int n_dir = in.size/sizeof(struct wfs_dentry);
        int flag = 0;
        for(int j=0;j<n_dir ; j++) {
            struct wfs_dentry *dentry = (struct wfs_dentry *)(inode_maps[inode]+INODE_SIZE+j*sizeof(struct wfs_dentry));
            if(strcmp(dentry->name,curr_dir)==0)
            {
                flag = 1;
                inode = dentry->inode_number;
                break;
            }
        }

        if(flag==0)
        return -1;
        if(index==NULL)
        break;
		path_name = index+1;
	}
    return inode; 
}

static int my_open(const char *path, struct fuse_file_info *fi){
    
    int inode = find_inode(path);
    if(inode<0)
    return -1;
    return 0;
    
}



static int my_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
    int inode = find_inode(path);
    if(inode<0)
    return 0;
    
    memset(buf, 0, size);
    memcpy(buf,(char*)(inode_maps[inode] + offset), size);
    return strlen(buf);
}


static struct fuse_operations my_operations = {
    .getattr = my_getattr,
    .open =  my_open,
    .read = my_read,
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

    build_inode_map();
    // printf("%i\n",find_inode("mnt/dir1/file10"));

    //deleting disk path from arguments to be passed to fuse
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;

    
    // Initialize FUSE with specified operations
    // Filter argc and argv here and then pass it to fuse_main
    return fuse_main(argc-1, argv, &my_operations, NULL);
}

#define FUSE_USE_VERSION 30
#include <sys/mman.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "wfs.h"

int find_inode(const char *path);

char *inode_maps[MAX_INODES];

void *disk_ptr;

static int my_getattr(const char *path, struct stat *stbuf)
{
    // Implementation of getattr function to retrieve file attributes
    // Fill stbuf structure with the attributes of the file/directory indicated by path
    // ...
    int inode_number = find_inode(path);
    if (inode_number == -1)
        return -1;
    char *offset = inode_maps[inode_number];

    struct wfs_inode *inode = (struct wfs_inode *)offset;

    stbuf->st_uid = inode->uid;
    stbuf->st_gid = inode->gid;
    stbuf->st_mtime = inode->mtime;
    stbuf->st_mode = inode->mode;
    stbuf->st_nlink = inode->links;
    stbuf->st_size = inode->size;

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

int find_inode(const char *path)
{
    char *path_name = malloc(strlen(path) + 1);
    strcpy(path_name, path);

    if (path_name[0] == '/')
    {
        path_name++;
    }
    if (path_name[strlen(path_name) - 1] == '/')
    {
        path_name[strlen(path_name) - 1] = '\0';
    }

    char *index;
    char curr_dir[MAX_PATH_LEN];
    int inode = 0;

    while (strlen(path_name) != 0)
    {
        index = strchr(path_name, '/');

        if (index != NULL)
        {
            strncpy(curr_dir, path_name, index - path_name);
            curr_dir[index - path_name] = '\0';
        }
        else
        {
            strcpy(curr_dir, path_name);
        }

        if (inode_maps[inode] == NULL)
            return -1;

        struct wfs_log_entry *log_entry = (struct wfs_log_entry *)inode_maps[inode];
        struct wfs_inode in = log_entry->inode;

        if (in.mode & S_IFREG)
            return -1;

        int n_dir = in.size / sizeof(struct wfs_dentry);
        int flag = 0;
        for (int j = 0; j < n_dir; j++)
        {
            struct wfs_dentry *dentry = (struct wfs_dentry *)(inode_maps[inode] + INODE_SIZE) + j;
            if (strcmp(dentry->name, curr_dir) == 0)
            {
                flag = 1;
                inode = dentry->inode_number;
                break;
            }
        }

        if (flag == 0)
            return -1;
        if (index == NULL)
            break;
        path_name = index + 1;
    }
    return inode;
}

static int my_open(const char *path, struct fuse_file_info *fi)
{
    printf("my open operation %s\n", path);

    int inode = find_inode(path);
    printf("inode number %d\n", inode);
    if (inode < 0)
        return -1;
    return 0;
}

static int my_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("my read operation %s\n", path);

    int inode = find_inode(path);
    printf("inode number %d\n", inode);
    if (inode < 0)
        return 0;

    memset(buf, 0, size);
    memcpy(buf, (char *)(inode_maps[inode] + INODE_SIZE + offset), size);
    return strlen(buf);
}

int my_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    printf("my read dir operation %s\n", path);

    int inode = find_inode(path);
    printf("inode number %d\n", inode);

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    struct wfs_log_entry *log_entry = (struct wfs_log_entry *)inode_maps[inode];
    struct wfs_inode in = log_entry->inode;

    int n_dir = in.size / sizeof(struct wfs_dentry);
    for (int j = 0; j < n_dir; j++)
    {
        struct wfs_dentry *dentry = (struct wfs_dentry *)(inode_maps[inode] + INODE_SIZE + j * sizeof(struct wfs_dentry));
        printf(":%s:\n", dentry->name);
        filler(buffer, dentry->name, NULL, 0);
    }
    return 0;
}

int find_free_inode()
{
    int i = 0;
    while (i < MAX_INODES)
    {
        if (inode_maps[i] == NULL)
            return i;
        i++;
    }

    return -1;
}

char *get_filename_and_parent_path(char *path_name) {
    /*
         path_name variable is editable and used as placeholder to return path to parent
    */
   int j = strlen(path_name);
    if (path_name[j - 1] == '/')
    {
        path_name[j - 1] = '\0';
        j--;
    }

    char *file_name = strrchr(path_name, '/');
    if (file_name)
    {
        file_name = file_name + 1;
        path_name[file_name - path_name-1] = '\0';
    } else {
        file_name = malloc(j+1);
        strncpy(file_name, path_name, j);
        path_name[0] = '\0';
    }

    if(path_name[0] == '\0') {
        path_name = "/";
    }

    return file_name;
}

int check_file_exists(struct wfs_dentry *dir_entries, int dentry_count, char *file_name) {
    for(int i=0;i<dentry_count;i++) {
        if(strcmp(dir_entries, file_name) == 0)
            return 1;
        dir_entries = dir_entries+1;
    }
    return 0;
}

static int my_mkdir(const char *path, mode_t mode)
{
    printf("making directory %s and mode %d \n", path , (int) mode);
    int j = strlen(path);
    char *path_name = malloc(j + 1);
    strcpy(path_name, path);

    char *dir_name = get_filename_and_parent_path(path_name);

    //extract parent inode 
    int parent_inode_number = find_inode(path_name);
    if(parent_inode_number == -1) 
        return ENOENT;
    
    int new_inode_number = find_free_inode();
    if(new_inode_number == -1) {
        perror("max inode limit reached");
        return -1;
    }

    struct wfs_log_entry *parent = (struct wfs_log_entry *)inode_maps[parent_inode_number];
    size_t log_entry_size = parent->inode.size + INODE_SIZE;
    struct wfs_dentry *dentry = (struct wfs_dentry *)(parent+INODE_SIZE);
    int dentry_count = parent->inode.size/sizeof(struct wfs_dentry);

    //check if the directory already exists 
    if(check_file_exists(dentry, dentry_count, dir_name) == 1) 
        return EEXIST;
    
    //create new log entry for parent inode
    struct wfs_sb *super_block = (struct wfs_sb *)disk_ptr;
    struct wfs_log_entry *new_parent_log = (struct wfs_log_entry *)((char *)disk_ptr + super_block->head);

    memcpy((void *)new_parent_log, (void *)parent, log_entry_size);

    //add new dentry for the new directory
    struct wfs_dentry *new_directory_entry = (struct wfs_dentry *)((char *)new_parent_log + log_entry_size);
    strcpy(new_directory_entry->name, dir_name);
    new_directory_entry->inode_number = new_inode_number;
    new_parent_log->inode.size = parent->inode.size+ sizeof(struct wfs_dentry);

    //create log entry for new directory
    struct wfs_log_entry* new_directory_entry_log = (struct wfs_log_entry *) ((char *)new_parent_log 
                                        + INODE_SIZE + new_parent_log->inode.size);
    memset((void *)new_directory_entry_log, 0, sizeof(struct wfs_log_entry));


    //update inode map for the parent and sub directory
    inode_maps[new_inode_number] = (char *)new_directory_entry_log;
    inode_maps[parent->inode.inode_number] = (char *)new_parent_log;

    //set inode details for new directory
    new_directory_entry_log->inode.inode_number = new_inode_number;
    new_directory_entry_log->inode.mode = mode;
    new_directory_entry_log->inode.links = 1;

    //mark parent old log deleted
    parent->inode.deleted = 1;

    //update head of super block
    super_block->head = (char *)new_directory_entry_log - (char *)disk_ptr + INODE_SIZE;

    return 0;
}


static int my_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    
    printf("writing to a file %s\n", path);
    int inode_number = find_inode(path);
    if(inode_number == -1) {
        return ENOENT;
    }

    char *inode_ptr = inode_maps[inode_number];
    struct wfs_log_entry *prev_log_entry = (struct wfs_log_entry *)inode_ptr;

    //allocate new log entry
    struct wfs_sb *super_block = (struct wfs_sb *)disk_ptr;
    char *free_block = (char *)disk_ptr + super_block->head;
    struct wfs_log_entry *new_log_entry = (struct wfs_log_entry *)free_block;

    size_t log_entry_size = prev_log_entry->inode.size + INODE_SIZE;
    memcpy((void *)new_log_entry, (void *)prev_log_entry, log_entry_size);

    //write the contents and update the inode size
    strncpy((char *)new_log_entry->data+offset, buf, size);
    uint32_t new_size = offset+size;
    if(new_size > prev_log_entry->inode.size) {
        new_log_entry->inode.size = new_size;
    }

    //update old entry and inodemaps
    prev_log_entry->inode.deleted = 1;
    inode_maps[inode_number] = (char *)new_log_entry;

    super_block->head = (char *)new_log_entry-(char *)disk_ptr + new_log_entry->inode.size + INODE_SIZE;
    return size;
}

static int my_mknod(const char *path, mode_t mode, dev_t dev) {
    printf("making node %s with mode %d \n", path , (int) mode);
    int j = strlen(path);
    char *path_name = malloc(j + 1);
    strcpy(path_name, path);

    char *file_name = get_filename_and_parent_path(path_name);

    //extract parent inode 
    int parent_inode_number = find_inode(path_name);
    if(parent_inode_number == -1) 
        return ENOENT;
    
    //get new inode number 
    int new_inode_number = find_free_inode();
    if(new_inode_number == -1) {
        perror("max inode limit reached");
        return -1;
    }

    struct wfs_log_entry *parent = (struct wfs_log_entry *)inode_maps[parent_inode_number];
    size_t log_entry_size = parent->inode.size + INODE_SIZE;

    struct wfs_dentry *dentry = (struct wfs_dentry *)(parent+INODE_SIZE);
    int dentry_count = parent->inode.size/sizeof(struct wfs_dentry);

    //check if the directory already exists 
    if(check_file_exists(dentry, dentry_count, dir_name) == 1) 
        return EEXIST;
    
    //create new log entry for parent inode
    struct wfs_sb *super_block = (struct wfs_sb *)disk_ptr;
    struct wfs_log_entry *new_parent_log = (struct wfs_log_entry *)((char *)disk_ptr + super_block->head);

    memcpy((void *)new_parent_log, (void *)parent, log_entry_size);

    //add new dentry for the new directory
    struct wfs_dentry *new_directory_entry = (struct wfs_dentry *)((char *)new_parent_log + log_entry_size);
    strcpy(new_directory_entry->name, file_name);
    new_directory_entry->inode_number = new_inode_number;
    new_parent_log->inode.size = parent->inode.size+ sizeof(struct wfs_dentry);

    //create log entry for new directory
    struct wfs_log_entry* new_directory_entry_log = (struct wfs_log_entry *) ((char *)new_parent_log 
                                        + INODE_SIZE + new_parent_log->inode.size);
    memset((void *)new_directory_entry_log, 0, sizeof(struct wfs_log_entry));

    //update inode map for the parent and sub directory
    inode_maps[new_inode_number] = (char *)new_directory_entry_log;
    inode_maps[parent->inode.inode_number] = (char *)new_parent_log;

    //set inode details for new directory
    new_directory_entry_log->inode.inode_number = new_inode_number;
    new_directory_entry_log->inode.mode = mode;
    new_directory_entry_log->inode.links = 1;

    //mark parent old log deleted
    parent->inode.deleted = 1;

    //update head of super block
    super_block->head = (char *)new_directory_entry_log - (char *)disk_ptr + INODE_SIZE;

    return 0;
}

static struct fuse_operations my_operations = {
    .getattr = my_getattr,
    .readdir = my_readdir,
    .open = my_open,
    .read = my_read,
    .mkdir = my_mkdir,
    .write = my_write,
    .mknod = my_mknod,
    // Add other functions (read, write, mkdir, etc.) here as needed
};

int main(int argc, char *argv[])
{

    // initailze inode maps
    for (int i = 0; i < MAX_INODES; i++)
    {
        inode_maps[i] = NULL;
    }

    if (argc <= 3)
    {
        printf("disk path and mount are required\n");
        exit(1);
    }

    char diskpath[MAX_PATH_LEN];

    strcpy(diskpath, argv[argc - 2]);

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
    size_t file_size = lseek(fd, 0, SEEK_END);

    disk_ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    build_inode_map();

    argv[argc - 2] = argv[argc - 1];
    argc--;

    return fuse_main(argc, argv, &my_operations, NULL);
}

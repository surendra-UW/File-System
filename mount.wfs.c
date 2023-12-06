#define FUSE_USE_VERSION 30
#include <sys/mman.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "wfs.h"


void* disk_ptr;
char root_dir[MAX_PATH_LEN];

static int my_getattr(const char *path, struct stat *stbuf) {
    // Implementation of getattr function to retrieve file attributes
    // Fill stbuf structure with the attributes of the file/directory indicated by path
    // ...

    return 0; // Return 0 on success
}


static int my_open(const char *path){

    char * path_name = malloc(strlen(path) + 1);
    strcpy(path_name, path);

    void* temp_disk_ptr = malloc(sizeof(disk_ptr));
    memcpy(temp_disk_ptr,disk_ptr,sizeof(disk_ptr));

    if(path_name[0] == '/'){
		path_name++;
	}
    if(path_name[strlen(path_name)-1] == '/'){
		path_name[strlen(path_name)-1] = '\0';
	}

    char* ind;
    int i = 0;
    char curr_dir[MAX_PATH_LEN];
    while(strlen(path_name) != 0){
		ind = strchr(path_name, '/');
		
		if(index != NULL){
			strncpy(curr_dir, path_name, ind - path_name);
			curr_dir[ind-path_name] = '\0';

            if (i==0){
                if (strcmp(curr_dir,root_dir)!=0)
                return -1;
            }
			
			flag = 0;
			for(int i = 0; i < curr_node -> num_children; i++){
				if(strcmp((curr_node -> children)[i] -> name, curr_folder) == 0){
					curr_node = (curr_node -> children)[i];
					flag = 1;
					break;
				}
			}
			if(flag == 0)
				eturn NULL;
		}
		else{
			strcpy(curr_folder, path_name);
			flag = 0;
			for(int i = 0; i < curr_node -> num_children; i++){
				if(strcmp((curr_node -> children)[i] -> name, curr_folder) == 0){
					curr_node = (curr_node -> children)[i];
					return curr_node;
				}
			}
			return NULL;
		}
		path_name = index+1;
        i += 1; 
	}
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
    // Initialize FUSE with specified operations
    // Filter argc and argv here and then pass it to fuse_main
    return fuse_main(argc-1, argv, &my_operations, NULL);
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main () {
    
    char *path = "dir3";
    int j = strlen(path);
    char *path_name = malloc(j + 1);
    strcpy(path_name, path);

    if (path_name[j - 1] == '/')
    {
        path_name[j - 1] = '\0';
        j--;
    }

    char *dir_name = strrchr(path_name, '/');
    if (dir_name)
    {
        dir_name = dir_name + 1;
        path_name[dir_name - path_name-1] = '\0';
    } else {
        dir_name = malloc(j+1);
        strncpy(dir_name, path_name, j);
        path_name[0] = '\0';
    }

    printf("directory name %s\n", dir_name);

    // extract the path where directory has to be created

    if(path_name[0] == '\0') {
        path_name = "/";
    }

    printf("directory name %s\n", dir_name); 

     printf("path name %s\n", path_name); 
    return 0;
}
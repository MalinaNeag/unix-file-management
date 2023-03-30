#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

/* TO BE IMPROVED THE READING WITH SCANF 
if (scanf("-%9[^ \t\n\r\f\v]", options) != 1 && strchr(options, ' ')) {
        perror("Invalid input! Example of usage: -ndl (- followed by desired letters given in menu)\n");
        exit(2);
    }
If I want to use formatted input, it applied the options of the first arg to all the arguments
*/
// TO BE FIXED void printSymbolicLinkInfo--> it doesn't print anything before -l command; I have to change something in the implementation of this function 
int checkArguments(int argc) {
    if(argc < 2) {
        perror("Incorrect number of arguments! Usage: ./a.out <file/directory/link> ...\n");
        exit(1);
    }
    return 0;
}

void menu_RegularFiles(char *options){
    printf("\n----  MENU ----\n");
    printf("-n: name\n-d: size\n-h: hard link count\n-m: time of last modification\n-a: access rights\n-l: create symbolic link\n");
    printf("\nPlease, enter your options: ");
    scanf("%s", options);
}

void menu_Directory(char *options){
    printf("\n----  MENU ----\n");
    printf("-n: name\n");
    printf("\nPlease, enter your options: ");
    scanf("%s", options);
}

void menu_SymbolicLink(char *options){
    printf("\n----  MENU ----\n");
    printf("-n: name\n-l: delete symbolic link\n-d: size of symbolic link\n-t: size of target file\n-a: access rights\n");
    printf("\nPlease, enter your options: ");
    scanf("%s", options);
}

void printAccessRights(struct stat filestat) {
    
    printf("\nAccess rights:\n");

    printf("\nUSER:\n-read: %s\n-write: %s\n-execute: %s\n", 
           (filestat.st_mode & S_IRUSR) ? "yes" : "no",
           (filestat.st_mode & S_IWUSR) ? "yes" : "no",
           (filestat.st_mode & S_IXUSR) ? "yes" : "no");

    printf("\nGROUP:\n-read: %s\n-write: %s\n-execute: %s\n", 
           (filestat.st_mode & S_IRGRP) ? "yes" : "no",
           (filestat.st_mode & S_IWGRP) ? "yes" : "no",
           (filestat.st_mode & S_IXGRP) ? "yes" : "no");

    printf("\nOTHERS:\n-read: %s\n-write: %s\n-execute: %s\n", 
           (filestat.st_mode & S_IROTH) ? "yes" : "no",
           (filestat.st_mode & S_IWOTH) ? "yes" : "no",
           (filestat.st_mode & S_IXOTH) ? "yes" : "no");
}

void printRegularFileInfo(char *filepath, char *options) {
    
    struct stat filestat;
    
    if(lstat(filepath, &filestat) == -1) {
        printf("Error: %s\n", strerror(errno));
        return;
    }
    
    if (strchr(options, 'n') != NULL) {
        printf("\nName: %s\n", filepath);
    }
    if (strchr(options, 'm') != NULL) {
        printf("\nLast modified: %s", ctime(&filestat.st_mtime));
    }
    if (strchr(options, 'a') != NULL) {
        printAccessRights(filestat);
    }
    if (strchr(options, 'h') != NULL) {
        printf("\nHard link count: %ld\n", filestat.st_nlink);
    }
    if (strchr(options, 'd') != NULL) {
        printf("\nSize: %ld bytes\n", filestat.st_size);
    }
    if (strchr(options, 'l') != NULL) {
        printf("\nEnter name of symbolic link: ");
        char linkname[256];
        scanf("%s", linkname);
        if (symlink(filepath, linkname) == -1) {
            printf("\nError creating symbolic link: %s\n", strerror(errno));
        } else {
            printf("\nSymbolic link created: %s -> %s\n", linkname, filepath);
        }
    }
}

// TO BE FIXED --> it doesn't print anything before -l command; I have to change something in the implementation of this function 
void printSymbolicLinkInfo(char *linkpath, char *options) {
    
    struct stat filestat;
    
    if(lstat(linkpath, &filestat) == -1) {
        printf("Error: %s\n", strerror(errno));
        return;
    }

    if (strchr(options, 'n') != NULL) {
        printf("\nName: %s\n", linkpath);
    }

    if (strchr(options, 'l') != NULL) {
        if(unlink(linkpath) == -1) {
            printf("\nError: %s\n", strerror(errno));
        } else {
            printf("\nSymbolic link deleted.\n");
            return;
        }
    }

    if(strchr(options, 'd') != NULL && strchr(options, 'l') == NULL) {
        printf("\nSize of symbolic link: %ld bytes\n", filestat.st_size);
    }

    if(strchr(options, 't') != NULL && strchr(options, 'l') == NULL) {
        struct stat targetstat;
        if(stat(linkpath, &targetstat) == -1) {
            printf("Error: %s\n", strerror(errno));
            return;
        }
        if(S_ISREG(targetstat.st_mode)) {
            printf("\nSize of target file: %ld bytes\n", targetstat.st_size);
        } else {
            printf("\nTarget file is not a regular file.\n");
        }
    }

    if(strchr(options, 'a') != NULL && strchr(options, 'l') == NULL) {
        printAccessRights(filestat);
    }
}


void printArgumentsInfo(char *path) {
    
    struct stat filestat;
    char options[10];

    printf("\n%s", path);
    
    if(lstat(path, &filestat) == -1) {
        printf("Error: %s\n", strerror(errno));
        return;
    }

    switch (filestat.st_mode & S_IFMT) {
        case S_IFREG:
            printf("- REGULAR FILE\n");
            menu_RegularFiles(options);
            printRegularFileInfo(path, options);
            break;
        case S_IFDIR:
            printf(" - DIRECTORY\n");
            menu_Directory(options);
            break;
        case S_IFLNK:
            printf(" - SYMBOLIC LINK\n");
            menu_SymbolicLink(options);
            printSymbolicLinkInfo(path, options);
            break;
        default:
            printf("File type not supported.\n");
            break;
    }
}


int main(int argc, char *argv[]) {

    checkArguments(argc);

    for(int i = 1; i < argc; i++) {
        printArgumentsInfo(argv[i]);
    }
   
    return 0;
}
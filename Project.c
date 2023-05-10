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
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>

/*
TO BE FIXED until MONDAY:
-the printing of the menu 
-the printing to grades.txt, because the content is overwritten for each .C argument
*/


int checkArguments(int argc) {   
    if(argc < 2) {
        perror("Incorrect number of arguments! Usage: ./a.out <file/directory/link> ...\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

void menu_RegularFiles() {
    printf("\n----  MENU ----\n");
    printf("-n: name\n-d: size\n-h: hard link count\n-m: time of last modification\n-a: access rights\n-l: create symbolic link\n");
    printf("\nPlease, enter your options: ");
}

void menu_Directory() {
    printf("\n----  MENU ----\n");
    printf("-n: name\n-d: size\n-a: access rights\n-c: total number of files with .c extension\n");
    printf("\nPlease, enter your options: ");
}

void menu_SymbolicLink() {
    printf("\n----  MENU ----\n");
    printf("-n: name\n-l: delete symbolic link\n-d: size of symbolic link\n-t: size of target file\n-a: access rights\n");
    printf("\nPlease, enter your options: ");
}

void waitForChildren(){
    
    int status;
    pid_t awaited_child;

    awaited_child = wait(&status);

    while(awaited_child > 0){
        if(WIFEXITED(status)){
            printf("\nChild process with PID %d has terminated normally with code %d.\n\n", awaited_child, WEXITSTATUS(status));
        }
        else{
            printf("\nChild process with PID %d has terminated abnormally with code %d.\n\n", awaited_child, WEXITSTATUS(status));
        }

        awaited_child = wait(&status);
    }

}

void printAccessRights(struct stat st) {
    
    printf("\nAccess rights:\n");

    printf("\nUSER:\n-read: %s\n-write: %s\n-execute: %s\n", 
           (st.st_mode & S_IRUSR) ? "yes" : "no",
           (st.st_mode & S_IWUSR) ? "yes" : "no",
           (st.st_mode & S_IXUSR) ? "yes" : "no");

    printf("\nGROUP:\n-read: %s\n-write: %s\n-execute: %s\n", 
           (st.st_mode & S_IRGRP) ? "yes" : "no",
           (st.st_mode & S_IWGRP) ? "yes" : "no",
           (st.st_mode & S_IXGRP) ? "yes" : "no");

    printf("\nOTHERS:\n-read: %s\n-write: %s\n-execute: %s\n", 
           (st.st_mode & S_IROTH) ? "yes" : "no",
           (st.st_mode & S_IWOTH) ? "yes" : "no",
           (st.st_mode & S_IXOTH) ? "yes" : "no");

}

void createNewTxtFile(char* dirpath, struct stat st) {
    
    pid_t pid;
    
    if((pid = fork()) < 0) {
        perror("\nChild process creation failed.\n");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        char filename[1024];
        snprintf(filename, sizeof(filename), "%s/%s_file.txt", dirpath, dirpath);
        if (execvp("touch", (char *[]){ "touch", filename, NULL }) == -1) {
            perror("\nError: execvp failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    else if(pid > 0) {
        waitForChildren();
    }

}

void setPermissions(char* linkpath, struct stat* st) {
    
    pid_t pid;

    if((pid = fork()) < 0) {
        perror("\nChild process creation failed.\n");
        exit(EXIT_FAILURE);
    }

    if(pid == 0) {
        int check;
        check = execlp("chmod", "chmod", "u+rwx,g+rw-x,o-rwx", linkpath, NULL);
        if(check == -1){
            perror(strerror(errno));
            exit(errno);
        }
    }

    else if(pid > 0) {
        waitForChildren();
    }
}

void compileCFile(char* filepath, struct stat st, int fd) {
   
    pid_t pid;
    int pfd[2];

    int hasCExtension = strstr(filepath, ".c") != NULL;

    if(pipe(pfd) < 0) {
        perror("\nPipe creation failed.\n");
        exit(EXIT_FAILURE);
    }

    if((pid = fork()) < 0) {
        perror("\nChild process creation failed.\n");
        exit(EXIT_FAILURE);
    }

    else if(pid == 0) {

        close(pfd[0]);

        if(hasCExtension) {
            int check;
            dup2(pfd[1], 1);
            check = execlp("bash", "bash", "script.sh", filepath, NULL);
            if(check == -1){
                perror(strerror(errno));
                exit(errno);
            }   
        }

        if (!hasCExtension) {
            printf("\n");
            printf("(The argument is a regular file. In addition, the number of lines will be displayed: ");
            
            char command[100];
            char* filename = strtok(filepath, "\n");
            
            snprintf(command, sizeof(command), "wc -l %s", filename);

            FILE* wc_output = popen(command, "r");
            
            if (wc_output == NULL) {
                perror(strerror(errno));
                exit(errno);
            }

            int line_count = 0;

            if (fscanf(wc_output, "%d", &line_count) != 1) {
                perror(strerror(errno));
                exit(errno);
            }

            if (pclose(wc_output) == -1) {
                perror(strerror(errno));
                exit(errno);
            }

            printf("%d)\n", line_count);
        }
        exit(EXIT_SUCCESS);
    }

    else if(pid > 0){

        if(hasCExtension){
                        
            int errors = 0, warnings = 0, score=0;

            close(pfd[1]);
            
            FILE* stream = fdopen(pfd[0],"r");;
            
            fscanf(stream, "%d %d", &errors, &warnings);
            printf("Errors: %d\nWarnings: %d\n", errors, warnings);
            
            if(errors == 0 && warnings == 0){
                score = 10;
            }

            if(errors >= 1){
                score = 1;
            }

            if(errors == 0 && warnings > 10){
                score = 2;
            }

            if(errors == 0 && warnings <= 10){
                score = 2 + 8 * (10 - warnings) / 10;
            }
            
            if (fd == -1) {
                perror("Error opening file.\n");
                exit(EXIT_FAILURE);
            }
              
            char filename[1000];
            snprintf(filename, sizeof(filename), "%s", filepath);
            //dprintf(fd, "%s: %d\n", filename, score);
            //close(fd);
            //close(pfd[0]);
            char string[1024];
            sprintf(string, "%s : %d\n", filename, score);
            write(fd, string, strlen(string));
            close(fd);
            close(pfd[0]);
        }
        waitForChildren();
    }  
}

void printRegularFileInfo(char* filepath) {
    
    struct stat st;

    char options[20];
    int numberOfOptions, flag;
    
    do {
        if(scanf("%20s", options) != 1) {
            perror("Scanf failed.\n");
        }

        flag = 1;
        numberOfOptions = strlen(options);

        if (options[0] != '-' || numberOfOptions < 2) {
            printf("The input you provided is not valid! Please give '-', followed by a string consisting of options. Ex: -nd\n");
            menu_RegularFiles();
            flag = 0;
        }
        else {
            for (int i = 1; i < numberOfOptions; i++) {
                if (!strchr("nmahdl", options[i])) {
                    printf("The input you provided is not valid! Please give '-', followed by a string consisting of options. Ex: -nd\n");
                    flag = 0;
                    menu_RegularFiles();
                    break;
                }
            }
        }
    }while(!flag);

    if(lstat(filepath, &st) == -1) {
        printf("Error: %s\n", strerror(errno));
        return;
    }

    for(int i = 1; i < numberOfOptions; i++) {
        
        switch (options[i]) {
            
            case 'n':
                printf("\nName: %s\n", filepath);
                break;
            
            case 'm':
                printf("\nTime of last modification: %s", ctime(&st.st_mtime));
                break;
            
            case 'a':
                printAccessRights(st);
                break;
            
            case 'h':
                printf("\nHard link count: %ld\n", st.st_nlink);
                break;
            
            case 'd':
                printf("\nSize: %ld bytes\n", st.st_size);
                break;
            
            case 'l':
                printf("\nEnter name of symbolic link: ");
                char linkname[256];
                scanf("%s", linkname);
                
                if (symlink(filepath, linkname) == -1) {
                    printf("\nError creating symbolic link: %s\n", strerror(errno));
                } 
                else {
                    printf("\nSymbolic link created: %s -> %s\n", linkname, filepath);
                }
            
            default:
                break;
        }
    }
}

void printSymbolicLinkInfo(char* linkpath) {
    
    struct stat st;
    struct stat targetstat;

    char options[20];
    int numberOfOptions, flag;
    
    do {
        if(scanf("%20s", options) != 1) {
            perror("Scanf failed.\n");
        }
        flag = 1;
        numberOfOptions = strlen(options);

        if (options[0] != '-' || numberOfOptions < 2) {
            printf("The input you provided is not valid! Please give '-', followed by a string consisting of options. Ex: -nd\n");
            menu_SymbolicLink();
            flag = 0;
        }
        else {
            for (int i = 1; i < numberOfOptions; i++) {
                if (!strchr("ndtal", options[i])) {
                    printf("The input you provided is not valid! Please give '-', followed by a string consisting of options. Ex: -nd\n");
                    flag = 0;
                    menu_SymbolicLink();
                    break;
                }
            }
        }
    }while(!flag);
    
    if(lstat(linkpath, &st) == -1) {
        printf("Error: %s\n", strerror(errno));
        return;
    }

    for(int i = 1; i < numberOfOptions; i++) {
        
        switch (options[i]) {
        
        case 'n':
            printf("\nName: %s\n", linkpath);
            break;
        
        case 'd':
            printf("\nSize of symbolic link: %ld bytes\n", st.st_size);
            break;
        
        case 't':
            if(stat(linkpath, &targetstat) == -1) {
                printf("Error: %s\n", strerror(errno));
                return;
            }
            if(S_ISREG(targetstat.st_mode)) {
                printf("\nSize of target file: %ld bytes\n", targetstat.st_size);
            } else {
                printf("\nTarget file is not a regular file.\n");
            }
            break;

        case 'a':
            printAccessRights(st);
            break;
        
        case 'l':
            if(unlink(linkpath) == -1) {
                printf("\nError: %s\n", strerror(errno));
            } 
            else {
                printf("\nSymbolic link deleted. Other following options cannot be provided.\n");
                return;
            }
            break;
        
        default:
            break;
        }
    }
}

void printDirectoryInfo(char* dirpath) {

    char options[20];
    int numberOfOptions, flag;
    struct stat st;
    
    do {
        if(scanf("%20s", options) != 1) {
            perror("Scanf failed.\n");
        }
        flag = 1;
        numberOfOptions = strlen(options);

        if (options[0] != '-' || numberOfOptions < 2) {
            printf("The input you provided is not valid! Please give '-', followed by a string consisting of options. Ex: -nd\n");
            menu_Directory();
            flag = 0;
        }
        else {
            for (int i = 1; i < numberOfOptions; i++) {
                if (!strchr("ndac", options[i])) {
                    printf("The input you provided is not valid! Please give '-', followed by a string consisting of options. Ex: -nd\n");
                    flag = 0;
                    menu_Directory();
                    break;
                }
            }
        }
    }while(!flag);

    DIR *dir = opendir(dirpath);
    
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    for(int i = 1; i < numberOfOptions; i++) {
        
        switch (options[i]) {
            
            case 'n':
                printf("\nName: %s\n", dirpath);
                break;
            
            case 'd':
                if (stat(dirpath, &st) == -1) {
                    perror("Error getting file st");
                    return;
                }
                printf("\nSize: %ld bytes\n", st.st_size);
                break;
            
            case 'a':
                printAccessRights(st);
                break;

            case 'c': ;
                int count = 0;
                struct dirent *entry;
                while ((entry = readdir(dir)) != NULL) {
                    char path[1024];
                    snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
                    if (stat(path, &st) == -1) {
                        perror("Error getting file st");
                        return;
                    }
                    if (S_ISREG(st.st_mode) && strstr(entry->d_name, ".c") != NULL) {
                        count++;
                    }
                }
                printf("\nTotal number of files with .c extension: %d\n", count);
                break;
            
            default:
                break;
        }
    }
}

void printArgumentsInfo(char* path) {
    
    struct stat st;
   
    printf("\n%s", path);
    
    if(lstat(path, &st) == -1) {
        perror(strerror(errno));
        exit(errno);
    }

    switch (st.st_mode & S_IFMT) {
        
        case S_IFREG:
            printf("- REGULAR FILE\n");
            menu_RegularFiles();
            printRegularFileInfo(path);
            break;
        
        case S_IFDIR:
            printf(" - DIRECTORY\n");
            menu_Directory();
            printDirectoryInfo(path);
            break;
        
        case S_IFLNK:
            printf(" - SYMBOLIC LINK\n");
            menu_SymbolicLink();
            printSymbolicLinkInfo(path);
            break;
        
        default:
            printf("File type not supported.\n");
            break;
    }

}

void createProcesses(char* path) {
    
    struct stat st;
   // int fd = open("grades.txt", O_RDWR | O_CREAT, S_IRWXU);
     int fd = open("grades.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU );

    if(lstat(path, &st) == -1) {
        perror(strerror(errno));
        exit(errno);
    }

    switch (st.st_mode & S_IFMT) {
     
        case S_IFREG:
            compileCFile(path, st, fd);
            break;
     
        case S_IFDIR:
            createNewTxtFile(path, st);
            break;
     
        case S_IFLNK:
            setPermissions(path, &st);
            break;
     
        default:
            printf("File type not supported.\n");
            break;
    
    }
            
}

int main(int argc, char* argv[]) {
 
    pid_t pid;

    checkArguments(argc);

    for(int i = 1; i < argc; i++) {

        if((pid = fork()) < 0) {
            perror("\nFork failed.\n");
            exit(EXIT_FAILURE);
        } 
        
        else if(pid == 0) {
            printArgumentsInfo(argv[i]);
            printf("\n");
            exit(EXIT_SUCCESS);
        }
        
        else if(pid > 0) {          
            createProcesses(argv[i]);
            waitForChildren();            
        }
    }
    return 0;    
}



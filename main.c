#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BUFFER_LENGTH 128
#define DEBUG 1

long PATH_MAX;
char* CURRENT_PATH;
char CURRENT_DIR[64];
char SHELL_NAME[] = "myshell";

enum builtin_cmds {
    CD,
    CLR,
    DIR,
    ENVIRON,
    ECHO,
    HELP,
    QUIT
};

void update_directory_vars();
void input_loop();

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // User supplied a file
    }


    if(DEBUG) {
        printf("Initial vars:\n");
        printf("shell name: %s\n", SHELL_NAME);
        printf("curr path: %s\n", CURRENT_PATH);
        printf("curr dir: %s\n", CURRENT_DIR);
    }

}

void update_directory_vars() {
    static int initial = 1;
    if (initial) { // Initial update of vars
        PATH_MAX = pathconf(".",  _PC_PATH_MAX); // Get max path length based on os
        if (PATH_MAX == -1)
            PATH_MAX = 1024;
        else if (PATH_MAX > 10240)
            PATH_MAX = 10240;
        CURRENT_PATH = malloc(sizeof(char) * PATH_MAX);
    }
    if (getcwd(CURRENT_PATH, PATH_MAX) == NULL) {
        printf("Could not get curr directory path\n");
        exit(1);
    }
    char* token;
    if ((token = strrchr(CURRENT_PATH, '/')) == NULL) {
        printf("Invalid director path, cannot parse\n");
        exit(1);
    }
    int len = strlen(token);
    memcpy(CURRENT_DIR, token+1, len);

    initial = 0;
}
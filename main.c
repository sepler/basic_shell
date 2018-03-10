#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define DEBUG 1

size_t BUFFER_LENGTH = 128;
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
void parse_line(char*);

int main(int argc, char* argv[]) {
    if (argc == 2) {
        FILE* fp;
        char* line = NULL;
        size_t len = 0;
        if ((fp = fopen(argv[1], "r")) == NULL) {
            printf("could not read supplied file\n");
            exit(1);
        }
        while ((len = getline(&line, &BUFFER_LENGTH, fp)) != -1) {
            line[strcspn(line, "\n")] = 0; //trim trailing newline if it exists
            if (DEBUG) {
                printf("read line from file: %s\n", line);
            }
            parse_line(line);
        }
        fclose(fp);
        if (line)
            free(line);
    }

    update_directory_vars();

    if(DEBUG) {
        printf("Initial vars:\n");
        printf("shell name: %s\n", SHELL_NAME);
        printf("curr path: %s\n", CURRENT_PATH);
        printf("curr dir: %s\n", CURRENT_DIR);
    }

    input_loop();
}

void input_loop() {

}

void parse_line(char* line) {
    if (DEBUG) {
        printf("parsing line: %s\n", line);
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
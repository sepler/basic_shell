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

typedef struct token_vector {
    size_t length;
    char** arr;
} token_vector;

void update_directory_vars();
void input_loop();
void parse_line(char*);
token_vector tokenize(char*, char*);

int main(int argc, char* argv[]) {
    update_directory_vars();

    if (argc == 2) {
        FILE* fp;
        char* line = NULL;
        size_t len = 0;
        if ((fp = fopen(argv[1], "r")) == NULL) {
            printf("could not read supplied file\n");
            exit(1);
        }
        while ((len = getline(&line, &BUFFER_LENGTH, fp)) != -1) {
            line[strcspn(line, "\r\n")] = 0; //trim trailing newline if it exists
            if (DEBUG) {
                printf("read line from file: %s\n", line);
            }
            if (strlen(line) > 0) {
                parse_line(line);
            }
        }
        if (line)
            free(line);
        fclose(fp);
    }

    if(DEBUG) {
        printf("Initial vars:\n");
        printf("shell name: %s\n", SHELL_NAME);
        printf("curr path: %s\n", CURRENT_PATH);
        printf("curr dir: %s\n", CURRENT_DIR);
    }

    input_loop();
}

void input_loop() {
    while (1) {
        printf("%s/%s> ", CURRENT_PATH, SHELL_NAME);
        char* line = NULL;
        getline(&line, &BUFFER_LENGTH, stdin);
        line[strcspn(line, "\r\n")] = 0; //trim trailing newline if it exists
        if (strlen(line) > 0) {
            parse_line(line);
        }
        if (line)
            free(line);
    }
}

void parse_line(char* line) {
    if (DEBUG) {
        printf("parsing line: !%s!\n", line);
    }
    token_vector tokens = tokenize(line, " ");
    
    printf("tokens:\n");
    int i;
    for (i = 0; i < tokens.length; i++) {
        printf("%s\n", tokens.arr[i]);
    }

    if (tokens.arr != NULL)
        free(tokens.arr);
}

token_vector tokenize(char* line, char* token) {
    token_vector v;
    v.length = 1;
    v.arr = (char**) malloc(sizeof(char*) * v.length);
    char* p = strtok(line, token);
    int i = 0;
    while (p != NULL) {
        if (i == v.length) {
            if (realloc(v.arr, sizeof(char*) * ++v.length) == NULL) {
                printf("could not realloc mem during tokenize\n");
                exit(1);
            }
        }
        v.arr[i++] = p;
        p = strtok(NULL, token);
    }
    return v;
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
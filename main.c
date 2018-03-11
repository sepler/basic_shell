#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>

#define DEBUG 1

extern char **environ;

typedef struct token_vector {
    size_t length;
    char** arr;
} token_vector;

size_t BUFFER_LENGTH = 128;
long SHELL_PATH_MAX;
char* CURRENT_PATH;
char CURRENT_DIR[64];
char SHELL_NAME[] = "myshell";
token_vector PATHS;

struct Stdin {
    int true;
    char* src;
};
struct Stdout {
    int true;
    char* dest;
    int overwrite;
};
struct Pipe {
    int true;
    char* src;
    char* dest;
};

void update_directory_vars();
void input_loop();
void parse_line(char*);
token_vector tokenize(char*, char*);
void create_process(token_vector);

int main(int argc, char* argv[]) {
    update_directory_vars();

    PATHS = tokenize(getenv("PATH"), ":");

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
        printf("%s> ", CURRENT_PATH);
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

    if (DEBUG) {
        printf("tokens %lu:\n", tokens.length);
        int i;
        for (i = 0; i < tokens.length; i++) {
            printf("%s\n", tokens.arr[i]);
        }
        printf("\n");
    }
    if (strcmp(tokens.arr[0], "cd") == 0) {
        if (tokens.length > 1) {
            if (chdir(tokens.arr[1]) == -1) {
                printf("err: could not find directory!\n");
            } else {
                update_directory_vars();
            }
        }
    } else if (strcmp(tokens.arr[0], "clr") == 0) {
        int i;
        for (i = 0; i < 50; i++) {
            printf("\n");
        }
    } else if (strcmp(tokens.arr[0], "dir") == 0) {
        DIR *d;
        struct dirent *dir;
        if (tokens.length > 1) {
            d = opendir(tokens.arr[1]);
        } else {
            d = opendir(".");
        }
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                printf("%s\n", dir->d_name);
            }
            closedir(d);
        }
    } else if (strcmp(tokens.arr[0], "environ") == 0) {
        for (char **env = environ; *env; env++)
            printf("%s\n", *env);
    } else if (strcmp(tokens.arr[0], "echo") == 0) {
        int i;
        for (i = 1; i < tokens.length; i++) {
            printf("%s ", tokens.arr[i]);
        }
        printf("\n");
    } else if (strcmp(tokens.arr[0], "help") == 0) {
        printf("todo: write help\n");
    } else if (strcmp(tokens.arr[0], "quit") == 0 || strcmp(tokens.arr[0], "q") == 0) {
        exit(0);
    } else {
        int success = 0;
        if (access(tokens.arr[0], F_OK) != -1) {
            success = 1;
            create_process(tokens);
        } else {
            if (DEBUG)
                printf("paths: %lu\n", PATHS.length);
            int i;
            for (i = 0; i < PATHS.length; i++) {
                char buf[SHELL_PATH_MAX];
                snprintf(buf, sizeof(buf), "%s/%s", PATHS.arr[i], tokens.arr[0]);
                if (DEBUG)
                    printf("path: %s\n", buf);
                if (access(buf, F_OK) != -1) {
                    success = 1;
                    tokens.arr[0] = buf;
                    create_process(tokens);
                    break;
                }
            }
        }
        if (!success) {
            printf("err: could not locate binary: %s\n", tokens.arr[0]);
        }

    }
}

void create_process(token_vector tokens) {
    printf("LAUNCHING PROCESS %s", tokens.arr[0]);

    struct Stdin p_stdin = {.true = 0};
    struct Stdout p_stdout = {.true = 0, .overwrite = 1};
    struct Pipe p_pipe = {.true = 0};
    int p_background = 0;
    
    int i;
    for (i = 0; i < tokens.length; i++) {
        if (strcmp(tokens.arr[i], "<") == 0) {
            p_stdin.true = 1;
            if (i-1 < 0) {
                printf("err: invalid < params\n");
                return;
            }
            strncpy(p_stdin.src, tokens.arr[i-1], strlen(tokens.arr[i-1]));
        } else if (strcmp(tokens.arr[i], ">") == 0) {
            p_stdout.true = 1;
            if (i+1 >= tokens.length) {
                printf("err: invalid > params\n");
                return;
            }
            strncpy(p_stdout.dest, tokens.arr[i+1], strlen(tokens.arr[i+1]));
        } else if (strcmp(tokens.arr[i], ">>") == 0) {
            p_stdout.true = 1;
            p_stdout.overwrite = 0;
            if (i+1 >= tokens.length) {
                printf("err: invalid > params\n");
                return;
            }
            strncpy(p_stdout.dest, tokens.arr[i+1], strlen(tokens.arr[i+1]));
        } else if (strcmp(tokens.arr[i], "|") == 0) {
            p_pipe.true = 1;
            if (i+1 >= tokens.length || i-1 < 0) {
                printf("err: invalid | params\n");
                return;
            }
            strncpy(p_pipe.src, tokens.arr[i-1], strlen(tokens.arr[i-1]));
            strncpy(p_pipe.dest, tokens.arr[i+1], strlen(tokens.arr[i+1]));
        }
    }
    if (strcmp(tokens.arr[tokens.length-1], "&") == 0) {
        p_background = 1;
    }
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
        SHELL_PATH_MAX = pathconf(".",  SHELL_PATH_MAX); // Get max path length based on os
        if (SHELL_PATH_MAX == -1)
            SHELL_PATH_MAX = 1024;
        else if (PATH_MAX > 10240)
            SHELL_PATH_MAX = 10240;
        CURRENT_PATH = malloc(sizeof(char) * SHELL_PATH_MAX);
    }
    if (getcwd(CURRENT_PATH, SHELL_PATH_MAX) == NULL) {
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

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
  Function Declarations for builtin shell commands:
 */
int lsh_cd(char** args);
int lsh_pwd(char** args);
int lsh_help(char** args);
int lsh_echo(char** args);
int lsh_exit(char** args);
int lsh_touch(char** args);
int lsh_rm(char** args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char* builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pwd",
  "echo",
  "touch",
  "rm"
};

int (*builtin_func[]) (char**) = {
  &lsh_cd,
  &lsh_help,
  &lsh_exit,
  &lsh_pwd,
  &lsh_echo,
  &lsh_touch,
  &lsh_rm
};



int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char*);
}

/*
  Builtin function implementations.
*/




int lsh_cd(char** args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else {
        if (chdir(args[1]) != 0) {
            perror("lsh");
        }
    }
    return 1;
}

int lsh_help(char** args)
{
    int i;
    printf("Nikola Perica's LSH\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int lsh_exit(char** args)
{
    return 0;
}

int lsh_pwd(char** args)
{
    char* cwd;
    cwd = getcwd(NULL, 0);
    printf("%s\n", cwd);
    return 1;
}

int lsh_echo(char** args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "lsh: expected argument to echo\n");
    }
    else {
        printf("%s \n", args[1]);
    }
    return 1;
}

int lsh_touch(char** args)
{
    FILE* newFile;
    newFile = fopen(args[1], "w");
    fclose(newFile);
    return 1;
}

int lsh_rm(char** args)
{
    int result = remove(args[1]);
    if (result != -1) {
        return 1;
    }
    printf("Couldn't delete file");
    return 1;
}

int lsh_launch(char** args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) {
        // Error forking
        perror("lsh");
    }
    else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int lsh_execute(char** args)
{
    int i;

    if (args[0] == NULL) {
        // An empty command was entered.
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}



#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char** lsh_split_line(char* line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char** tokens = malloc(bufsize * sizeof(char*));
    char* token;

    if (!tokens) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

#define LSH_RL_BUFSIZE 1024
char* lsh_read_line(void)
{
    char* line = NULL;
    ssize_t bufsize = 0; // have getline allocate a buffer for us

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);  // We recieved an EOF
        }
        else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

void lsh_loop(void)
{
    char* line;
    char** args;
    int status;

    do {
        printf("> ");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char** argv)
{
    // Load config files, if any.

    // Run command loop.
    lsh_loop();

    // Perform any shutdown/cleanup.

    return EXIT_SUCCESS;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_LEN 512
#define DELIM " \n\r\t"


struct Alias {
    char *alias_name;
    char **replacement;
    struct Alias *next;
    struct Alias *prev;
};

struct Alias *head = NULL;
struct Alias *tail = NULL;

char *getCommand() {
    char *command = NULL;
    size_t size;
    ssize_t n_read = getline(&command, &size, stdin);
    if (n_read < 0) {
        if (feof(stdin))
            exit(0);
        exit(1);
    }
    return command;
}

char **getArgs(char *command) {
        char *cm = strdup(command);
        char **args = malloc(100 * sizeof(char*));
        char *arg;
        arg = strtok(cm, DELIM);
        int i = 0;
        while (arg != NULL) {
            args[i] = arg;
            i++;
            arg = strtok(NULL, DELIM);
        }
        args[i] = NULL;
        return args;
}



char **getArgs_redirect(char *command) {
    char *cm = strdup(command);
    char **args = malloc(100 * sizeof(char*));
    char *arg;
    arg = strtok(cm, ">");
    int i = 0;
    while (arg != NULL) {
        args[i] = arg;
        i++;
        arg = strtok(NULL, ">");
    }
    args[i] = NULL;
    return args;
}


int redirect(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        i++;
    }
    if (i != 2) {
        write(STDERR_FILENO, "Redirection misformatted.\n",
            strlen("Redirection misformatted.\n"));
        return -1;
    }
    char **args_command;
    char **args_file;
    args_command = getArgs(args[0]);
    args_file = getArgs(args[1]);
    int j = 0;
    while (args_file[j] != NULL) {
        j++;
    }
    if (j != 1) {
        write(STDERR_FILENO, "Redirection misformatted.\n",
             strlen("Redirection misformatted.\n"));
        return -1;
    }
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) {
        FILE *f = fopen(args_file[0], "w+");
        dup2(fileno(f), fileno(stdout));
        if (execv(args_command[0], args_command) < 0) {
            write(STDERR_FILENO, args_command[0], strlen(args_command[0]));
            write(STDERR_FILENO, ": Command not found.\n",
                 strlen(": Command not found.\n"));
            _exit(1);
        }
    } else {
        waitpid(pid, &status, 0);
    }
    return 0;
}

struct Alias *searchAlias(char *name) {
    struct Alias *temp = NULL;
    temp = head;
    while (temp != NULL) {
        if (strcmp(temp->alias_name, name) == 0) {
            return temp;
        } else {
            temp = temp->next;
        }
    }

    return NULL;
}

int alias(char **args) {
    struct Alias *temp = NULL;
    int i = 0;
    while (args[i] != NULL) {
        i++;
    }
    // args contain only "alias"
    if (i == 1) {
        temp = head;
        while (temp != NULL) {
            write(STDOUT_FILENO, temp->alias_name, strlen(temp->alias_name));
            write(STDOUT_FILENO, " ", strlen(" "));
            int j = 0;
            while (temp->replacement[j] != NULL) {
                write(STDOUT_FILENO, temp->replacement[j],
                    strlen(temp->replacement[j]));
                if (temp->replacement[j + 1] != NULL)
                    write(STDOUT_FILENO, " ", strlen(" "));
                j++;
            }
            write(STDOUT_FILENO, "\n", strlen("\n"));
            temp = temp->next;
        }
        return 0;
    }
    // only "alias" + alias_name
    temp = searchAlias(args[1]);
    if (i == 2) {
        if (temp != NULL) {
            write(STDOUT_FILENO, temp->alias_name, strlen(temp->alias_name));
            write(STDOUT_FILENO, " ", strlen(" "));
            int m = 0;
            while (temp->replacement[m] != NULL) {
                write(STDOUT_FILENO, temp->replacement[m],
                     strlen(temp->replacement[m]));
                if (temp->replacement[m + 1] != NULL) {
                    write(STDOUT_FILENO, " ", strlen(" "));
                } else {
                    write(STDOUT_FILENO, "\n", strlen("\n"));
                }
                m++;
            }
        }
        return 0;
    }
    // alias + alias_name + replacement
    if ( (strcmp(args[1], "alias") == 0) ||
         (strcmp(args[1], "unalias") == 0) ||
          (strcmp(args[1], "exit") == 0)) {
        write(STDERR_FILENO, "alias: Too dangerous to alias that.\n",
             strlen("alias: Too dangerous to alias that.\n"));
        return -1;
    }

    char **replacement;
    replacement = malloc(100 * sizeof(char*));
    int a = 0, b = 2;
    while (args[b] != NULL) {
        replacement[a] = args[b];
        a++;
        b++;
    }
    replacement[a] = NULL;
    if (temp != NULL) {
        temp->replacement = replacement;
        return 0;
    }
    struct Alias *newAlias = NULL;
    newAlias = (struct Alias*)malloc(sizeof(struct Alias));
    newAlias->alias_name = args[1];
    newAlias->replacement = replacement;
    newAlias->next = NULL;

    if (head == NULL) {
        newAlias->prev = NULL;
        head = newAlias;
        tail = head;
    } else {
        newAlias->prev = tail;
        tail->next = newAlias;
        tail = tail->next;
    }
    return 0;
}

int unalias(char **args) {
    int i = 0;
    while (args[i] != NULL) {
        i++;
    }
    if (i != 2) {
        write(STDERR_FILENO, "unalias: Incorrect number of arguments.\n",
         strlen("unalias: Incorrect number of arguments.\n"));
        return -1;
    }
    struct Alias *temp = NULL;
    temp = searchAlias(args[1]);
    if (temp != NULL) {
        free(temp->replacement);
        if (head == tail) {
            head = NULL;
            tail = NULL;
        } else if (temp == head) {
            head = temp->next;
            head->prev = NULL;
        } else if (temp == tail) {
            tail = temp->prev;
            tail->next = NULL;
        } else {
            temp->next->prev = temp->prev;
            temp->prev->next = temp->next;
        }
        free(temp);
    }
    return 0;
}



void execute(char **args) {
    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) {
        if (execv(args[0], args) < 0) {
            write(STDERR_FILENO, args[0], strlen(args[0]));
            write(STDERR_FILENO, ": Command not found.\n",
             strlen(": Command not found.\n"));
            _exit(1);
        }
    } else {
        waitpid(pid, &status, 0);
    }
}

int process(char **args, char *command) {
    // empty command
    if (strspn(command, DELIM) == strlen(command)) {
        // free(command);
        return 0;
    }
    // redirect
    int count = 0, i = 0;
    while (command[i] != '\0') {
        if (command[i] == '>')
            count++;
        i++;
    }
    if (count > 1) {
        write(STDERR_FILENO, "Redirection misformatted.\n",
         strlen("Redirection misformatted.\n"));
        // free(command);
        return -1;
    } else if (count == 1) {
        args = getArgs_redirect(command);
        redirect(args);
        // free(command);
        //free(args);
        return 1;
    }

    args = getArgs(command);
    // exit
    if (strcmp(args[0], "exit") == 0) {
        free(args);
        // free(command);
        exit(0);
    }

    // alias
    if (strcmp(args[0], "alias") == 0) {
        alias(args);
        // free(args);
        return 0;
    }

    // unalias
    if (strcmp(args[0], "unalias") == 0) {
        unalias(args);
        // free(command);
        // free(args);
        return 0;
    }

    // if execute alias
    struct Alias *temp = NULL;
    temp = searchAlias(args[0]);
    if (temp != NULL) {
        execute(temp->replacement);
        // free(command);
        // free(args);
        return 0;;
    }

    // non-builtin
    execute(args);
    // free(command);
    // free(args);
    return 0;
}

int main(int argc, char* argv[]) {
    FILE *fp;
    char *command = NULL;
    char **args = NULL;
    if (argc == 2) {
        if (!(fp = fopen(argv[1], "r"))) {
            write(STDERR_FILENO, "Error: Cannot open file ",
             strlen("Error: Cannot open file "));
            write(STDERR_FILENO, argv[1], strlen(argv[1]));
            write(STDERR_FILENO, ".\n", strlen(".\n"));
            exit(1);
        }
        while (1) {
            size_t size;
            ssize_t n_read = getline(&command, &size, fp);
            if (n_read < 0) {
                if (feof(fp)) {
                    fclose(fp);
                    exit(0);
                }
                exit(1);
            }
            if (strlen(command) > MAX_LEN) {
                continue;
            }
            write(STDOUT_FILENO, command, strlen(command));
            process(args, command);
            free(args);
        }

    } else if (argc < 1 || argc >2) {
        write(STDERR_FILENO, "Usage: mysh [batch-file]\n",
         strlen("Usage: mysh [batch-file]\n"));
        exit(1);
    }

    while (1) {
        write(STDOUT_FILENO, "mysh> ", strlen("mysh> "));
        command = getCommand();
        if (strlen(command) > MAX_LEN) {
            continue;
        }
        process(args, command);
        free(args);
    }
}

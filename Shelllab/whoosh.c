/* This is the main file for the `whoosh` interpreter and the part
   that you modify. */

#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
#include "ast.h"
#include "fail.h"

static void run_script(script *scr);

static void run_group(script_group *group);

static void run_command(script_command *command, int flag);

static void set_var(script_var *var, int new_value);

static void run_and_command(script_command *command);

static void set_status_to_list(int status);


//static volatile int pid = 0;
static int group_mode;

#define VAR 50

static script_var *var_list[VAR];

static int var_index = 0;

static int array_index = 0;
static volatile int pid;


typedef struct {
    int *array;
    size_t used;
    size_t size;
} Array;

static Array array;

static Array continued_list;

static void initArray(Array *a, size_t initialSize) {
    a->array = (int *) malloc(initialSize * sizeof(int));
    a->used = 0;
    a->size = initialSize;
}

static void insertArray(Array *a, int element) {
    if (a->used == a->size) {
        a->size *= 2;
        a->array = (int *) realloc(a->array, a->size * sizeof(int));
    }
    a->array[a->used++] = element;
}

static void freeArray(Array *a) {
    free(a->array);
    a->array = NULL;
    a->used = a->size = 0;
}


/* You probably shouldn't change main at all. */

int main(int argc, char **argv) {
    script *scr;

    if ((argc != 1) && (argc != 2)) {
        fprintf(stderr, "usage: %s [<script-file>]\n", argv[0]);
        exit(1);
    }

    scr = parse_script_file((argc > 1) ? argv[1] : NULL);

    run_script(scr);

    return 0;
}


static void sigint_handler(int sig) {
    int status;
    int i;
    for (i = 0; i < array.used; i++) {
        if (array.array[i] != -1)
            Kill(array.array[i], SIGTERM);
    }
    Waitpid(pid, &status, WNOHANG);//Wait to compeleted
//    if (pid != 0) {
////        if(sig == SIGINT) {
//        Kill(-pid, SIGTERM);
////        }
////        else if (sig == SIGCONT){
////            Kill(-pid, SIGCONT);
////        }
//
//    }

    return;
}

//static void sigchld_handler(int sig) {
//    int status;
//    pid_t pid;
//    while ((pid = Waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
//        if (WIFSTOPPED(status)) {
//            //Terminate
//        } else if (WIFCONTINUED(status)) {
//            Kill(-pid, SIGCONT);//Not sure
//            //keep going
//        } else {
//            //sio_puts()
//        }
//    }
//
//    return;
//}
//
//static void sigstp_handler(int sig) {
//    pid_t pid;
//    Waitpid(pid, NULL, 0);
//    Kill(-pid, SIGTSTP);
//    return;
//}

static void handle_or(script_group *group, int flag) {
    int i, status, to_delete;
    sigset_t mask;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGINT);
    Sigprocmask(SIG_BLOCK, &mask, NULL);


    int pid_list[2 * (group->num_commands)];
    for (i = 0; i < group->num_commands; i++) {
        if ((pid_list[i] = Fork()) == 0) {
            insertArray(&array, pid_list[i]);
            if (group->commands[i].pid_to != NULL) {
                var_list[var_index] = group->commands[i].pid_to;
                set_var(var_list[var_index], pid_list[i]);
                var_index++;
            }
            Setpgid(0, 0);
            run_and_command(&group->commands[i]);
        }
    }

    //  Sigprocmask(SIG_UNBLOCK, &mask, NULL);
    int first_pid = Waitpid(-1, &status, 0);
    if(WUNTRACED == WIFSTOPPED(status))
    {
        freeArray(&continued_list);
        insertArray(&continued_list, first_pid);
    }
    // int lenth = VAR;
    for (i = 0; i < group->num_commands; i++) {
        if (pid_list[i] != first_pid) {
            Kill(pid_list[i], SIGTERM);
           to_delete = Waitpid(pid_list[i], NULL, 0);
           // printf("%d\n", to_delete);
//            for (i = 0; i < array.used; i++) {
//                if (array.array[i] == to_delete)
//                    array.array[i] = -1;
//            }
        }
    }
    if (flag) {
        set_status_to_list(status);
    }

    //  return;

//    sigset_t mask;
//    Sigemptyset(&mask);
//    Sigaddset(&mask, SIGCHLD);
//    //  int num_comds = 0;
//    //  signal(SIGCHLD, sigchld_handler);
//    char **command = malloc(sizeof(argv));
//    int j = 0, k = 0;
////    while (argv[l] != NULL) {
////        if (strcmp(argv[l], "||") == 0) {
////            num_comds++;
////        }
////        l++;
////    }
//    //   num_comds++;
//    int end = 0;
//    while (argv[j] != NULL && end != 1) {
//        k = 0;
//        while (strcmp(argv[j], "||") != 0) {
//            command[k] = argv[j];
//            j++;
//            if (argv[j] == NULL) {
//                end = 1;
//                k++;
//                break;
//            }
//            k++;
//        }
//        command[k] = NULL;
//        j++;
//        Sigprocmask(SIG_BLOCK, &mask, NULL);
//        pid = fork();
//
//        Setpgid(0, 0);
//        if (pid < 0) {
//           // printf("Child process not created\n");
//            unix_error("Child process not created\\n");
//            return;
//        }
//        if (pid == 0) {
//            Sigprocmask(SIG_UNBLOCK, &mask, NULL);
//            if(execve(command[0], command, environ) < 0)
//            {
//                Kill(-pid, SIGTERM);
//                continue;
//            }
//          //  Execve(command[0], command, environ);
//        }
//        Sigprocmask(SIG_UNBLOCK, &mask, NULL);
//
//        Waitpid(pid, NULL, 0);

//    }
}

/**Helper method for Dup2 redirect**/
static void redirect(int oldfd, int newfd) {
    if (oldfd != newfd) {
        if (Dup2(oldfd, newfd) != -1)
            Close(oldfd); /* successfully redirected */
        else
            perror("Redirect Error");
    }
}

static void handle_pipe(script_group *group, int in_fd, size_t pos, int flag) {
    int i, to_delete;
    sigset_t mask;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGINT);

    if (pos + 1 == group->num_commands) {
        Sigprocmask(SIG_UNBLOCK, &mask, NULL);
        redirect(in_fd, STDIN_FILENO);
        run_and_command(&group->commands[pos]);
        int status;
        to_delete = Wait(&status);
        if(WUNTRACED == WIFSTOPPED(status))
        {
            freeArray(&continued_list);
            insertArray(&continued_list, to_delete);
        }
//        for (i = 0; i < array.used; i++) {
//            if (array.array[i] == to_delete)
//                array.array[i] = -1;
//        }

        if (flag) {

            set_status_to_list(status);
//
////            return WEXITSTATUS(status);
//            printf("%d\n", WEXITSTATUS(status));
        }

        return;
    } else {
        int fds[2];
        Pipe(fds);
//        //pid_t pid;
//        if (Pipe(fds) < 0) {
//            perror("Pipe error");
//            return;
//        }
        Sigprocmask(SIG_BLOCK, &mask, NULL);
        pid = Fork();
        freeArray(&array);
        insertArray(&array, pid);
        if (group->commands[pos].pid_to != NULL) {
            var_list[var_index] = group->commands[pos].pid_to;
            set_var(var_list[var_index], pid);
            var_index++;
        }
        Setpgid(0, 0);
        switch (pid) {
            case -1:
                perror("no child process");
                return;
            case 0:
                Close(fds[0]);
                redirect(in_fd, STDIN_FILENO);
                redirect(fds[1], STDOUT_FILENO);
                run_and_command(&group->commands[pos]);
            default:
                Close(fds[1]);
                Close(in_fd);
                handle_pipe(group, fds[0], pos + 1, flag);
        }
    }

//    int fds[2];
//    pid_t pid;
//    if(pos == group->num_commands - 1)
//    {
//        run_command(&group->commands[pos]);
//        return;
//    }
//    pipe(fds);
//    if((pid = fork()) == 0)
//    {
//        Dup2(fds[0], 0);
//        Close(fds[1]);
//        handle_pipe(group, 0, pos + 1);
//    }
//    Dup2(fds[1], 1);
//    Close(fds[0]);
//    run_command(&group->commands[pos]);



    /**Get rid of them, maybe re-implement them in the future**/
////  const char** argv;
////  argv = malloc(sizeof(char*) * (command->num_arguments + 2));
//    int fds[2];
//    int fds2[2];
//    int num_comds = group->num_commands;
//   // char **command = malloc(sizeof(argv));
//    pid_t pid;
//    int end = 0;
//    int i = 0, j = 0, k = 0, l = 0;
////    while (argv[l] != NULL) {
////        if (strcmp(argv[l], "|") == 0) {
////            num_comds++;
////        }
////        l++;
////    }
////    num_comds++;
//    const char **argv;
//    printf("%d\n", num_comds);
//
//    //while (i < num_comds) {
//        argv = malloc(sizeof(char *) * (group->commands[i].num_arguments + 2));
//        argv[0] = group->commands[i].program;
//
//        for (i = 0; i < group->commands[i].num_arguments; i++) {
//            if (group->commands[i].arguments[i].kind == ARGUMENT_LITERAL)
//                argv[i + 1] = group->commands[i].arguments[i].u.literal;
//            else
//                argv[i + 1] = group->commands[i].arguments[i].u.var->value;
//        }
//        argv[group->commands[i].num_arguments + 1] = NULL;
//        int p;
////        for(p = 0; p < sizeof(argv); p++)
////        {
//            printf("%s\n", argv[0]);
//       // }
////        printf("%s\n", argv[0]);
////        j++;
////        if (i % 2 != 0) {
////            pipe(fds);
////        } else {
////            pipe(fds2);
////        }
////        pid = fork();
////       // setpgid(0,0);
////        if (pid < 0) {
////            if (i != num_comds - 1) {
////                if (i % 2 != 0) {
////                    Close(fds[1]);
////                } else {
////                    Close(fds2[1]);
////                }
////
////            }
////            printf("Child process not created\n");
////            return;
////        }
////        if (pid == 0) {
////            if (i == 0) {
////                Dup2(fds2[1], STDOUT_FILENO);
////            } else if (i == num_comds - 1) {
////                if (num_comds % 2 != 0) {
////                    Dup2(fds[0], STDIN_FILENO);
////                } else {
////                    Dup2(fds2[0], STDIN_FILENO);
////                }
////
////            } else {
////                if (i % 2 != 0) {
////                    Dup2(fds2[0], STDIN_FILENO);
////                    Dup2(fds[1], STDOUT_FILENO);
////                } else {
////                    Dup2(fds[0], STDIN_FILENO);
////                    Dup2(fds2[1], STDOUT_FILENO);
////                }
////
////            }
////
////
////            //Execve(command[0], command, environ);
////            Execve(argv[0], argv, environ);
//////                kill(getpid(), SIGTERM);
//////            }
////
////        }
////        if (i == 0) {
////            Close(fds2[1]);
////        } else if (i == num_comds - 1) {
////            if (num_comds % 2 != 0) {
////                Close(fds[0]);
////            } else {
////                Close(fds2[0]);
////            }
////        } else {
////            if (i % 2 != 0) {
////                Close(fds2[0]);
////                Close(fds[1]);
////            } else {
////                Close(fds[0]);
////                Close(fds2[1]);
////            }
////
////        }
//////        int status;
//////        if (Wait(&status) > 0) {
//////            if (WIFEXITED(status) != 0) {
////////            return WEXITSTATUS(status);
//////                //        printf("%d\n", WEXITSTATUS(status));
//////                return;
//////            }
//////        }
////        int status;
////        while( Wait(&status) != -1){}
////
////
////        i++;
//   // }
}

//
//static void run_pipe(script_group *group) {
//    Signal(SIGCHLD, sigchld_handler);
//    Signal(SIGINT, sigint_handler);
//    Signal(SIGTSTP, sigstp_handler);
//    sigset_t mask, empty_mask;
//
//    Sigemptyset(&mask);
//    Sigemptyset(&empty_mask);
//    Sigaddset(&mask, SIGCHLD);
//    Sigprocmask(SIG_BLOCK, &mask, NULL);
//
//
//    pid_t pid = fork();
//    setpgid(0,0);
//    int status;
//    if (pid == 0) {
//        Sigprocmask(SIG_UNBLOCK, &mask, NULL);
//        handle_pipe(group, STDIN_FILENO, 0);
//    }
//    Sigsuspend(&empty_mask);
//    Waitpid(pid, &status, 0);
//}


static void run_script(script *scr) {
    Signal(SIGINT, sigint_handler);

    if (scr->num_groups == 1) {
        run_group(&scr->groups[0]);
    } else {
        /* You'll have to make run_script do better than this */
        // fail("only 1 group supported");
        int i;
        for (i = 0; i < scr->num_groups; i++) {
            run_group(&scr->groups[i]);
            // printf("www");
            freeArray(&array);
        }
        for(i = 0; i < continued_list.used; i++)
        {
            Kill(continued_list.array[i], SIGTSTP);
            Waitpid(continued_list.array[i], NULL, 0);

        }


    }
}


static void run_group(script_group *group) {
    /* You'll have to make run_group do better than this, too */
    group_mode = group->mode;
    int flag = 0;
    int repeat = 1;
    if (group->repeats != 1) {
        // fail("only repeat 1 supported");
        repeat = group->repeats;
        // sio_puts(repeat +"");
        //   printf("%d\n", repeat);
    }
    if (group->result_to != NULL) {
        // fail("setting variables not supported");
        // set_var(group->commands[j].pid_to, group->result_to->value);
        flag = 1;
        var_list[var_index] = group->result_to;
    }
    int k;

    for (k = 0; k < repeat; k++) {
        if (group->num_commands == 1) {
            run_command(&group->commands[0], flag);
        } else {
            if (group_mode == GROUP_AND) {
                if ((pid = Fork()) == 0)
                    handle_pipe(group, STDIN_FILENO, 0, flag);
                Wait(NULL);

            } else if (group_mode == GROUP_OR) {
                handle_or(group, flag);
                /* And here *///fail("only 1 command supported");
            } else {
                int i;
                for (i = 0; i < group->num_commands; i++) {
                    run_command(&group->commands[i], flag);
                }
            }

        }
        // group_mode = -1;
    }
}

static void run_command(script_command *command, int flag) {
    const char **argv;
    int i, to_delete;

    argv = malloc(sizeof(char *) * (command->num_arguments + 2));
    argv[0] = command->program;

    for (i = 0; i < command->num_arguments; i++) {
        if (command->arguments[i].kind == ARGUMENT_LITERAL)
            argv[i + 1] = command->arguments[i].u.literal;
        else
            argv[i + 1] = command->arguments[i].u.var->value;
    }
    argv[command->num_arguments + 1] = NULL;
//    int j;
//    for(j = 0; j < strlen(argv); j++)
//    {
////        if(argv[j] == NULL)
////        {
////            continue;
////        }
//        printf("%s\n", argv[j]);
//    }

//    Signal(SIGCHLD, sigchld_handler);
//    Signal(SIGINT, sigint_handler);
//    Signal(SIGTSTP, sigstp_handler);
//    sigset_t mask, empty_mask;
//
    sigset_t mask;
    Sigemptyset(&mask);
    Sigaddset(&mask, SIGINT);
    Sigprocmask(SIG_BLOCK, &mask, NULL);
//

    pid = Fork();
    freeArray(&array);
    insertArray(&array, pid);
    Setpgid(0, 0);
    if (command->pid_to != NULL) {
        var_list[var_index] = command->pid_to;
        set_var(var_list[var_index], pid);
        var_index++;
    }
    if (pid < 0) {
        unix_error("Fork error");
        return;
    }
    if (pid == 0) {

        Execve(argv[0], (char *const *) argv, environ);
    }
    Sigprocmask(SIG_UNBLOCK, &mask, NULL);
    int status;
     to_delete = Wait(&status);
    if(WUNTRACED == WIFSTOPPED(status))
    {
        freeArray(&continued_list);
        insertArray(&continued_list, to_delete);
    }
//    for (i = 0; i < array.used; i++) {
//        if (array.array[i] == to_delete)
//            array.array[i] = -1;
//    }

    if (flag) {
        set_status_to_list(status);
    }
//    if (Wait(&status) > 0) {
//        if (WIFEXITED(status) != 0) {
//            set_status_to_list(status);
//            return;
//        }
//    }
//
//    Sigsuspend(&empty_mask);
    //  sigprocmask(SIG_UNBLOCK, &mask, NULL);
    free(argv);
}


/* This run_command function is a good start, but note that it runs
   the command as a replacement for the `whoosh` script, instead of
   creating a new process. */

static void run_and_command(script_command *command) {
    const char **argv;
    int i;

    argv = malloc(sizeof(char *) * (command->num_arguments + 2));
    argv[0] = command->program;

    for (i = 0; i < command->num_arguments; i++) {
        if (command->arguments[i].kind == ARGUMENT_LITERAL)
            argv[i + 1] = command->arguments[i].u.literal;
        else
            argv[i + 1] = command->arguments[i].u.var->value;
    }
    argv[command->num_arguments + 1] = NULL;
    Execve(argv[0], (char *const *) argv, environ);
    free(argv);
}

/* You'll likely want to use this set_var function for converting a
   numeric value to a string and installing it as a variable's
   value: */

static void set_var(script_var *var, int new_value) {
    char buffer[32];
    free((void *) var->value);
    snprintf(buffer, sizeof(buffer), "%d", new_value);
    var->value = strdup(buffer);
}

static void set_status_to_list(int status) {
    if (WIFEXITED(status)) {
        set_var(var_list[var_index], WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        set_var(var_list[var_index], (-1 * WTERMSIG(status)));
    }
    var_index++;

}
#include "fash/fash.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


typedef char** fash_command_args_t;     // null terminated

typedef struct fash_command_info_t {
    const char*     command;
    const char**    info;   // Array of stringsm, last element must be NULL
    void            (*function)(fash_command_args_t);
} fash_command_info_t;

uint64_t __fash_command_args_count(fash_command_args_t args) {
    if (args == NULL) return 0;
    uint64_t count = 0;
    for (int i = 0; args[i] != NULL; i++) {
        count++;
    }
    return count;
}

void __fash_command_args_dispose(fash_command_args_t* args) {
    if (*args == NULL) return;
    for (int i = 0; (*args)[i] != NULL; i++) {
        free((*args)[i]);
    }
    free((void *)(((uint64_t)*args) - sizeof(char*)));
}

fash_command_args_t* __fash_command_args_parse(const char* command, const char* input) {
    fash_command_args_t* args;
    if (input == NULL) {
        *args = NULL;
        return NULL;
    }
    if (input[strlen(command)] != ' ') {
        *args = NULL;
        return NULL;
    }
    uint64_t com_len = strlen(command);
    uint64_t arg_len = strlen(input) - com_len;
    char* __args = malloc(arg_len);
    memset(__args, 0, arg_len);
    if (__args == NULL) {
        free(__args);
        *args = NULL;
        return NULL;
    }
    strcpy(__args, (const char*)((uint8_t*)input + strlen(command) + 1));
    uint64_t arg_count = 1;
    for (int i = 0; i < arg_len; i++) {
        if (__args[i] == ' ') {
            arg_count++;
        }
    }
    uint8_t* __arg_data = malloc((arg_count + 1 + 1) * sizeof(char*));
    if (__arg_data == NULL) {
        free(__args);
        *args = NULL;
        return NULL;
    }
    memset(__arg_data, 0, (arg_count + 1 + 1) * sizeof(char*));
    char** __arg_ptrs = (char**)(__arg_data + sizeof(char*));
    uint64_t arg_index = 0;
    uint64_t arg_start = 0;
    *args = (fash_command_args_t)__arg_ptrs;
    for (int i = 0; i < arg_len; i++) {
        if (__args[i] == ' ' || __args[i] == '\0') {
            uint64_t seg_len = i - arg_start;
            char* __seg = malloc(seg_len+1);
            if (__seg == NULL) {
                __fash_command_args_dispose(args);
                free(__args);
                return NULL;
            }
            memset(__seg, 0, seg_len+1);
            memcpy(__seg, __args + arg_start, seg_len);
            __arg_ptrs[arg_index++] = __seg;
            arg_start = i + 1;
            if (__args[i] == '\0') {
                break;
            }
        }
    }
    free(__args);
    return args;
}

void fash_clear_command(fash_command_args_t args) {
    tty_clear();
}

void fash_arg_command(fash_command_args_t args) {
    printf("\n");
    uint64_t count = __fash_command_args_count(args);
    if (count == 0) {
        printf("\tNo arguments provided\n\n");
        return;
    }
    printf("\tArguments count: %d\n\n\tArguments:\n", count);
    for (int i = 0; i < count; i++) {
        printf("\t\t%s\n", args[i]);
    }
    printf("\n");
}

void fash_help_command(fash_command_args_t args);
// NULL terminated array of command info
static fash_command_info_t fash_command_infos[] = {
    {
        "help",
        (const char*[]) {
            "Prints this help message",
            NULL
        },
        fash_help_command
    },
    {
        "clear",
        (const char*[]) {
            "Clears the screen",
            "No arguments taken",
            NULL
        },
        fash_clear_command
    },
    {
        "arg",
        (const char*[]) {
            "Tests argument parsing",
            "Takes as many arguments as desired",
            NULL
        },
        fash_arg_command
    },
    {
        NULL,
        NULL,
        NULL
    }
};

void fash_help_command(fash_command_args_t args) {
    printf("\nAvailable commands:\n");
    for (int i = 0; (fash_command_infos[i].command != NULL); i++) {
        printf("\n\t%s\n", fash_command_infos[i].command);
        if (fash_command_infos[i].info != NULL) {
            for (int j = 0; (fash_command_infos[i].info[j] != NULL); j++) {
                printf("\t\t%s\n", fash_command_infos[i].info[j]);
            }
        }
    }
    printf("\n");
}

static bool __fash_command_match(const char* input, const char* command) {
    unsigned char* __s1 = (unsigned char *)input;
    unsigned char* __s2 = (unsigned char *)command;
    for (/**/; /**/; __s1++, __s2++) {
        if (*__s1 == 0 && *__s2 == 0)
            return true;
		if (*__s1 != *__s2)
			return false;
	}
}

void fash_execute_command(const char* input) {
    if (!input || !*input) {
        return;
    }
    uint64_t input_len = strlen(input);
    char* __command = malloc(input_len);
    if (__command == NULL) {
        free(__command);
        printf("Error\n");
        return;
    }
    memset(__command, 0, input_len);
    uint64_t command_len = 0;
    for (int i = 0; i < input_len; i++) {
        command_len++;
        if (input[i] == ' ' || input[i] == '\n' || input[i] == '\0') {
            break;
        }
        __command[i] = input[i];
    }
    for (int i = 0; (fash_command_infos[i].command != NULL); i++) {
        if (__fash_command_match(__command, fash_command_infos[i].command)) {
            fash_command_args_t* args = __fash_command_args_parse(__command, input);
            fash_command_infos[i].function(*args);
            if (args != NULL)
                __fash_command_args_dispose(args);
            free(__command);
            return;
        }
    }
    printf("Command not found\n");
    free(__command);
}

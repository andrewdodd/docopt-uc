#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum {
  CLI_SHELL_COMMAND_LENGTH_MAX = 50, // i.e. max character length
  CLI_SHELL_COMMAND_MAX_TOKENS = 15, // i.e. max separate tokens (' ' delimited)
};

enum CliShell_Error {
  CLI_SHELL_SUCCESS = 0,
  CLI_SHELL_FAILURE = -1,
};

// Fwd declaration of type
struct cliShell;

typedef char const *(*CliShell_getPrompt)();
typedef char const *(*CliShell_processCommandFunc)(uint8_t argc, char **argv);

struct cliShell *CliShell_alloc(CliShell_getPrompt getPrompt,
                                CliShell_processCommandFunc processCommand,
                                FILE *outfp);
void CliShell_start(struct cliShell *cli);
enum CliShell_Error CliShell_handleChar(struct cliShell *cli, char c);

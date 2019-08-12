/*
 * CLI Shell module.
 *
 * For a great explainer on ANSI control sequences, see:
 * http://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html
 */
#include "cli_shell.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  CHAR_CARRIAGE_RETURN = 0x0d,
  CHAR_BELL = 0x07,
  CHAR_BACKSPACE = 0x08,
  CHAR_DELETE = 0x7f,
  CHAR_SPACE = 0x20,
  CHAR_ESCAPE = 0x1b,
};

enum {
  HISTORY_DEPTH = 4,
};

struct cliShell {
  FILE *out;
  CliShell_getPrompt getPrompt;
  CliShell_processCommandFunc processCommand;

  char current[CLI_SHELL_COMMAND_LENGTH_MAX];
  char history[HISTORY_DEPTH][CLI_SHELL_COMMAND_LENGTH_MAX];
  int8_t currentHistoryDepth;
  int8_t historyOffset;
  uint8_t lastHistoryIdx;
  uint8_t escapeLen;
  char escapeSeq[5];
  char *appendAt;
};

static void TxString(struct cliShell *cli, char const *const s) {
  fprintf(cli->out, "%s", s);
}

static void TxChar(struct cliShell *cli, char c) { fprintf(cli->out, "%c", c); }

static uint8_t getCurrentLineLength(struct cliShell *cli) {
  return (uint8_t)(cli->appendAt - cli->current);
}

static int8_t MIN(int8_t a, int8_t b) {
  if (a < b)
    return a;
  else
    return b;
}

static int8_t MAX(int8_t a, int8_t b) {
  if (a > b)
    return a;
  else
    return b;
}

static void clearTerminalLine(struct cliShell *cli) {
  uint8_t len = getCurrentLineLength(cli);
  if (len > 0) {
    char buf[15];
    // Move LEN left and clear line to the right
    snprintf(buf, sizeof(buf), "\x1b[%dD\x1b[0K", len);
    TxString(cli, buf);
  }
}

static void updateHistoryBuffers(struct cliShell *cli) {
  if (strcmp(cli->history[cli->lastHistoryIdx], cli->current) == 0) {
    // current command is same as the last one
    return;
  }
  cli->currentHistoryDepth++;
  cli->currentHistoryDepth = MIN(cli->currentHistoryDepth, HISTORY_DEPTH);
  cli->lastHistoryIdx = (cli->lastHistoryIdx + 1) % HISTORY_DEPTH;
  strcpy(cli->history[cli->lastHistoryIdx], cli->current);
}

static void writePrompt(struct cliShell *cli) {
  TxString(cli, cli->getPrompt());
}

static void clearCurrentCommand(struct cliShell *cli) {
  memset(cli->current, 0, sizeof(cli->current));
  cli->appendAt = cli->current;
}

static void replaceCurrentCommandWithHistory(struct cliShell *cli,
                                             int8_t offsetDirection) {
  cli->historyOffset += offsetDirection;
  cli->historyOffset = MAX(cli->historyOffset, -1);
  if (cli->historyOffset == -1) {
    clearCurrentCommand(cli);
    return;
  }
  cli->historyOffset = MIN(cli->historyOffset, cli->currentHistoryDepth - 1);

  uint8_t idx = (cli->lastHistoryIdx + (HISTORY_DEPTH - cli->historyOffset)) %
                HISTORY_DEPTH;
  strcpy(cli->current, cli->history[idx]);

  // establish new append point
  cli->appendAt = cli->current + strlen((char const *)cli->current);
}

static uint8_t handleEscapeSequence(struct cliShell *cli, uint8_t escapeLen,
                                    char const *const escapeSeq) {
  if (escapeLen < 2) {
    return escapeLen;
  }

  if (escapeLen == 2) {
    if (escapeSeq[0] == CHAR_ESCAPE && escapeSeq[1] == CHAR_ESCAPE) {
      // double escape, so reset command buffer
      clearTerminalLine(cli);
      clearCurrentCommand(cli);
      return 0;
    }

    if (escapeSeq[0] == CHAR_ESCAPE &&
        ((escapeSeq[1] == 'A') || (escapeSeq[1] == 'B'))) {
      // up arrow: ESC A, down arrow: ESC B
      clearTerminalLine(cli);
      replaceCurrentCommandWithHistory(cli, escapeSeq[1] == 'A' ? 1 : -1);
      TxString(cli, cli->current);
      return 0;
    }

    if (escapeSeq[0] == CHAR_ESCAPE && escapeSeq[1] != '[') {
      // This is likely an escape sequence we don't support
      TxChar(cli, CHAR_BELL);
      return 0;
    }
    return escapeLen;
  }

  if (strcmp("\x1b[A", escapeSeq) == 0) {
    // up arrow: ESC [ A
    clearTerminalLine(cli);
    replaceCurrentCommandWithHistory(cli, 1);
    TxString(cli, cli->current);
  } else if (strcmp("\x1b[B", escapeSeq) == 0) {
    // up arrow: ESC [ B
    clearTerminalLine(cli);
    replaceCurrentCommandWithHistory(cli, -1);
    TxString(cli, cli->current);
  } else {
    // this is an unsupported escape code
    TxChar(cli, CHAR_BELL);
  }

  // bit of a hack, but if you get two arrow keys in a row, it is possible you
  // should "restart" the escape len
  if (escapeSeq[escapeLen - 1] == CHAR_ESCAPE) {
    // e.g. if we receive LEFT then UP, our buffer will look like:
    // [ESC, 'D', ESC], and we are about to receive an 'A'
    // so let's pretend we just received the start of a new escape sequence
    return 1;
  }
  return 0;
}

enum CliShell_Error CliShell_handleChar(struct cliShell *cli, char c) {
  uint8_t len = getCurrentLineLength(cli);

  if (cli->escapeLen > 0) {
    cli->escapeSeq[cli->escapeLen++] = c;
    cli->escapeSeq[cli->escapeLen] = 0; // force null termination
    cli->escapeLen = handleEscapeSequence(cli, cli->escapeLen, cli->escapeSeq);
    return CLI_SHELL_SUCCESS;
  }

  if (c == CHAR_ESCAPE) {
    cli->escapeSeq[cli->escapeLen++] = c;
    return CLI_SHELL_SUCCESS;
  }

  // , as first character is alternative "history" command
  if (c == ',' && len == 0) {
    replaceCurrentCommandWithHistory(cli, 1);
    TxString(cli, cli->current);
    return CLI_SHELL_SUCCESS;
  }

  cli->historyOffset = -1;
  if (c == CHAR_CARRIAGE_RETURN) {
    *cli->appendAt = '\0'; // terminate string
    // If there is actually text, then save it
    if (len > 0) {
      updateHistoryBuffers(cli);
    }

    // tokenise
    // an extra one to detect "too many"
    char *tokens[CLI_SHELL_COMMAND_MAX_TOKENS + 1];
    uint8_t tokenCount = 0;
    tokens[tokenCount] = strtok(cli->current, " ");
    while (tokenCount < (CLI_SHELL_COMMAND_MAX_TOKENS + 1) &&
           tokens[tokenCount] != NULL) {
      tokens[++tokenCount] = strtok(NULL, " ");
    }

    TxChar(cli, CHAR_CARRIAGE_RETURN);
    TxChar(cli, '\n');
    char const *err = NULL;
    if (tokenCount == 0) {
      err = "No command";
    } else if (tokenCount > CLI_SHELL_COMMAND_MAX_TOKENS) {
      err = "Too many tokens";
    } else {
      err = cli->processCommand(tokenCount, tokens);
    }

    if (err) {
      TxString(cli, "Error: ");
      TxString(cli, err);
      TxString(cli, "\r\n");
    }

    memset(cli->current, 0, sizeof(cli->current));
    cli->appendAt = &cli->current[0];
    writePrompt(cli);
  } else if (c == CHAR_BACKSPACE || c == CHAR_DELETE) {
    // check if we are beyond the first char
    if (len > 0) {
      cli->appendAt--;
      // get rid of the last character by overwriting with a ' '
      // NB: 0x08 is Backspace
      TxString(cli, "\x08 \x08");
    }
  } else {
    if (len >= sizeof(cli->current) - 1) {
      cli->appendAt--;
      // Chime if the Command buffer is full
      TxChar(cli, CHAR_BELL);
      // Move back, but we will overwrite this with the echo below
      TxChar(cli, CHAR_BACKSPACE);
    }

    *cli->appendAt++ = c;
    TxChar(cli, c); // echo to the user
  }
  return CLI_SHELL_SUCCESS;
}

void CliShell_start(struct cliShell *cli) {
  FILE *outfp = cli->out;
  CliShell_getPrompt getPrompt = cli->getPrompt;
  CliShell_processCommandFunc processCommand = cli->processCommand;
  memset(cli, 0, sizeof(struct cliShell));
  cli->out = outfp;
  cli->getPrompt = getPrompt;
  cli->processCommand = processCommand;
  cli->historyOffset = -1;
  cli->appendAt = &cli->current[0];

  TxString(cli, "\r\nCLI starting ...\r\n");
  writePrompt(cli);
}

struct cliShell *CliShell_alloc(CliShell_getPrompt getPrompt,
                                CliShell_processCommandFunc processCommand,
                                FILE *outfp) {
  if ((getPrompt == NULL) || processCommand == NULL) {
    return NULL;
  }

  struct cliShell *cli = malloc(sizeof(struct cliShell));
  if (cli == NULL) {
    return NULL;
  }

  cli->historyOffset = -1;
  cli->out = outfp;
  cli->getPrompt = getPrompt;
  cli->processCommand = processCommand;
  return cli;
}

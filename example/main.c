#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <termios.h> //termios, TCSANOW, ECHO, ICANON
#include <unistd.h>  //STDIN_FILENO

#include "cli_shell.h"

#if 0
static char const *handleCommand(uint8_t argc, char **argv) {
  if (argc == 1 && strcmp("hi", argv[0]) == 0) {
    printf("HI yourself\r\n");
    return NULL;
  }

  static char errbuf[50];
  if (argc > 0) {
    snprintf(errbuf, sizeof(errbuf), "no cmd: %d, [0]=%s", argc, argv[0]);
  } else {
    snprintf(errbuf, sizeof(errbuf), "no cmd: %d", argc);
  }
  return errbuf;
}

static char const *getPrompt() { return "PROMPT>"; }
#else
#include "navalfate_autogen.h"

static char const *handleCommand(uint8_t argc, char **argv) {
 return Navalfate_processCommand(argc, argv);
}
static char const *getPrompt() { 
 return Navalfate_getPrompt();
}
#endif

static bool isProbablyAKillSignal(char c) {
  // This is just as a bit of a get out of gaol free for the program
  return c < 7;
}

int main(void) {
  struct cliShell *cli = CliShell_alloc(getPrompt, handleCommand, stdout);
  if (cli == NULL) {
    printf("unable to alloc cli");
    exit(0);
  }

  CliShell_start(cli);

  // Create a UART / serialport esque environment in the terminal
  // For more info, see SO answer here:
  // https://stackoverflow.com/a/38316343/756908
  static struct termios oldTermios, newTermios;
  tcgetattr(STDIN_FILENO, &oldTermios);
  newTermios = oldTermios;
  cfmakeraw(&newTermios);
  tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);

  // This part would be implemented in a microcontroller with a task / interrupt
  // handler etc ... NB: in this implementation the "stdout" is similar to the
  // TXBUF (depending on your chip) and the "getchar()" here is similar to the
  // bytes coming in on the RXBUF
  char c;
  do {
    c = getchar();
    enum CliShell_Error err = CliShell_handleChar(cli, c);
    if (err != CLI_SHELL_SUCCESS) {
      printf("Had error while handling char:%c [%x]\r\n", c, c);
    }
  } while (!isProbablyAKillSignal(c));

  tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
}

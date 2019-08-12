#ifndef DOCOPT_ARGS_H
#define DOCOPT_ARGS_H

#include <stdbool.h>
#include <stdint.h>

enum {
   DOCOPT_ARGS_TOKENS_MAX = 16,
   DOCOPT_ARGS_NAMED_ARGS_MAX = 7,
   DOCOPT_ARGS_POSITIONAL_ARGS_MAX = 7,
};

enum DocoptError {
    DOCOPT_NO_ERROR = 0,
    DOCOPT_ERROR_TOO_MANY_TOKENS,
    DOCOPT_ERROR_TOO_MANY_NAMED,
    DOCOPT_ERROR_TOO_MANY_POSITIONAL,
};

typedef struct {
   /* commands */
   uint64_t opcode;
   /* options without arguments */
   bool help;
   /* Named Arguments */
   uint8_t namedCount;
   char   *namedLabel[DOCOPT_ARGS_NAMED_ARGS_MAX];
   char   *namedValue[DOCOPT_ARGS_NAMED_ARGS_MAX];
   /* Positional Arguments */
   uint8_t posCount;
   char   *posValue[DOCOPT_ARGS_POSITIONAL_ARGS_MAX];
} DocoptArgs;

#endif // DOCOPT_ARGS_H

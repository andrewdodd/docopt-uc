#include "{{rendering.include_name}}_autogen.h"

#include <string.h>

#ifndef BV
#  define BV(n) (((uint64_t)1) << (n))
#endif

#define CMD1(a)                (BV(a))
#define CMD2(a, b)             (CMD1(a) | BV(b))
#define CMD3(a, b, c)          (CMD2((a), (b)) | BV(c))
#define CMD4(a, b, c, d)       (CMD3((a), (b), (c)) | BV(d))
#define CMD5(a, b, c, d, e)    (CMD4((a), (b), (c), (d)) | BV(e))
#define CMD6(a, b, c, d, e, f) (CMD5((a), (b), (c), (d), (e)) | BV(f))

// https://stackoverflow.com/questions/807244/c-compiler-asserts-how-to-implement
/** A compile time assertion check.
 *
 *  Validate at compile time that the predicate is true without
 *  generating code. This can be used at any point in a source file
 *  where typedef is legal.
 *
 *  On success, compilation proceeds normally.
 *
 *  On failure, attempts to typedef an array type of negative size. The
 *  offending line will look like
 *      typedef assertion_failed_file_h_42[-1]
 *  where file is the content of the second parameter which should
 *  typically be related in some obvious way to the containing file
 *  name, 42 is the line number in the file on which the assertion
 *  appears, and -1 is the result of a calculation based on the
 *  predicate failing.
 *
 *  \param predicate The predicate to test. It must evaluate to
 *  something that can be coerced to a normal C boolean.
 *
 *  \param file A sequence of legal identifier characters that should
 *  uniquely identify the source file in which this condition appears.
 */
#define CASSERT(predicate, file) _impl_CASSERT_LINE(predicate,__LINE__,file)

#define _impl_PASTE(a,b) a##b
#define _impl_CASSERT_LINE(predicate, line, file) \
    typedef char _impl_PASTE(assertion_failed_##file##_,line)[2*!!(predicate)-1];


enum {
  // AUTOGEN LIST OF OPTIONS - START
  {% for token in rendering.tokens -%}
  {{token|escape_c_keywords}} = {{loop.index0}},
  {% endfor -%}
  // AUTOGEN LIST OF OPTIONS - END
  LAST,
};

// Check will fit in opcode bitmap
// https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c
#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
CASSERT(LAST < (FIELD_SIZEOF(DocoptArgs, opcode) * 8), __file__);

static const char *Names[] = {
  // AUTOGEN NAMES OF OPTIONS - START
  {% for token in rendering.tokens -%}
  "{{token}}",
  {% endfor -%}
  // AUTOGEN NAMES OF OPTIONS - END
};

/**
 * Parses the arguments into the DocoptArgs struct.
 *
 * NB: Command tokens are only entered into the opcode once, so if a command 
 *     token is present more than once the following copies will be inserted
 *     into the positional arguments list.
 */
static enum DocoptError parseArgs(DocoptArgs *args, uint8_t argc, char **argv) {
  memset(args, 0, sizeof(DocoptArgs));

  if (argc > DOCOPT_ARGS_TOKENS_MAX) {
    return DOCOPT_ERROR_TOO_MANY_TOKENS;
  }

  // find all matching commands. 
  // NB: argsConsumed needs to have sufficient bits for the DOCOPT_ARGS_TOKENS_MAX 
  //     number of tokens (not the opcode bit width)
  uint32_t argsConsumed = 0;
  CASSERT(LAST < (sizeof(argsConsumed) * 8), __file__);

  for (int i = 0; i < argc; i++) {
    char *arg = argv[i];

    // do help first, as it is a bit of a special case
    if (!args->help) {
      if ((strcmp("?", arg) == 0) || (strcmp("-h", arg) == 0) || 
          (strcmp("--help", arg) == 0)) {
        args->help = true;
        argsConsumed |= BV(i);
        continue;
      }
    }
    
    for (int j = 0; j < LAST; j++) {
      if ((args->opcode & BV(j)) != 0) {
         continue;  // no need to check this one again
      }
      if (strcmp(Names[j], arg) == 0) {
         args->opcode |= BV(j);
         argsConsumed |= BV(i);
         break;
      }
    }
  }

  // Collect anything that was not a command
  enum DocoptError err = DOCOPT_NO_ERROR;
  for (int i = 0; i < argc; i++)
  {
     if ((BV(i) & argsConsumed) != 0)
     {
         continue;
     }

     char *arg = argv[i];
     if (arg[0] == '-' && arg[1] == '-') {
        args->namedLabel[args->namedCount] = &arg[2];
        args->namedValue[args->namedCount] = NULL;

        while (*arg != '\0') {
          if (*arg == '=') {
            *arg = '\0';
            args->namedValue[args->namedCount] = arg + 1;
            break;
          }
          arg++;
        }
        args->namedCount++;
     } else {
        args->posValue[args->posCount++] = arg;
     }

     if (args->posCount >= DOCOPT_ARGS_POSITIONAL_ARGS_MAX) {
        err = DOCOPT_ERROR_TOO_MANY_NAMED;
        break;
     }
     if (args->namedCount >= DOCOPT_ARGS_NAMED_ARGS_MAX) {
        err = DOCOPT_ERROR_TOO_MANY_POSITIONAL;
        break;
     }
  }
  return err;
}

char const *{{rendering.module_prefix}}_processCommand(uint8_t argc, char **argv)
{
  {%- if rendering.multithreaded %}
  DocoptArgs docoptArgs; // Lives on the stack
  {% else %}
  static DocoptArgs docoptArgs; // Shared between all calls
  {% endif -%}
  int errNo = parseArgs(&docoptArgs, argc, argv); 

  if (errNo != DOCOPT_NO_ERROR) {
    return {{rendering.module_prefix}}_handle_Error(&docoptArgs);
  }

  switch (docoptArgs.opcode) {
    // AUTOGEN CASES FOR COMMAND COMBINATIONS - START
    {% for command in rendering.commands -%}
    case CMD{{command.parts|length}}({{command.parts|escape_c_keywords|join(", ")}}):
      return {{rendering.module_prefix}}_handle_{{command.function_name}}(&docoptArgs);
    {% endfor -%}
    // AUTOGEN CASES FOR COMMAND COMBINATIONS - END
    default:
      break;
  }
  if (docoptArgs.help) {
    return {{rendering.module_prefix}}_handle_Help(&docoptArgs);
  }
  return "Unknown command";
}

char const *{{rendering.module_prefix}}_getPrompt(void) {
  return "{{rendering.prompt}} ";
}

char const *{{rendering.module_prefix}}_getHelpText(void) {
  return "{{rendering.help}}";
}

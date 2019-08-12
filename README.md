# Docopt uC
A Docopt library suitable for simple CLI generation in microcontrollers.

# How to use

This library should be installed however you normally install Python packages (i.e. it is usually recommended to use a virtual environment or something similar e.g. `mkvirtualenv my-environment`). To install from PyPI:

    > pip install docopt_uc
   
To use with the built-in C and H file templates, simply provide the module name and your docopt file:

    > docopt-uc mymodule mymodule.docopt
   
This will produce three files in the directory, the `docopt_args.h` file (which you only need one of in your project), and the two generated files. For example:

    > ls
    navalfate.docopt
    > docopt-uc navalfate navalfate.docopt
    > ls
    docopt_args.h       navalfate.docopt    navalfate_autogen.c navalfate_autogen.h
    
The `XXX_autogen.h` file will specify a number of functions that need implementations provided (i.e. one for each command).

If you want to provide your own C and H file templates, that is also possible via options.

# A more detailed example

This repository has an example folder that contains:

 - A slightly modified `navalfate.docopt` file
 - A basic `main.c` file, that sets up a more UART-like terminal environment (well...on my Mac it does) for the rest of the example to use
 - A basic `cli_shell` implementation, that implements basic command history, and does what you might expect a CLI in a small embedded project might do
 - The start of the implementation for the Naval Fate CLI functionality

 There is an additional README in that folder which explains how to run the example and see this library in action.

# Why this library

There comes a time in every embedded-systems project where a CLI becomes a necessity. This could be for testing, for factory calibration, for use in the field, or just for letting a product manager play with CLI so they know you're doing something.

Almost all of these CLIs generate hours of discussion on "features" (both real product features and techical features) such as:

 - Should we have command completion, command history, etc etc?
 - How should we structure these menus best, both in code and actually at the terminal? 
 - How can we allow the product manager the freedom to rearrange the CLI, but be confident we have the code right? and
 - Who is going to write all this boilerplate (because...let's face it...a lot of this code is easy and tedious but usually quite important)?


# This library's position

This library takes a few hard positions on the items above, and comes what I hope is a middle ground:

 1. Boilerplate is unavoidable

    CLIs are typically low-value, non-differentiating parts of your embedded products (unless you are Cisco...in which case they are part of an operator lock-in strategy). They are also usually really boring and repetitive to implement. However, they are also usually very important, as they are used for debugging, calibration, configuration, firmware upgrading, etc, and they often try to cut across the entire application (e.g. does the config update 'right now' _and_ in the persistant store?). By flattening out the CLI menu structure; generating all the necessary function headers; and by handling the dispatch for you, using a tool like `docopt-uc` can give you confidence that nothing has been missed.
    
 2. Shell features are different to the CLI command handling

    This library believes command completion, command history, etc. all belong to the __shell implementation__ (sure...command completion needs to know the commands, but that's a different story). This library is looking to solve the problem *"what function should I call once the user pushes enter"*. It assumes the shell will do the work to parse the user's command into a standard `(uint8_t argc, char **argv)` signature, and it just needs to dispatch to the correct CLI handler function.
    
 3. Coping with change

    A CLI is often built incrementally in a project, to aid with development, testing, board-bring-up and well...anything. Product managers also evolve their understanding of products as they come to life. Too many times I have had to rearrange a CLI menu comprised of strange tables of tables of structs with unclear members such as `{void *leaf, char *nodeName, void *nextLevel, void *userArg}` etc, only to be unsure if every edge case was covered. This library believes the docopt file should be the product manager's responsibility, while using this library to generate the functions needed means that renames, restructurings, additions and deletions are all hopefully within the capabilities of your more junior engineers.
    
 4. At the terminal

    This might be a bit controversial but...the implementation of the default template for this library doesn't care for the order of your command words. While some people get a bit upset by this, I personally believe it is a better UX to be flexible. In the provided example, all of these commands would produce the same result:
    
        > ship create Titanic
        > create ship Titanic
        > create Titanic ship
        > Titanic create ship
        ... etc
    
    I agree a user doing this at your CLI is a bit strange (especially the last one), but the translation error in the first two is pretty common in my experience and something that is easy to support if your technical design doesn't rest on tables of tables of tables (I really don't like the table thing, but I guess you figured that out).
    
# Caveats

Obviously there are some caveats and limitations with this library. An inexhausive list include:

### There's a limit of 64 unique keywords (and an assumption of 64-bit integer support)

The design uses string comparison on each of the passed arguments to build an "opcode". It then uses this opcode to dispatch to the correct handler function (and consequently doesn't care about the order of those commands). I chose a 64-bit unsigned integer because 64-bit is probably supported by your compiler AND it is wide enough for you to hopefully never run out of unique keywords.

If you would like to use a 32-bit or 16-bit or 128-bit wide opcode, you can also just edit the `docopt_args.h` file you use (just be careful that the `docopt-uc` command does not over-write it...perhaps by passing the `--no-docopt-args-h` argument). The generated C file only checks that the number of keywords will fit in the `opcode` struct member.

You can try it out by setting the opcode size to `uint8_t` and running just `make` in the example, you should get a compile time error because there are 9 keywords:
    
    navalfate_autogen.c:65:1: error: 'assertion_failed___file___65' declared as an array with a negative size
    CASSERT(LAST < (FIELD_SIZEOF(DocoptArgs, opcode) * 8), __file__);

If you do run out, you can always make two CLIs and stitch them together with your own dispatch function...something like:

    static char const *handleCommand(uint8_t argc, char **argv) {
      if (strcmp("a-side", *argv[0]) == 0) {
        return Aside_processCommand(argc-1, argv++); // consume the first arg
      } else if (strcmp("b-side", *argv[0]) == 0) {
        return Bside_processCommand(argc-1, argv++); // consume the first arg
      }
      return "Not supported";
    }

But you should also reconsider both a) if this library is really appropriate, and b) if the extent of your CLI is really appropriate.

### It is probably not that fast

Doing so many loops and string comparisons is possibly not that fast, but hey...it's a CLI, probably running over something with a rate specified in baud, how fast does it need to be?

### It doesn't handle all docoptions

For example:

 - It doesn't handle the OR'd command options (e.g. `mine (set|remove)`), you have to do them the long way (i.e. `mine set... mine remove...`).
 - The default C implementation only supports named arguments with a `--` prefix.

It does do a pretty good job otherwise, which includes:

 - Giving you the named arguments in two lists of `namedLabel` and `namedValue`, which have their preceeding `--` and `=` removed respectively.
 - Giving you all positional arguments in the `posValue` array.

I think strikes a good balance at this stage, especially for embedded projects.

# Thanks

To [Kim Blomqvist's blog post](http://kblomqvist.github.io/2013/03/21/creating-beatiful-command-line-interfaces-for-embedded-systems-part1) for inspiring me to do this, and to the [docopt library](http://docopt.org/) for making it possible. 
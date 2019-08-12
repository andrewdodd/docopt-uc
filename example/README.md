# Naval Fate

This example is a partial implementation to show you how this library works, and what the result looks like.

To get it running, follow the steps below.

# Getting started

You can download the files from the directory, or you could clone the repository:

    > git clone https://github.com/andrewdodd/docopt-uc.git
    
Make and activate a virtualenv for this project (or use pipenv or do what you like!), e.g.:

    > mkvirtualenv docopt-uc    
    
You should install the Docopt-uC library, either from the PyPI repository:

    > pip install docopt-uc
    
Or directly from the cloned repository:

    > cd docopt-uc       # i.e. change into the root of the repo, next to the setup.py file
    > pip install -e ./  # install an "editable" version of this library
    
    
Move to the example directory and see what is there:

    > cd example
    > ls
    README.md         cli_shell.c       cli_shell.h       main.c
    makefile          navalfate.docopt  navalfate_impl.c  obj

You should see:

 - `cli_shell.h` & `cli_shell.c` - An implementation of a basic shell, similar to what you might find in a simple embedded project
 - `main.c` - A file to create a UART-like terminal and to glue the shell and docopt implementations together
 - `makefile` - For building the example
 - `navalfate.docopt` - The Docopt compatible CLI definition
 - `navalfate_impl.c` - The start of the implemation of the actual CLI handlers.

 But you should not see any automatically generated files.

# Generate the cli

You should be able to run the exported command `docopt-uc` (if the docopt library successfully installed). E.g. 

    > docopt-uc --help
    
To generate the CLI, run the following command:

    > docopt-uc navalfate navalfate.docopt
    
This should create the `docopt_args.h` file, and the two `navalfate_autogen` files:

    > ls
    README.md           cli_shell.c         cli_shell.h         docopt_args.h
    main.c              makefile            navalfate.docopt    navalfate_autogen.c
    navalfate_autogen.h navalfate_impl.c    obj
    
# Build the example

You'll need a C compiler installed, but it should be as simple as:

    > make
    
You should see:

    gcc -c -o obj/main.o main.c -I.
    gcc -c -o obj/cli_shell.o cli_shell.c -I.
    gcc -c -o obj/navalfate_autogen.o navalfate_autogen.c -I.
    gcc -c -o obj/navalfate_impl.o navalfate_impl.c -I.
    gcc -o example obj/main.o obj/cli_shell.o obj/navalfate_autogen.o obj/navalfate_impl.o -I.
    
And you should have a binary built called `example`, which can be used like this:

    > ./example

    CLI starting ...
    Naval_Fate> -h
    Error: Usage:
      Naval_Fate> ships
      Naval_Fate> ship create <name>
      Naval_Fate> ship <name> move <x> <y> [--speed=<kn>]
      Naval_Fate> ship shoot <x> <y>
      Naval_Fate> mine set <x> <y> [--moored|--drifting]
      Naval_Fate> mine remove <x> <y> [--moored|--drifting]
      Naval_Fate> --help
      Naval_Fate> --version
    
    Options:
      -h --help     Show this screen.
      --version     Show version.
      --speed=<kn>  Speed in knots [default: 10].
      --moored      Moored (anchored) mine.
      --drifting    Drifting mine.
    
    Naval_Fate> ship create Titanic
    [0]: Titanic
    Naval_Fate> move ship Titanic 1 2
    Moving ship Titanic to 1, 2 at unknown speed
    Naval_Fate> ship Titanic move 3 4 --speed=5
    Moving ship Titanic to 3, 4 at 5 knots
    Naval_Fate> ship Titanic move 6 7 --fullspeed=ahead
    Moving ship Titanic to 6, 7 with an unsupported option list:
      > fullspeed = ahead
    Naval_Fate> create ship HMS.Endeavour
    [1]: HMS.Endeavour
    Naval_Fate> ships
    Ships: 2
    [0]: Titanic
    [1]: HMS.Endeavour
    Naval_Fate>
    
Which is hopefully enough for you to understand and follow what is going on.

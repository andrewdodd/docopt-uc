"""Docopt uC - docopt for basic microcontroller CLIs

Usage:
  docopt-uc <module_name> <docopt_file> [options]
  docopt-uc (-h | --help)
  docopt-uc --version

Example, try:
  docopt-uc NavalFate navalfate.docopt --short=">"
  docopt-uc NavalFate navalfate.docopt --template_h=OUR_TEMPLATE_automatic.h --template_c=OUR_TEMPLATE_automatic.c --template_prefix=OUR_TEMPLATE

Options:
  -h --help     Show this screen.
  --template_h=<filename>  Name of .h templates [default: CLI_TEMPLATE_autogen.h]. 
                           NB: this is shipped with the package
  --template_c=<filename>  Name of .c templates [default: CLI_TEMPLATE_autogen.c].
                           NB: this is shipped with the package
  --template_prefix=<str>  Part of the template filename to replace [default: CLI_TEMPLATE].
  --output_dir=<str>       Where to write files [default: ./].
  --short=<prompt>         Replace the prompt with this instead (i.e. replace
                           the "docopt_uc.py" with this string)
  --no-docopt-args-h       Prevent the output of the "docopt_args.h" file [default: False]

"""

import sys
import os.path
import docopt
import shutil
from jinja2 import Template, environment
import pkg_resources

C_RESERVED_WORDS = {
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "int", "long", "register", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while",
} # yapf: disable


def escape_c_keywords(s):
    if isinstance(s, list):
        return [escape_c_keywords(x) for x in s]
    if s in C_RESERVED_WORDS:
        return "_" + s
    return s


environment.DEFAULT_FILTERS['escape_c_keywords'] = escape_c_keywords


class Command:
    def __init__(self, parts, docopt_text=None):
        self.parts = parts
        self.docopt_text = docopt_text

    @property
    def function_name(self):
        return "".join(p.capitalize() for p in self.parts).replace("_", "")


class Rendering:
    def __init__(self, module_name, commands, prompt, doc):
        self.module_name = module_name
        self.commands = commands
        self.prompt = prompt
        self.doc = doc

    @property
    def help(self):
        return self.doc.replace("\n", "\\r\\n\\\n")

    @property
    def tokens(self):
        t = set()
        l = list()

        #[p for c in self.commands for p in c.parts]
        for cmd in self.commands:
            for part in cmd.parts:
                if part not in t:
                    t.add(part)
                    l.append(part)
        return l

    @property
    def include_name(self):
        return self.module_name.lower().strip()

    @property
    def module_prefix(self):
        name = self.include_name
        parts = name.split('_')
        return "".join([p.capitalize() for p in parts])


def read_template_file_contents(filename):
    try:
        with open(filename, 'r') as f:
            contents = f.read()
            return Template(contents)
    except FileNotFoundError as ex:
        pass
    # https://stackoverflow.com/questions/6028000/how-to-read-a-static-file-from-inside-a-python-package
    # try as a package resource
    resource_path = '/'.join(('templates', filename))
    fn = pkg_resources.resource_filename(__name__, resource_path)
    with open(fn, 'r') as f:
        contents = f.read()
        return Template(contents)


def main():
    args = docopt.docopt(__doc__, version='none')

    with open(args['<docopt_file>'], 'r') as f:
        args['<docopt_file>'] = f.read()

    template_h_name = args['--template_h']
    template_c_name = args['--template_c']

    args['template_h_obj'] = read_template_file_contents(args['--template_h'])
    args['template_c_obj'] = read_template_file_contents(args['--template_c'])

    doc = args['<docopt_file>']
    usage = docopt.printable_usage(doc)
    all_options = docopt.parse_defaults(doc)
    pattern = docopt.parse_pattern(docopt.formal_usage(usage), all_options)
    prompt = usage.split()[1].strip()

    usage_lines = [x.replace(prompt, "") for x in usage.split('\n')[1:]]

    tokens = []
    commands = []
    # I'm not sure why we have to reach in here, but it "works"
    required_commands = pattern.children[0].children
    for idx, required in enumerate(required_commands):
        parts = [
            o.name for o in required.children if isinstance(o, docopt.Command)
        ]
        if not parts:
            continue
        # "help" is a special case? So exclude?
        if "help" in parts:
            continue
        tokens.extend(parts)
        docopt_text = usage_lines[idx].strip() if idx < len(usage_lines) else None
        commands.append(Command(parts, docopt_text))

    if args['--short'] is not None:
        doc = doc.replace(prompt + " ", args['--short'] + " ")

    rendering = Rendering(args['<module_name>'], commands, prompt, doc)

    if len(rendering.tokens) > 64:
        raise docopt.DocoptExit(
            'Too many unique tokens ({}) for Docopt μC (max:64)'.format(
                len(rendering.tokens)))

    too_long_commands = []
    for cmd in rendering.commands:
        if len(cmd.parts) > 6:
            too_long_commands.append(cmd)

    if too_long_commands:
        summaries = [
            ' > {}'.format(" ".join(p for p in c.parts))
            for c in too_long_commands
        ]

        raise docopt.DocoptExit('\n'.join([
            'The following commands are too long for Docopt μC (max: 6 long):'
        ] + summaries))

    output_h_filename = args['--template_h'].replace(args['--template_prefix'],
                                                     rendering.include_name)
    output_c_filename = args['--template_c'].replace(args['--template_prefix'],
                                                     rendering.include_name)

    output_h_filename = os.path.join(args['--output_dir'], output_h_filename)
    output_c_filename = os.path.join(args['--output_dir'], output_c_filename)

    with open(output_h_filename, 'w') as f:
        f.write(args['template_h_obj'].render(rendering=rendering))

    with open(output_c_filename, 'w') as f:
        f.write(args['template_c_obj'].render(rendering=rendering))

    if args["--no-docopt-args-h"] is False:
        # copy the docopt header file to the output directory
        docopt_args = pkg_resources.resource_filename(__name__,
                                                    'templates/docopt_args.h')
        shutil.copy2(docopt_args, args['--output_dir'])


if __name__ == '__main__':
    main()

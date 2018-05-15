# This script is modified version of single.py from a great C++ library for
# Lua bindings called "sol"
# https://github.com/ThePhD/sol2

import argparse
import os
import datetime
import re


parser = argparse.ArgumentParser(
    description = 'Converts rest to a single header file')
parser.add_argument('--output', '-o', nargs = 1,
    help = 'path to output file', metavar = 'file', default = 'single/rest.hpp')
args = parser.parse_args()

single_file = args.output if type(args.output) == str else args.output[0]
license_file = 'LICENSE.txt'
include_guard = 'REST_SINGLE_INCLUDE_HPP'

includes = []
project_include = re.compile(r'#include <rest/(.*?)>')
depend_include = re.compile(r'#include <(.*?)>')
ifndef_cpp = re.compile(r'#ifndef REST_.*?HPP')
define_cpp = re.compile(r'#define REST_.*?HPP')
endif_cpp = re.compile(r'#endif // REST_.*?HPP')


def is_include_guard(line):
    return ifndef_cpp.match(line) or \
        define_cpp.match(line) or endif_cpp.match(line)


def get_project_include(line):
    project_match = project_include.match(line)
    if project_match:
        return os.path.join('rest', project_match.group(1))
    return None


def get_depend_include(line):
    depend_match = depend_include.match(line)
    if depend_match:
        return depend_match.group(1)
    return None


def build_intro():
    intro = """// {license}
// This file was generated with a script.
// Generated {time} UTC

#ifndef {guard}
#define {guard}

"""
    with open(license_file, 'r') as f:
        license = '// '.join(f.readlines())

    return intro.format(
        license = license,
        time = datetime.datetime.utcnow(),
        guard = include_guard)


def process_file(filename, out):
    if filename in includes:
        return  # Already processed
    includes.append(filename)

    print('processing {}'.format(filename))

    out.write('// beginning of {}\n\n'.format(filename))
    empty_line_state = True

    with open(filename, 'r') as f:
        for line in f:
            # skip comments
            if line.startswith('//'):
                continue

            # skip include guard non-sense
            if is_include_guard(line):
                continue

            name = get_project_include(line)
            if name:
                process_file(name, out)
                continue

            dependency = get_depend_include(line)
            if dependency:
                depend_file = os.path.join('dependency', dependency)
                if depend_file in includes:
                    continue
                includes.append(depend_file)

            empty_line = len(line.strip()) == 0
            if empty_line and empty_line_state:
                continue
            empty_line_state = empty_line

            # Source code line
            out.write(line)

    out.write('// end of {}\n\n'.format(filename))


with open(single_file, 'w') as f:
    f.write(build_intro())
    process_file('rest.hpp', f)
    f.write('#endif // {}\n\n'.format(include_guard))


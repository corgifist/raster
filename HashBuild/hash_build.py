# Hash Build is the most (I guess) minimalistic build system for C++
# It uses hashing algorithms to decide which file to compile or link
# In theory, it will be much faster than Make or some other build systems
# (Only in theory)

import hashlib
import sys
import subprocess
import os
import colorama

from hash_build_common import *
from hash_build_lexer import build_lexer, fetch_all_tokens
from hash_build_parser import parse
from hash_build_interpreter import interpret_master
from hash_build_env import var_env, scenarios

colorama.init()
argv = sys.argv[1:]
hash_build_file_exists = os.path.exists("hash_build")

if (len(argv) == 0 or argv[0] == '--help') and not hash_build_file_exists:
    print("Usage info:")
    print(f"\t{sys.argv[0]} [flags...]")
    print("Flags:")
    print("\tNo flags yet, sry!")
    exit(0)

if not hash_build_file_exists:
    fatal("no hash_build script found in current directory!")
    exit(1)

info("found hash_build script, starting build")

if not os.path.exists("hash_build_files"):
    info("hash_build_files folder does not exist, creating one...")
    os.mkdir("hash_build_files")

try:
    gcc_process = subprocess.call(["g++"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
except FileNotFoundError:
    fatal("gcc/g++ is not installed on your machine")
    info("hash_build can only work with gcc/g++, so if you have other compilers installed, please install gcc/g++ alongside them")
    exit(1)

with open("hash_build", "r+") as hash_build_file:
    hash_build_script = hash_build_file.read()
    lexer = build_lexer(hash_build_script)
    ast = parse(hash_build_script)
    # print(ast)
    interpret_master(ast)
    
    default_scenario = 'null'
    scenario_overriden = False
    index = 0
    for arg in argv:
        if arg.startswith("-s"):
            name = arg[2:]
            var_env[name] = argv[index + 1]
            index += 1
        if arg.startswith("-d"):
            default_scenario = arg[2:]
            scenario_overriden = True
        index += 1

    if not scenario_overriden:
        try:
            default_scenario = var_env['default_scenario']
        except KeyError:
            warning("default_scenario is not set, so it will be automatically set to 'build'")
            default_scenario = 'build'
    interpret_master(scenarios[default_scenario])
        
    
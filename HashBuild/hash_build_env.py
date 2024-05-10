import subprocess
import sys
import os
from hash_build_common import *
import shutil
import platform
import glob
import pathlib
from hash_build_common import warning, info, fatal


scenarios = {}
var_env = {}

compile_options = []
compile_definitions = []
link_options = []
include_path = None

dump_compilation_process = False

def cat(a, b):
    return a + b

def cats(a, b):
    return str(a) + str(b)

def object_compile(source, arch, args=[], library_path="."):
    machine_bit = f'-m{arch}'
    if type(source) == str:
        return object_compile([source], arch)
    
    progress = 0
    result_objects = list()
    for file in source:
        name_hash = hash_string(file)
        hash_path = f"hash_build_files/{name_hash}.hash"
        if not os.path.exists(hash_path):
            with open(hash_path, "w+") as tmp_file:
                pass
        with open(f"hash_build_files/{name_hash}.hash", "r+") as hash_file:
            with open(file, "r") as source_file:
                progress += 1
                source_file_code = source_file.read()
                code_hash = hash_string(source_file_code)
                saved_hash_code = hash_file.read()
                output_file = f"hash_build_files/{name_hash}.o"
                result_objects.append(output_file)
                if code_hash == saved_hash_code:
                    info(f"[{progress}/{len(source)}] no need to recompile {file} ({output_file})")
                    continue
                hash_file.write(code_hash)
                extension = file.split(".")[-1]
                std_cxx_override_exists = 'std_cxx' in var_env
                std_c_override_exists = 'std_c' in var_env
                compile_command = f"{'g++' if extension == 'cpp' else 'gcc'} -L {library_path} {'-I' + include_path if include_path != None else ''} -c {file} {machine_bit} -o {output_file} {' '.join(compile_definitions)} {('-std=c++' + var_env['std_cxx'] if std_cxx_override_exists else '') if extension == 'cpp' else ''} {('-std=c' + var_env['std_c'] if std_c_override_exists else '') if extension == 'c' else ''}{' ' + ' '.join(compile_options) if len(compile_options) != 0 else ''} {' '.join(args)}"
                # info(compile_command)
                info(f"[{progress}/{len(source)}] compiling {file} ({output_file})")
                compilation_process = subprocess.run(list(filter(lambda x: x != '', compile_command.split(" "))), stdout=sys.stdout, stderr=sys.stderr)
                if (compilation_process.returncode != 0):
                    hash_file.close()
                    os.remove(f"hash_build_files/{name_hash}.hash")
                    fatal(f"compiler returned non-zero code while compiling {file}")
                    exit(1)
    
    info("object compilation finished successfully")
    return result_objects

def object_clean():
    shutil.rmtree("hash_build_files")
    os.mkdir("hash_build_files")

def dump_var_env():
    print(var_env)

def glob_files_gen(root: pathlib.Path, ext):
    for item in root.iterdir():
        yield item
        if item.is_dir():
            yield from glob_files_gen(item, ext)

def glob_files(root, ext):
    files = list(glob_files_gen(pathlib.Path(root), ext))
    files = list(map(lambda x: x.__str__(), files))
    files = list(filter(lambda x: x.endswith("." + ext), files))
    return files

def get_platform():
    return platform.system().lower()

def eq(a, b):
    return a == b

def _not(x):
    return not x

def link_executable(objects, out, link_libraries=[], shared='null', basename='implib', library_path='.', rpath=''):
    if shared == 'shared':
        shared = True
    else:
        shared = False
    str_objects = ' '.join(objects)
    transformed_libraries = list()
    for lib in link_libraries:
        transformed_libraries.append("-l" + lib)
    out_implib = ""
    if get_platform() == "windows" and shared:
        out_implib = f"-Wl,--export-all-symbols,--out-implib,lib{basename}.a"
    if rpath != '':
        rpath = f'-Wl,-rpath,{rpath}'
    link_command = f"g++ -L {library_path} {'-shared' if shared else ''} -o {out} {str_objects}{' ' if len(link_libraries) != 0 else ''}{' '.join(transformed_libraries)} {' '.join(link_options)} {out_implib} {rpath}"
    info(f"linking {'executable' if not shared else 'shared library'} {out}")
    link_process = subprocess.run(list(filter(lambda x: x != '', link_command.split(" "))), stdout=sys.stdout, stderr=sys.stderr)
    if link_process.returncode != 0:
        fatal("linker returned non-zero code!")
        exit(1)
    return True

def list_append(list, value):
    list.append(value)

def object_add_compile_definition(definition):
    compile_definitions.append("-D" + definition)

interpreter_proc = None

def call_scenario(scenario_name):
    interpreter_proc(scenarios[scenario_name])

def object_set_include_path(x):
    global include_path
    include_path = x

def for_each(iterable, var_name, callee):
    for x in iterable:
        var_env[var_name] = x
        interpreter_proc(scenarios[callee])

def object_add_link_options(option):
    link_options.append("-" + option)

def string_contains(x, y):
    return x.contains(y)

def load_module(path):
    return exec(open(path).read())

functions = {
    "print": print,
    'cat': cat,
    "object_compile": object_compile,
    "object_add_compile_options": lambda x: compile_options.append("-" + x),
    "dump_compilation_commands": lambda x: [dump_compilation_commands := True][0],
    "dump_var_env": dump_var_env,
    "glob_files": glob_files,
    "object_clean": object_clean,
    "link_executable": link_executable,
    "get_platform": get_platform,
    'eq': eq,
    "list": list,
    "rm": os.remove,
    "rmtree": shutil.rmtree,
    "mkdir": os.mkdir,
    "string_contains": string_contains,
    "path_exists": os.path.exists,
    "object_add_compile_definition": object_add_compile_definition,
    "object_set_include_path": object_set_include_path,
    "list_append": list_append,
    "warning": warning,
    "call_scenario": call_scenario,
    "for_each": for_each,
    "subprocess_run": subprocess.run,
    "object_add_link_options": object_add_link_options,
    "info": info,
    "fatal": fatal,
    'not': _not,
    "mv": shutil.move,
    "load_module": load_module
}

def arg_wrapper(func, args):
    try:
        return func(*args)
    except Exception as ex:
        fatal(f"fatal error at arg_wrapper: {str(func)}, {str(args)}")
        print(str(ex))
        exit(1)
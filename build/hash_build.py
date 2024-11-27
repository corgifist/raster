import argparse
import typing
import pathlib
import os
import platform
import hashlib
import subprocess
import sys
import shutil
import re

from colorama import init, just_fix_windows_console, Fore, Style

just_fix_windows_console()
init()

def get_platform():
    return platform.system().lower()

shared_library_prefix = ""
shared_library_extension = ""

if get_platform() == "linux" or "bsd" in get_platform():
    shared_library_prefix = "lib"
    shared_library_extension = "so"
elif get_platform() == "darwin":
    shared_library_prefix = "lib"
    shared_library_extension = "dylib"
elif get_platform() == "windows":
    shared_library_prefix = ""
    shared_library_extension = "dll"

compilation_database = []

def hash_string(string: str):
    return hashlib.blake2s(string.encode()).hexdigest()

argument_parser = argparse.ArgumentParser(prog="HashBuild", description="embeddable build system")

argument_parser.add_argument("-e", "--entry_point", metavar="ENTRY_POINT_NAME", help="set up entry point", default=None, type=str)
argument_parser.add_argument("-c", "--clean", help="clean object & dependency cache", default=False, action="store_true")
argument_parser.add_argument("-d", "--database", help="generate compilation database", default=False, action="store_true")

arguments = vars(argument_parser.parse_args())

# prints colored text, info/warn/error functions are using this under the hood
def colored_text(text: str, type: str, color: str):
    print(f"{color}{type}:{Style.RESET_ALL} {text}", flush=True)

def info(text: str):
    colored_text(text, "info", Fore.CYAN)

def warn(text: str):
    colored_text(text, "warn", Fore.YELLOW)

def error(text: str):
    colored_text(text, "error", Fore.RED)

def empty_entry_point():
    error("no entry point specified!")
    info("use `set_builder_entry_point` to set up an entry point")
    exit(1)

builder_entry_point = None
registered_entry_points = {
    "default_entry_point": empty_entry_point
}
suppressing_redefinition_warnings = False

def get_entry_point(name: str) -> typing.Callable:
    if name in registered_entry_points:
        return registered_entry_points[name]
    else:
        error(f"entry point {name} doesn't exist!")
        exit(1)

# suppress entry point redefinition warnings
def suppress_redefinition_warnings(value: bool = True):
    global suppressing_redefinition_warnings
    suppressing_redefinition_warnings = value

# manually registers entry point so it can be accessible through command line
def register_entry_point(name: str, function: typing.Callable, warn_on_redefinition=suppressing_redefinition_warnings, set_as_default=False):
    if name in registered_entry_points and warn_on_redefinition:
        warn(f"redefinition of {name} entry point")

    registered_entry_points[name] = function

    if set_as_default:
        global builder_entry_point
        builder_entry_point = name

def get_scope_functions():
    return dict(globals(), **locals())

def glob_files_gen(root: pathlib.Path, ext):
    for item in root.iterdir():
        yield item
        if item.is_dir():
            yield from glob_files_gen(item, ext)

# returns a list of all filed within some folder
def glob_files(root: str, ext: str | typing.List[str]):
    if type(ext) is list:
        acc = []
        for extension in ext:
            acc += glob_files(root, extension)

        return list(set(acc))

    files = list(glob_files_gen(pathlib.Path(root), ext))
    files = list(map(lambda x: x.__str__(), files))
    files = list(filter(lambda x: x.endswith("." + ext), files))
    return files

def path_exists(path: str):
    return os.path.exists(path)
    
def path_basename(path: str):
    return os.path.basename(path)

def mkdir(path: str):
    os.mkdir(path)

# ensures that .hash_build_files folder exists
def ensure_cache_folder_exists():
    if not path_exists(".hash_build_files/"):
        mkdir(".hash_build_files")

    if not path_exists(".hash_build_files/dependency_cache"):
        mkdir(".hash_build_files/dependency_cache")

ensure_cache_folder_exists()

build_arch = "64"
std_c = None
std_cxx = None

global_compiler_flags = []
global_linker_flags = []

global_include_paths = []

def set_build_arch(arch: str):
    global build_arch
    build_arch = arch

def set_std_c(standard: str):
    global std_c 
    std_c = standard

def set_std_cxx(standard: str):
    global std_cxx
    std_cxx = standard

def create_file(path: str):
    with open(path, "w+") as f:
        pass

def read_file(path: str):
    with open(path, "r+") as f:
        return f.read()
    
def write_file(path: str, content: str):
    with open(path, "w+") as f:
        f.write(content)

def file_directory(path: str):
    return os.path.dirname(path) + "/"

def file_extension(path: str):
    return path.split(".")[-1]

def filter_string_from_spaces(string: str) -> str:
    return " ".join(list(filter(lambda x: x != '', string.split(" "))))

def filter_list_from_empty_strings(l: typing.List[str]) -> typing.List[str]:
    return list(filter(lambda x: x != '', l))

# ensures that some file exists. if it doesn't exist, it creates it
def ensure_file_exists(path: str):
    if not path_exists(path):
        create_file(path)

# ensures that a special hash file exists for some path
def ensure_hash_file_exists(path: str) -> str:
    hash_path = f".hash_build_files/{hash_string(path)}.hash"
    ensure_file_exists(hash_path)
    return hash_path

# simple subprocess.run() wrapper
def spawn_subprocess(command: typing.List[str]) -> int:
    return subprocess.run(command, stdout=sys.stdout, stderr=sys.stderr).returncode

# wrapper around rmtree
def rmdir(path: str):
    shutil.rmtree(path)

# wrapper around mv
def mv(src: str, dst: str):
    shutil.move(src, dst)

# removes .hash_build_files folder which contains all the compiled source code and dependency scanner caches
def clean_cache():
    if path_exists(".hash_build_files/"):
        rmdir(".hash_build_files/")
    ensure_cache_folder_exists()

include_regex = re.compile(r"[\s+]?#[\s+]?include[\s+]?\".*\"")
include_file_path_regex = re.compile(r"\".*\"")

source_dependencies_cache = {}

# checks if the library is installed using pkg-config if ld
# availability of some libraries can't be checked through pkg-config
# so if pkg-config check failed we're using ld & cc to ensure that library is absent
def is_library_available(library: str):
    pkg_config_process = subprocess.run(['pkg-config', "--libs", library], capture_output=True, text=True)
    if pkg_config_process.returncode != 0:
        ld_process = subprocess.run(f'echo "int main(){{}}" | cc -o /dev/null -x c - -l{library} 2>/dev/null', shell=True)
        return ld_process.returncode == 0
    return pkg_config_process.returncode == 0

# calls pkg-config with some arguments and returns the result
def pkg_config_with_option(library: str, option: str):
    pkg_config_process = subprocess.run(['pkg-config', option, library], capture_output=True, text=True)
    if pkg_config_process.returncode == 0:
        return pkg_config_process.stdout.replace("\n", "")
    else:
        return None

# get linker flags required for using some library using pkg-config
def get_library(library: str):
    result = pkg_config_with_option(library, "--libs")
    if result is not None:
        return result
    else:
        error(f"library {library} is not available!")
        exit(1)

# get compiler flags required for using some library using pkg-config
def get_cflags(library: str):
    result = pkg_config_with_option(library, "--cflags")
    if result is not None:
        return result
    else:
        error(f"cflags for library {library} are not available!")
        exit(1)

# check if library is installed using pkg-config
# simply is a wrapper for is_library_available with some additional logic
def check_if_library_available(library: str):
    info(f"checking availability of {library}")
    if not is_library_available(library):
        error(f"library {library} is required for this project!")
        exit(1)

# parses some file and returns a list of detected includes (#include "...")
def get_source_file_header_dependencies(source_path: str) -> typing.List[str]:
    if not path_exists(source_path):
        return []
    if source_path in source_dependencies_cache:
        return list(set(source_dependencies_cache[source_path]))

    source_name_hash = hash_string(source_path)
    source_content_hash = hash_string(read_file(source_path))
    saved_dependencies_path = f".hash_build_files/dependency_cache/{source_name_hash}.dependencies"
    saved_source_hash_path = f".hash_build_files/dependency_cache/{source_name_hash}.hash"

    ensure_file_exists(saved_dependencies_path)
    ensure_file_exists(saved_source_hash_path)

    if read_file(saved_source_hash_path) == source_content_hash:
        saved_dependencies_string = read_file(saved_dependencies_path)
        try:
            saved_dependencies = eval(saved_dependencies_string)
            source_dependencies_cache[source_path] = saved_dependencies
            return saved_dependencies
        except:
            pass

    source_content = read_file(source_path)
    source_lines = source_content.split("\n")
    source_directory = file_directory(source_path)
    dependencies = []
    for line in source_lines:
        line = line.strip()
        if "#include" in line.replace(" ", ""):
            include_match = re.search(include_regex, line)
            if include_match is not None:
                include_file_path = re.search(include_file_path_regex, include_match.group(0))
                include_file_path = include_file_path.group(0).replace("\"", "")
                if path_exists(source_directory + include_file_path):
                    dependencies.append(source_directory + include_file_path)
                else:
                    for include_path in global_include_paths:
                        constructed_path = include_path + "/" + include_file_path
                        if path_exists(constructed_path):
                            dependencies.append(constructed_path)
                            break

    source_dependencies_cache[source_path] = list(set(dependencies))
    write_file(saved_source_hash_path, source_content_hash)
    write_file(saved_dependencies_path, str(dependencies))
    return list(set(dependencies))

      
def dependency_must_be_recompiled(dependency_path: str, dependency_holder: str, previous_dependencies: list[str] = []):
    previous_dependencies = list(set(previous_dependencies))
    dependency_hash_file = ensure_hash_file_exists(dependency_path + dependency_holder)

    dependency_source_hash = hash_string(read_file(dependency_path))
    saved_dependency_hash = read_file(dependency_hash_file)
    # print(dependency_source_hash + " == " + saved_dependency_hash, " | ", dependency_path)
    if dependency_source_hash != saved_dependency_hash:
        return True

    if dependency_path not in previous_dependencies:
        deep_dependencies = get_source_file_header_dependencies(dependency_path)
        for dependency in deep_dependencies:
            if dependency in previous_dependencies:
                continue
            if dependency_must_be_recompiled(dependency, dependency_holder, previous_dependencies + deep_dependencies):
                return True
    else:
        pass

    return False  

# recursively collects all includes (#include) of a file
def collect_all_dependencies(file: str, previous_acc: list[str] = []):
    acc = get_source_file_header_dependencies(file)
    for dependency in acc:
        if file in previous_acc:
            continue
        acc += collect_all_dependencies(dependency, acc)
        acc = list(set(acc))
    return acc

# compiles one single file using `cc` command (c/c++)
def compile_file(file: str, local_compiler_flags=[], progress=None, files_count=None, log_compiler_commands=False):
    if not path_exists(file):
        error(f"file {file} doesn't exist!")
        exit(1)
    name_hash = hash_string(file)
    hash_path = ensure_hash_file_exists(file)

    source_code_hash = hash_string(read_file(file))
    saved_source_code_hash = read_file(hash_path)
    object_file_path = f".hash_build_files/{name_hash}.o"
    file_dependencies = get_source_file_header_dependencies(file)

    file_must_be_recompiled = False
    if source_code_hash != saved_source_code_hash:
        file_must_be_recompiled = True

    for dependency in file_dependencies:
        if dependency_must_be_recompiled(dependency, file):
            file_must_be_recompiled = True
            break

    all_dependencies = collect_all_dependencies(file)

    if file_must_be_recompiled:
        architecture_flag = f"-m{build_arch}"
        compiler_flags = local_compiler_flags + global_compiler_flags
        compiler_command = []
        compiler_command.append("cc")
        compiler_command.append("-c")
        compiler_command.append(architecture_flag)
        compiler_command.append("-o")
        compiler_command.append(object_file_path)
        if std_cxx is not None:
            compiler_command.append(f"--std=c++{std_cxx}")
        elif std_c is not None:
            compiler_command.append(f"--std=c{std_c}")
        compiler_command.append(file)
        compiler_command += compiler_flags

        for include_path in global_include_paths:
            compiler_command.append(f"-I{include_path}")

        compilation_database.append({
            'directory': os.getcwd(),
            'arguments': compiler_command,
            'file': file,
            'output': object_file_path
        })

        compiler_command = filter_list_from_empty_strings(compiler_command)
        
        compilation_message = f"compiling {file} ({object_file_path})"
        if progress is not None and files_count is not None:
            compilation_message = f"[{progress}/{files_count}] " + compilation_message
        info(compilation_message)
        if log_compiler_commands:
            print(f"\t{' '.join(compiler_command)}")
        compiler_return_code = subprocess.run(" ".join(compiler_command), shell=True, stdout=sys.stdout, stderr=sys.stderr).returncode
        if compiler_return_code == 0:
            write_file(hash_path, source_code_hash)
            for dependency in all_dependencies:
                dependency_hash_file = ensure_hash_file_exists(dependency + file)
                write_file(dependency_hash_file, hash_string(read_file(dependency)))
                # print(f"saving {dependency} {hash_string(read_file(dependency))}")
        else:
            error("compiler returned non-zero code!")
            exit(1)
    else:
        compilation_message = f"no need to recompile {file} ({object_file_path})"
        if progress is not None and files_count is not None:
            compilation_message = f"[{progress}/{files_count}] " + compilation_message
        info(compilation_message)
    
    return object_file_path

# returns string in format "-Wl,-rpath,{path}"
def rpath(path: str) -> str:
    return f"-Wl,-rpath,{path}"

# calls compile_file on each file in files (list[str])
def compile_files(files: typing.List[str], local_compiler_flags=[], log_compiler_commands=False):
    if type(files) is str:
        files = [files]

    result = []
    for index, file in enumerate(files):
        result.append(compile_file(file, local_compiler_flags=local_compiler_flags, progress=index + 1, files_count=len(files), log_compiler_commands=log_compiler_commands))
    return result

def process_shared_library_name(name: str):
    s = f"{file_directory(name)}{shared_library_prefix}{path_basename(name)}.{shared_library_extension}"
    return s[1:] if s[0] == "/" or s[0] == "\\" else s

# links objects into a single executable or a shared library
# use cxx=True if your project depends on libstdc++
def link_objects(objects: typing.List[str], executable_name="executable", local_linker_flags=[], log_linker_commands=False, executable_type="executable", cxx=False):
    architecture_flag = f"-m{build_arch}"
    linker_flags = local_linker_flags + global_linker_flags
    building_shared_library = (executable_type == "shared")
    
    output_name = executable_name
    if building_shared_library:
        output_name = process_shared_library_name(output_name)
    
    linker_command = []
    linker_command.append("cc")
    linker_command.append(architecture_flag)
    if building_shared_library:
        linker_command.append("-shared")
    linker_command.append("-o")
    linker_command.append(output_name)

    for object in objects:
        linker_command.append(object)
    
    if cxx:
        linker_command.append("-lstdc++")

    for flag in linker_flags:
        linker_command.append(flag)

    linker_command = filter_list_from_empty_strings(linker_command)
    pretty_executable_type = "shared library" if building_shared_library else "executable"
    info(f"linking {pretty_executable_type} {output_name}")
    if log_linker_commands:
        print(f"\t{' '.join(linker_command)}")

    linker_return_code = subprocess.run(" ".join(linker_command), shell=True, stdout=sys.stdout, stderr=sys.stderr).returncode
    if linker_return_code != 0:
        error("linker returned non-zero code!")
        exit(1)

# helper funcion which calls link_objects() under the hood
def link_executable(objects: list[str], executable_name="executable", local_linker_flags=[], log_linker_commands=False, cxx=False):
    link_objects(objects=objects, executable_name=executable_name, local_linker_flags=local_linker_flags, log_linker_commands=log_linker_commands, executable_type="executable", cxx=cxx)

# helper function which calls link_objects(executable_type="shared") under the hood
def link_shared_library(objects: list[str], executable_name="executable", local_linker_flags=[], log_linker_commands=False, cxx=False):
    link_objects(objects=objects, executable_name=executable_name, local_linker_flags=local_linker_flags, log_linker_commands=log_linker_commands, executable_type="shared", cxx=cxx)

# helper function which calls link_executable(compile_files(files)) under the hood
def executable(files: list[str] | str, executable_name="", local_compiler_flags=[], local_linker_flags=[], log_compiler_commands=False, log_linker_commands=False, cxx=False):
    link_executable(compile_files(files, local_compiler_flags, log_compiler_commands), executable_name=executable_name, local_linker_flags=local_linker_flags, log_linker_commands=log_linker_commands, cxx=cxx)

# helper function which calls link_shared_library(compile_files(files)) under the hood
def shared_library(files: list[str] | str, executable_name="", local_compiler_flags=[], local_linker_flags=[], log_compiler_commands=False, log_linker_commands=False, cxx=False):
    link_shared_library(compile_files(files, local_compiler_flags, log_compiler_commands), executable_name=executable_name, local_linker_flags=local_linker_flags, log_linker_commands=log_linker_commands, cxx=cxx)

# decorator used to register entry points
def entry_point(original_function=None, default=False):
    def _decorate(function):
        register_entry_point(function.__name__, function, set_as_default=default)
        return function

    if original_function:
        return _decorate(original_function)

    return _decorate

# enables entry points support. without this function you won't be able to use any entry points at all
def router():
    if arguments["clean"] is True:
        clean_cache()
        return

    if not arguments["entry_point"] is None:
        global builder_entry_point
        builder_entry_point = arguments["entry_point"]

    if builder_entry_point is None:
        empty_entry_point()
    else:
        get_entry_point(builder_entry_point)()

    if arguments["database"]:
        ensure_file_exists("compilation_commands.json")
        write_file("compilation_commands.json", str(compilation_database))

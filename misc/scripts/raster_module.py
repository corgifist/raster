from hash_build_env import *
import os
import hashlib
import shutil

def list_nth(array, index):
    return array[int(index)]

def unsafe_arg_wrapper(func, args):
    return func(*args)

def try_scenario(try_logic, catch_logic, exception_msg):
    global arg_wrapper, unsafe_arg_wrapper
    reserved_arg_wrapper = arg_wrapper
    try:
        arg_wrapper = unsafe_arg_wrapper
        call_scenario(try_logic)
        arg_wrapper = reserved_arg_wrapper
    except Exception as e:
        var_env[exception_msg] = str(e)
        arg_wrapper = reserved_arg_wrapper
        call_scenario(catch_logic)

def list_len(array):
    return len(array)

def greater(a, b):
    return float(a) > float(b)

def less(a, b):
    return float(a) < float(b)

def write_build_number():
    bn_content = open("include/build_number.h", "r").read()
    with open("include/build_number.h", "w+") as bn_file:
        print(bn_content)
        bn_lines = bn_content.split("\n")
        bn_second_parts = bn_lines[1].split(" ")
        incremented_number = int(bn_second_parts[2]) + 1
        bn_file.write("#pragma once\n#define BUILD_NUMBER " + str(incremented_number))

def hash_(s):
    return hashlib.md5(s.encode("utf-8")).hexdigest()

def string_replace(string, subject, replacement):
    return string.replace(subject, replacement)


functions["greater"] = greater
functions["less"] = less
functions["list_len"] = list_len
functions["list_nth"] = list_nth
functions["try_scenario"] = try_scenario
functions["write_build_number"] = write_build_number
functions["hash"] = hash_
functions["string_replace"] = string_replace
functions["rmdir"] = shutil.rmtree
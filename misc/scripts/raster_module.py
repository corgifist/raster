from hash_build_env import *

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


functions["greater"] = greater
functions["less"] = less
functions["list_len"] = list_len
functions["list_nth"] = list_nth
functions["try_scenario"] = try_scenario
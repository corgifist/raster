import hash_build_env
from hash_build_interpreter import *
from hash_build_common import *


def interpret_master(block):
    for node in block:
        interpret(node)

def interpret(node):
    if (len(node)) == 0:
        return node

    if node[0] == 'assign':
        # [assign, name, value]
        value = interpret(node[2])
        hash_build_env.var_env[node[1]] = value
        return value
    elif node[0] == 'scenario':
        # [scenario, name, ast]
        hash_build_env.scenarios[node[1]] = node[2]
    elif node[0] == 'call':
        # [call, callee, args]
        calle_key = node[1]
        interpreted_args = list()
        for arg in node[2]:
            interpreted_args.append(interpret(arg))
        if not calle_key in hash_build_env.functions:
            fatal(f"unknown function {calle_key}")
            exit(1)
        ret_val = hash_build_env.arg_wrapper(hash_build_env.functions[calle_key], interpreted_args)
        return ret_val
    elif node[0] == 'var':
        # [var, var_name]
        try:
            return hash_build_env.var_env[node[1]]
        except KeyError:
            fatal(f"hash_build_interpreter: no variable named {node[1]}")
            exit(1)
    elif node[0] == 'if':
        # [if, expr, block, else_block]
        if interpret(node[1]):
            interpret_master(node[2])
        elif node[3] != None:
            interpret_master(node[3])
    elif node[0] == 'list':
        transformed_list = list()
        for elem in node[1]:
            transformed_list.append(interpret(elem))
        return transformed_list
    
    return node

hash_build_env.interpreter_proc = interpret_master
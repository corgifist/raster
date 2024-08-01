import ply.yacc as yacc
from hash_build_common import *
from hash_build_lexer import tokens

root_expr = 'scenario'

def p_script(node):
    '''script : if script
              | if'''
    if len(node) == 2:
        node[0] = [node[1]]
    else:
        node[0] = [node[1]] + node[2]

def p_if(node):
    '''if : IF assignment block
          | IF assignment block ELSE block
          | assignment'''
    if len(node) == 2:
        node[0] = node[1]
    elif len(node) == 4:
        node[0] = ['if', node[2], node[3], None]
    else:
        node[0] = ['if', node[2], node[3], node[5]]

def p_assignment(node):
    '''assignment : ATOM EQ call
                  | call'''
    if len(node) == 2:
        node[0] = node[1]
    else:
        node[0] = ['assign', node[1], node[3]]

def p_call(node):
    '''call : ATOM LPAREN call_args RPAREN
            | ATOM LPAREN RPAREN
            | list'''
    if len(node) == 2:
        node[0] = node[1]
    elif len(node) == 4:
        node[0] = ['call', node[1], []]
    else:
        node[0] = ['call', node[1], node[3]]

def p_list(node):
    '''list : LBRACKET RBRACKET
            | LBRACKET call_args RBRACKET
            | scenario'''
    if len(node) == 3:
        node[0] = ['list', []]
    elif len(node) == 2:
        node[0] = node[1]
    else:
        node[0] = ['list', node[2]]
    

def p_scenario(node):
    '''scenario : SCENARIO ATOM block
                | basic_expr'''
    if len(node) == 2:
        node[0] = node[1]
    else:
        node[0] = ['scenario', node[2], node[3]]


def p_block(node):
    '''block : LBRACE script RBRACE
             | LBRACE RBRACE'''
    if len(node) == 3:
        node[0] = []
    else:
        node[0] = node[2]

def p_call_args(node):
    '''call_args : script COMMA call_args 
                 | script'''
    if len(node) == 2:
        node[0] = node[1]
    else:
        node[0] = node[1] + node[3]


def p_basic_expr(node):
    '''basic_expr : STRING
                  | ATOM
                  | DOLLAR ATOM'''
    if len(node) == 3:
        node[0] = ['var', node[2]]
    else:
        node[0] = node[1]

def p_error(p):
    fatal("hash_build_parser: parsing error in input " + str(p))

def build_parser():
    return yacc.yacc()

def parse(string):
    return build_parser().parse(string)
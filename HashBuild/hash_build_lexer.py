import ply.lex as lex
from hash_build_common import *

tokens = (
    'ATOM',
    'LBRACE',
    "RBRACE",
    'LPAREN',
    'RPAREN',
    'STRING',
    'COLON',
    'EQ',
    'COMMA',
    'COMMENT',
    'SCENARIO',
    'DOLLAR',
    'IF',
    'ELSE',
    'LBRACKET',
    'RBRACKET'
)


t_LBRACE    = r'\{'
t_RBRACE    = r'\}'
t_RPAREN    = r'\)'
t_LPAREN    = r'\('
t_COLON     = r'\:'
t_EQ        = r'\='
t_COMMA     = r'\,'
t_DOLLAR    = r'\$'

t_LBRACKET  = r'\['
t_RBRACKET  = r'\]'

def t_COMMENT(t):
    r'\#.*'
    pass

def t_ATOM(t):
    r'[a-zA-Z_\!0-9]+'
    if t.value == 'scenario':
        t.type = 'SCENARIO'
    elif t.value == 'if':
        t.type = 'IF'
    elif t.value == 'else':
        t.type = 'ELSE'
    return t

def t_STRING(t):
    r'\"(\\.|[^\"])*\"'
    t.value = str(t.value).replace('"', '')
    return t

def t_newline(t):
    r'\n+'
    t.lexer.lineno += len(t.value)

t_ignore = ' \t'

def t_error(t):
    fatal(f"hash_build_lexer: illegal character '{t.value[0]}'")
    t.lexer.skip(1)

def build_lexer(input):
    lexer = lex.lex()
    lexer.input(input)
    return lexer

def fetch_all_tokens(lexer):
    acc = list()
    while True:
        tok = lexer.token()
        if not tok:
            break
        acc.append(tok)
    return acc
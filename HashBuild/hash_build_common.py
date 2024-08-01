import colorama
import hashlib

def fatal(msg):
    print(colorama.Fore.RED + "fatal: " + colorama.Fore.RESET + msg)

def info(msg):
    print(colorama.Fore.CYAN + "info: " + colorama.Fore.RESET + msg)

def warning(msg):
    print(colorama.Fore.YELLOW + "warning: " + colorama.Fore.RESET + msg)


def hash_string(string):
    return hashlib.blake2s(string.encode()).hexdigest()
#!/usr/bin/env python

import pdb
import ply.lex as lex
import sys

reserved = {
    "book": "BOOK",
    "dcf": "DCF",
    "ddm": "DDM",
    "discrate": "DISCRATE",
    "erp": "ERP",
    "earnings": "EARNINGS",
    "graham": "GRAHAM",
    "growth": "GROWTH",
    "multiple": "MULTIPLE",
    "no_growth_pe": "NO_GROWTH_PE",
    "relative": "RELATIVE",
    "revenue": "REVENUE",
    "rfr": "RFR",
    "tm": "TERMINAL_MULT",
    "weight": "WEIGHT",
    "years": "YEARS"
}

tokens = ["COMMENT", "NUMBER", "ID"] + list(reserved.values())

def t_ID(t):
    r"[a-zA-Z]\w*"
    t.type = reserved.get(t.value, "ID")
    return t

def t_NUMBER(t):
    r"[-+]?\d*\.\d+|[-+]?\d+"
    t.value = float(t.value)
    return t

# Special rules
t_ignore  = " \t"
t_ignore_COMMENT = r'\#.*'
def t_error(t):
    print "Illegal character '%s'" % t.value[0]
    sys.exit(-1)
def t_newline(t):
    r"\n+"
    t.lexer.lineno += len(t.value)

lexer = lex.lex(optimize=1, lextab="modeltab")

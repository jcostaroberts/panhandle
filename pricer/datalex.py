#!/usr/bin/env python

import ply.lex as lex
import sys

tokens = (
    "BETA",
    "BOOK",
    "CAPEX",
    "CASH",
    "CHANGE_AP",
    "CHANGE_AR",
    "CHANGE_INV",
    "COMMENT",
    "D_AND_A",
    "DEBT",
    "DEFAULT_SPREAD",
    "DIVIDEND",
    "EARNINGS",
    "EPS",
    "EQUITY",
    "INTEREST_EXPENSE",
    "MKTCAP",
    "MILLIONS",
    "NUMBER",
    "PRINCIPAL_REPAID",
    "REVENUE",
    "SHARES",
    "TAX_RATE",
    "THOUSANDS",
    "UNIT"
)


t_BETA = r"beta"
t_BOOK = r"book"
t_CAPEX = r"capex"
t_CASH = r"cash"
t_CHANGE_AP = r"change_ap"
t_CHANGE_AR = r"change_ar"
t_CHANGE_INV = r"change_inv"
t_ignore_COMMENT = r'\#.*'
t_D_AND_A = r"d_and_a"
t_DEBT = r"debt"
t_DEFAULT_SPREAD = r"default_spread"
t_DIVIDEND = r"dividend"
t_EARNINGS = r"earnings"
t_EPS = r"eps"
t_EQUITY = r"equity"
t_INTEREST_EXPENSE = r"interest_expense"
t_MILLIONS = r"millions"
t_MKTCAP = r"mktcap"
def t_NUMBER(t):
    r"[-+]?\d*\.\d+|[-+]?\d+"
    t.value = float(t.value)
    return t
t_PRINCIPAL_REPAID = r"principal_repaid"
t_REVENUE = r"revenue"
t_SHARES = r"shares"
t_TAX_RATE = r"tax_rate"
t_THOUSANDS = r"thousands"
t_UNIT = r"unit"

# Special rules
t_ignore  = " \t"
def t_error(t):
    print("Illegal character '%s'" % t.value[0])
    sys.exit(-1)
    #t.lexer.skip(1)
def t_newline(t):
    r"\n+"
    t.lexer.lineno += len(t.value)

lexer = lex.lex(optimize=1, lextab="datatab")

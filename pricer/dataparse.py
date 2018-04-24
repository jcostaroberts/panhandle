#!/usr/bin/env python

import pdb
import ply.yacc as yacc

from datalex import tokens
from pricertypes import Data

# unit [thousands|millions]
# measure NUMBER
# measure NUMBER
# ...

def p_data(p):
    "data : units datalines"
    p[0] = Data(p[1], p[2])

def p_units(p):
    "units : UNIT multiple"
    p[0] = p[2]

def p_multiple_thousands(p):
    "multiple : THOUSANDS"
    p[0] = 1000

def p_multiple_millions(p):
    "multiple : MILLIONS"
    p[0] = 1000000

def p_datalines_dataline(p):
    "datalines : dataline"
    p[0] = p[1]

def p_datalines_datalines(p):
    "datalines : datalines dataline"
    p[0] = dict(p[1].items() + p[2].items())

def p_dataline(p):
    "dataline : metric NUMBER"
    p[0] = {p[1]: p[2]}

def p_metric(p):
    """metric : DIVIDEND
              | EARNINGS
              | EPS
              | D_AND_A
              | CAPEX
              | CHANGE_INV
              | CHANGE_AR
              | CHANGE_AP
              | PRINCIPAL_REPAID
              | INTEREST_EXPENSE
              | DEFAULT_SPREAD
              | TAX_RATE
              | EQUITY
              | DEBT
              | MKTCAP
              | CASH
              | SHARES
              | BOOK
              | BETA
              | REVENUE"""
    p[0] = p[1]

def p_error(p):
    if p:
        print "Syntax error at token %s" % p
        sys.exit(-1)
    else:
        print "Syntax error at EOF"
        sys.exit(-1)

parser = yacc.yacc(tabmodule="dataparsetab")

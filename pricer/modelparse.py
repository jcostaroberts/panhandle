#!/usr/bin/env python

import pdb
import ply.yacc as yacc
import sys

from modellex import tokens
from pricertypes import Dcf, Ddm, Graham, Relative, EnsembleValuation

# <valuation_id> weight NUMBER dcf growth NUMBER rfr NUMBER erp NUMBER years NUMBER [tm NUMBER]
# <valuation_id> weight NUMBER ddm growth NUMBER discrate NUMBER years NUMBER
# <valuation_id> weight NUMBER graham growth NUMBER rfr NUMBER no_growth_pe NUMBER
# <valuation_id> weight NUMBER relative [book|earnings|revenue] multiple NUMBER

def range_check(num, m, mn=None, mx=None):
    if mn and num < mn:
        print "%s must be >= %f" % (m, mn)
        sys.exit(-1)
    if mx and num > mx:
        print "%s must be <= %f" % (m, mx)
        sys.exit(-1)

def p_ensembles(p):
    "ensembles : models"
    pt = {}
    for model in p[1]:
        if model.valuation_id not in pt:
            pt[model.valuation_id] = EnsembleValuation(model.valuation_id)
        pt[model.valuation_id].valuation.append(model)
    p[0] = pt

def p_models_model(p):
    "models : model"
    p[0] = [p[1]]

def p_models_models(p):
    "models : models model"
    p[0] = p[1] + [p[2]]

def p_model(p):
    "model : ID WEIGHT NUMBER method"
    p[4]._set_id_weight(p[1], p[3])
    p[0] = p[4]

def p_method_dcf(p):
    "method : DCF GROWTH NUMBER RFR NUMBER ERP NUMBER YEARS NUMBER terminal_multiplier"
    range_check(p[3], "growth", mn=-1, mx=1)
    range_check(p[5], "rfr", mn=-1, mx=1)
    range_check(p[7], "erp", mn=-1, mx=1)
    range_check(p[9], "years", mn=0)
    if p[10]:
        range_check(p[10], "tm", mn=0)
    p[0] = Dcf(p[3], p[5], p[7], p[9], p[10])

def p_terminal_multiplier_number(p):
    "terminal_multiplier : TERMINAL_MULT NUMBER"
    p[0] = p[2]

def p_terminal_multiplier_none(p):
    "terminal_multiplier :"
    p[0] = None

def p_method_ddm(p):
    "method : DDM GROWTH NUMBER DISCRATE NUMBER YEARS NUMBER"
    range_check(p[3], "growth", mn=-1, mx=1)
    range_check(p[5], "discrate", mn=-1, mx=1)
    range_check(p[7], "years", mn=0)
    p[0] = Ddm(p[3], p[5], p[7])

def p_method_relative(p):
    "method : RELATIVE metric MULTIPLE NUMBER"
    p[0] = Relative(p[2], p[4])

def p_method_graham(p):
    "method : GRAHAM GROWTH NUMBER RFR NUMBER NO_GROWTH_PE NUMBER"
    range_check(p[3], "growth", mn=-1, mx=1)
    range_check(p[5], "rfr", mn=-1, mx=1)
    p[0] = Graham(p[3], p[5], p[7])

def p_metric(p):
    """metric : BOOK
              | EARNINGS
              | REVENUE"""
    p[0] = p[1]

def p_error(p):
    if p:
        print "Syntax error at token %s" % p
        sys.exit(-1)
    else:
        print "Syntax error at EOF"
        sys.exit(-1)

parser = yacc.yacc(tabmodule="modelparsetab")
#pdb.set_trace()

#!/usr/bin/env python

import argparse
import datalex
import dataparse
import modellex
import modelparse
import pdb

from prettytable import PrettyTable
from pricertypes import ValuationInfo

def print_tables(vi):
    print "\n"
    for en in vi.ensemble:
        t = PrettyTable(["Method", "Weight", "Value"])
        t.float_format = ".2"
        t.align["Method"] = "l"

        # adjust valuation weights if necessary
        weight_sum = sum([k.weight for k in vi.ensemble[en].valuation])
        weight_mult = 1.0/weight_sum
        for v in vi.ensemble[en].valuation:
            v.weight *= weight_mult

        # hack: run valuations to get inferred values for notes
        map(lambda x: x.value(vi.data), vi.ensemble[en].valuation)
        
        # get weighted valuations
        vals = map(lambda x: [x.method + " (%s)" % x.notes(vi.data), x.weight, x.value(vi.data)],
                   vi.ensemble[en].valuation)
        total = sum(map(lambda x: x[1]*x[2], vals))
        map(lambda x: t.add_row(x), vals)
        t.add_row(["Total", 1.0, total])

        # PrettyTable voodoo to get a line above the "Total" row
        t._compute_widths(t._format_rows(t._rows, t._get_options({})), t._get_options({}))
        t.del_row(-1)
        t.add_row(map(lambda x: "-"*x, t._widths))
        t.add_row(["Total", 1.0, total])

        # Print table
        print "Valuation: %s" % en
        print t
        print "\n"

def handle_modelfile(modelfile, vi):
    f = open(modelfile, "r")
    txt = f.read()
    f.close()
    pt = modelparse.parser.parse(txt, lexer=modellex.lexer)
    vi.ensemble = pt

def handle_datafile(datafile, vi):
    f = open(datafile, "r")
    txt = f.read()
    f.close()
    pt = dataparse.parser.parse(txt, lexer=datalex.lexer)
    vi.data = pt

def handle_input(datafile, modelfile):
    vi = ValuationInfo()
    handle_modelfile(modelfile, vi)
    handle_datafile(datafile, vi)
    return vi

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--datafile", type=str, required=True,
                        help="File containing data for valuation")
    parser.add_argument("-m", "--modelfile", type=str, required=True,
                        help="File containing model descriptions and weights")
    args = parser.parse_args()
    return (args.datafile, args.modelfile)

def main():
    print_tables(handle_input(*parse_args()))

if __name__ == "__main__":
    main()

#!/usr/bin/env python

import sys

def check_data(data, metric, needed_by):
    if metric not in data.metrics:
        print "Metric '%s' needed by %s not in datafile" % (metric, needed_by)
        sys.exit(-1)

class Data(object):
    def __init__(self, units, metrics):
        self.units = units
        self.metrics = metrics

class Valuation(object):
    def __init__(self, method):
        self.method = method
    
    def _set_id_weight(self, valuation_id, weight):
        self.valuation_id = valuation_id
        self.weight = weight
    
    def value(self, data):
        raise NotImplementedError
    
    def notes(self, data):
        raise NotImplementedError

class Dcf(Valuation):
    def __init__(self, growth, rfr, erp, years, tm=None):
        Valuation.__init__(self, "dcf")
        self.growth = growth
        self.rfr = rfr
        self.erp = erp
        self.years = years
        self.tm = tm
        self._tm = None
    
    def value(self, data):
        check_data(data, "earnings", self.method)
        check_data(data, "d_and_a", self.method)
        check_data(data, "capex", self.method)
        check_data(data, "change_inv", self.method)
        check_data(data, "change_ar", self.method)
        check_data(data, "change_ap", self.method)
        check_data(data, "principal_repaid", self.method)
        check_data(data, "interest_expense", self.method)
        check_data(data, "beta", self.method)
        check_data(data, "default_spread", self.method)
        check_data(data, "tax_rate", self.method)
        check_data(data, "equity", self.method)
        check_data(data, "debt", self.method)
        check_data(data, "mktcap", self.method)
        check_data(data, "cash", self.method)
        check_data(data, "shares", self.method)
        return self._value(self.years, data.metrics["earnings"], data.metrics["d_and_a"],
                           data.metrics["capex"], data.metrics["change_inv"],
                           data.metrics["change_ar"], data.metrics["change_ap"],
                           data.metrics["principal_repaid"], data.metrics["interest_expense"],
                           self.rfr, data.metrics["beta"], self.erp,
                           data.metrics["default_spread"], data.metrics["tax_rate"],
                           data.metrics["equity"], data.metrics["debt"], self.growth,
                           data.metrics["mktcap"], data.metrics["cash"], data.metrics["shares"],
                           self.tm)
    
    def _value(self, periods, earnings, depreciation, capex, delta_inv, delta_ar, delta_ap,
               principal_repaid, int_ex, rfr, beta, erp, default_spread,
               tax_rate, equity, debt, growth_rate, mktcap, cash, shares,
               terminal_multiple=None):
        fcf = earnings + depreciation - (capex + delta_inv + delta_ar + delta_ap +
              principal_repaid + int_ex)
        coe = rfr + beta*erp
        after_tax_cod = (rfr + default_spread)*(1 - tax_rate)
        wacc = coe*(equity/(equity + debt)) + after_tax_cod*(debt/(equity + debt))
        tev_to_fcf = (mktcap + debt - cash)/fcf

        # FCF sum
        iv = last = fcf
        for i in range(1, int(periods)+1):
            last = (last*(1+growth_rate))
            discounted = last/(1 + wacc)**i
            iv += discounted
        terminal_multiple = terminal_multiple or tev_to_fcf
        self._tm = terminal_multiple
        terminal_val = terminal_multiple*last
        iv += terminal_val/(1 + wacc)**i

        return iv/shares

    def notes(self, data):
        return "growth=%.2f%%, rfr=%.2f%%, erp=%.2f%%, years=%d, tm=%.2f" % \
               (self.growth*100, self.rfr*100, self.erp*100, int(self.years), self._tm)

class Ddm(Valuation):
    def __init__(self, growth, disc_rate, years):
        Valuation.__init__(self, "ddm")
        self.growth = growth
        self.disc_rate = disc_rate
        self.years = years

    def value(self, data):
        check_data(data, "dividend", self.method)
        return self._value(data.metrics["dividend"], self.growth, self.disc_rate, self.years)

    def _value(self, dividend, growth, disc_rate, periods):
        p = 0
        for i in range(1, int(periods)+1):
            p += dividend*((1+growth)**i)/((1+disc_rate)**i)
        return p

    def notes(self, data):
        return "growth=%.2f%%, discount=%.2f%%, years=%d" % \
               (self.growth*100, self.disc_rate*100, int(self.years))

class Relative(Valuation):
    def __init__(self, metric, multiple):
        Valuation.__init__(self, "relative")
        self.metric = metric
        self.multiple = multiple

    def value(self, data):
        check_data(data, self.metric, self.method)
        check_data(data, "shares", self.method)
        return self._value(self.multiple, data.metrics[self.metric],
                           data.metrics["shares"])
    
    def _value(self, target_multiple, metric, shares):
        return (metric/shares)*target_multiple
    
    def notes(self, data):
        return "metric=%s, multiple=%.2f" % (self.metric, self.multiple)

class Graham(Valuation):
    def __init__(self, growth, rfr, no_growth_pe):
        Valuation.__init__(self, "graham")
        self.growth = growth
        self.rfr = rfr
        self.no_growth_pe = no_growth_pe

    def value(self, data):
        check_data(data, "eps", self.method)
        check_data(data, "mktcap", self.method)
        check_data(data, "shares", self.method)
        v = data.metrics["eps"]*(self.no_growth_pe+200*self.growth)
        return (v*4.4)/(100*self.rfr)

    def notes(self, data):
        pps = data.metrics["mktcap"]/data.metrics["shares"]
        implied_growth = (pps*(self.rfr*100)/(4.4*data.metrics["eps"])) - \
                          (self.no_growth_pe*100)/2.0
        return "growth=%.2f%%, rfr=%.2f%%, implied growth=%.2f%%" % \
               (self.growth*100, self.rfr*100, implied_growth)

class EnsembleValuation(object):
    valuation_id = None
    def __init__(self, valuation_id):
        self.valuation_id = valuation_id
        self.valuation = []

# Should contain a collection of EmsembleValuations + Data
class ValuationInfo(object):
    ensemble = {}
    data = None

A program for valuation.

### Usage:

```
% ./pricer.py [-h] -d DATAFILE -m MODELFILE
```

### Modelfiles:

Each final valuation is identified by a name, and may be an ensemble valuation, i.e., a valuation comprised of a set of weighted sub-valuations. For example, you might have an ensemble valuation named "pessimistic" that's made up of three sub-valuations: a discounted cashflow model (weighted 60%, say), a dividend discount model (weighted 20%), and a multiple of earnings (weighted 20%). All sub-valuations are specified along with model parameters (e.g., assumed growth rates) in a modelfile whose location is given to pricer.py as an argument. The sub-valuations are specified using a strict grammar, as defined in modellex.py and modelparse.py.

Lines in these modelfiles may take any of the following forms:

```
# weight: How much to weight valuation in ensemble valuation
# dcf: Discounted cashflow model
# growth: Assumed growth rate
# rfr: Risk-free rate, e.g., the 10-year Treasury yield
# erp: Equity risk premium
# years: Number of years to model
# tm: Assumed terminal EV/FCF multiple
VALUATION_ID weight NUMBER dcf growth NUMBER rfr NUMBER erp NUMBER years NUMBER [tm NUMBER]

# weight: How much to weight valuation in ensemble valuation
# ddm: Dividend discount model
# growth: Assumed growth rate
# discrate: Discount rate
# years: Number of years to model
VALUATION_ID weight NUMBER ddm growth NUMBER discrate NUMBER years NUMBER

# weight: How much to weight valuation in ensemble valuation
# relative: Valuation based on a multiple of specified metric
# multiple: Multiple to apply to the specified metric
VALUATION_ID weight NUMBER relative [book|earnings|revenue] multiple NUMBER

# weight: How much to weight valuation in ensemble valuation
# graham: Graham model
# growth: Assumed growth rate
# rfr: Risk-free rate, e.g., the 10-year Treasury yield
# no_growth_pe: P/E used for no-growth company (Graham uses 8.5)
VALUATION_ID weight NUMBER graham growth NUMBER rfr NUMBER no_growth_pe NUMBER
```

The modelfile for the "pessimistic" valuation described above might look as follows:

```
pessimistic weight 0.6 dcf growth -0.01 rfr 0.025 erp 0.045 years 10 tm 15
pessimistic weight 0.2 ddm growth -0.01 discrate 0.06 years 25
pessimistic weight 0.2 relative earnings multiple 13.5
```

### Datafiles:

The valuation models need input data. These data go in a datafile whose location is also given to pricer.py as an argument. The datafile's format is as follows:

```
unit [thousands|millions]

METRIC NUMBER
METRIC NUMBER
...
METRIC NUMBER

# METRIC can be any of the following:
# beta
# book
# capex
# cash
# change_ap (change in accounts payable)
# change_ar (change in accounts receivable)
# change_inv (change in inventory)
# d_and_a (depreciation and amortization)
# debt
# default_spread
# dividend
# earnings
# eps
# equity
# interest_expense
# mktcap
# principal_repaid
# revenue
# shares
# tax_rate
```

### Output:

```
Valuation: pessimistic
+---------------------------------------------------------------+--------+-------+
| Method                                                        | Weight | Value |
+---------------------------------------------------------------+--------+-------+
| dcf (growth=-1.00%, rfr=2.50%, erp=4.50%, years=10, tm=15.00) |  0.60  | 42.34 |
| ddm (growth=-1.00%, discount=6.00%, years=25)                 |  0.20  | 16.44 |
| relative (metric=earnings, multiple=13.50)                    |  0.20  | 44.38 |
| ------------------------------------------------------------- | ------ | ----- |
| Total                                                         |  1.00  | 34.39 |
+---------------------------------------------------------------+--------+-------+
```

### Adding functionality:

To add a metric: (1) Update the tokens structure in datalex.py and add a regular expression to the metric. (2) Update p_metric in dataparse.py. (3) Add logic to use the new metric in pricertypes.py. (4) Update the list of metrics above. (5) Update the list of metrics in templatedata.txt.

To change or add a model specification: (1) Add the token and regular expression to modellex.py. (2) Update modelparse.py to use the new token. (3) Update the appropriate value() method to use the new specification. (4) Update the list above. (5) Update the list of models in templatemodel.txt.


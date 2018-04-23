#include <argp.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#define U16 unsigned short
#define UNS DBL_MIN
#define MAX_STRLEN 30
#define MAX_PERIODS 12

#define ZERO(x) (x == 0.0)

/* macros for setting/getting input_series types */
#define ISET(x,y) if ((x).e < MAX_PERIODS) (x).v[(x).e++] = y;
#define IV(x,i) ((x).v[i])
#define IV0(x) ((x).v[0]) // NOTE: Assumes the first argument is most recent
#define IE(x) ((x).e)
#define QTR(f,e) ((f)->period*IE(e))

/* macros for setting/getting metric types */
#define MSET(X,V,Q) {(X).v=V; (X).q=Q; (X).set=1;}
#define MV(x) ((x).v)
#define MQ(x) ((x).q)
#define MBLANK(x) ((x).set == 0)

/* macros for getting the minimum duration of a series of values */
#define MIN(A,B) (A < B ? B : A)
#define MINQTR2(P,A,B) (P*MIN(IE(A),IE(B)))
#define MINQTR3(P,A,B,C) (MIN(P*IE(A),MINQTR2(P,B,C)))
#define MINQTR4(P,A,B,C,D) (MIN(MINQTR2(P,A,B),MINQTR2(P,C,D)))

/* To add a new metric: (1) add a field to the metrics struct; (2) add
 * the computation to compute_metrics; (3) add the metric to the show routine.
 *
 * To add a new input: (1) add a flag to the argp_option array; (2) add
 * a field to the financials struct; (3) parse the option in parse_opt; */

// TODO: ADD MARGINS

static struct argp_option options[] = {
    {"name", 'n', "NAME", 0, "Company name"},
    {"mktcap", 'P', "NUM", 0,  "Market cap"},
    {"price",  'p', "NUM", 0,  "Share price"},
    {"ev", 'v', "NUM", 0, "Enterprise value"},
    {"div", 'D', "NUM", 0, "Dividend"},
    {"div-ps", 'd', "NUM", 0, "Dividend per share"},
    {"reporting-period", 'q', "NUM 1-4", 0, "Reporting period"},
    {"earn-ps", 'e', "NUM", 0, "One period of earnings"},
    {"earn", 'E', "NUM", 0, "One period of per-share earnings"},
    {"assets", 'A', "NUM", 0, "Assets"},
    {"current-assets", 'a', "NUM", 0, "Current assets"},
    {"intangibles", 'i', "NUM", 0, "Intangibles"},
    {"goodwill", 'g', "NUM", 0, "Goodwill"},
    {"liabilities", 'L', "NUM", 0, "Liabilities"},
    {"current-liabilities", 'l', "NUM", 0, "Current liabilities"},
    {"std", 't', "NUM", 0, "Short-term debt"},
    {"ltd", 'T', "NUM", 0, "Long-term debt"},
    {"inventories", 'I', "NUM", 0, "Inventories"},
    {"cash", 'c', "NUM", 0, "Cash"},
    {"cfo", 'C', "NUM", 0, "Cash flow from operating activities"},
    {"minority-interest", 'm', "NUM", 0, "Minority interest"},
    {"ebitda", 'b', "NUM", 0, "EBITDA"},
    {"ebit", 'B', "NUM", 0, "EBIT/Operating profit"},
    {"d-and-a", 'r', "NUM", 0, "Depreciation and amortization"},
    {"capex", 'x', "NUM", 0, "Capital expenditure"},
    {"tax", 'X', "NUM", 0, "Cash taxes"},
    {0}
};

typedef struct input_series_dbl {
    double v[MAX_PERIODS]; /* entries */
    U16 e;                 /* number of entries */
} in_series;

typedef struct financials {
    char name[MAX_STRLEN+1];
    char mktcap_str[MAX_STRLEN+1];
    double price;
    double mktcap;
    double ev;
    U16 period;
    in_series div_ps;
    in_series div;
    in_series earn_ps;
    in_series earn;
    in_series cfo;
    in_series ebit;
    in_series d_and_a;
    in_series capex;
    in_series ebitda;
    in_series assets;
    in_series intangibles;
    in_series goodwill;
    in_series liabilities;
    in_series stdebt;
    in_series ltdebt;
    in_series curr_assets;
    in_series curr_liabilities;
    in_series inventories;
    in_series cash;
    in_series minority_int;
    in_series tax;
} financials;

typedef struct metric_dbl {
    double v; /* metric value */
    U16 q;    /* number of quarters used to compute this metric */
    U16 set;    /* has this structure had its value set? */
} metric;

typedef struct metrics {
    double ev;
    metric earnings;
    metric ebitda;
    metric fcf; // EBITDA minus capex minus tax
    metric cfo; // Cash flow from operating activities
    metric ev_to_ebitda;
    metric p_to_e;
    metric p_to_fcf;
    metric p_to_cfo;
    metric book;
    metric p_to_b;
    metric p_to_c;
    metric div_yield;
    metric div_cover;
    metric roc; // Pre-tax return on capital
    metric roe; // Return on equity
    metric current;
    metric quick;
    metric d_to_e;
    metric l_to_e;
    metric l_to_ebitda;
    metric std_to_d;
    metric ltd_to_d;
} metrics;

/* Convert a string matching <float>[t|m|b] to a double. So,
 * e.g., 123.4m becomes 123400000.0 and -1.3t becomes -1300.0.
 * Assumes a null-terminated string. */
double
mstrtod(char *s) {
    char m = s[strlen(s)-1];
    char *t;
    double f = strtod(s, &t);

    switch(m) {
        case 't':
        case 'T':
            f *= 1e3;
            break;
        case 'm':
        case 'M':
            f *= 1e6;
            break;
        case 'b':
        case 'B':
            f *= 1e9;
            break;
        default:
            if (*t != '\0') {
                printf("Invalid amount: %s, t=%c\n", s, *t);
                exit(-1);
            }
            break;
    }
    return f;
}

/* Convert a double to an easily read string. */
int
dtomstr(double f, char *s) {
    char c = 't';
    double g = f;
    if (f >= 1e3 && f < 1e6)
        g /= 1e3;
    else if (f < 1e9) {
        c = 'm';
        g /= 1e6;
    } else if (f >= 1e9 && f < 1e12) {
        c = 'b';
        g /= 1e9;
    } else
        return -1;
    snprintf(s, MAX_STRLEN, "%.3f%c", g, c);
    return 0;
}

static error_t
parse_opt(int key, char *arg, struct argp_state *state) {
    financials *f = state->input;

    switch(key) {
        case 'n':
            strncpy(f->name, arg, MAX_STRLEN);
            break;
        case 'p':
            f->price = mstrtod(arg);
            break;
        case 'P':
            strncpy(f->mktcap_str, arg, MAX_STRLEN);
            f->mktcap = mstrtod(arg);
            break;
        case 'd':
            ISET(f->div_ps, mstrtod(arg));
            break;
        case 'D':
            ISET(f->div, mstrtod(arg));
            break;
        case 'q':
            f->period = (U16)strtoul(arg, (&arg+strlen(arg)), 10);
            break;
        case 'e':
            ISET(f->earn_ps, mstrtod(arg));
            break;
        case 'E':
            ISET(f->earn, mstrtod(arg));
            break;
        case 'b':
            ISET(f->ebitda, mstrtod(arg));
            break;
        case 'B':
            ISET(f->ebit, mstrtod(arg));
            break;
        case 'r':
            ISET(f->d_and_a, mstrtod(arg));
            break;
        case 'A':
            ISET(f->assets, mstrtod(arg));
            break;
        case 'a':
            ISET(f->curr_assets, mstrtod(arg));
            break;
        case 'i':
            ISET(f->intangibles, mstrtod(arg));
            break;
        case 'I':
            ISET(f->inventories, mstrtod(arg));
            break;
        case 'g':
            ISET(f->goodwill, mstrtod(arg));
            break;
        case 'L':
            ISET(f->liabilities, mstrtod(arg));
            break;
        case 'l':
            ISET(f->curr_liabilities, mstrtod(arg));
            break;
        case 't':
            ISET(f->stdebt, mstrtod(arg));
            break;
        case 'T':
            ISET(f->ltdebt, mstrtod(arg));
            break;
        case 'v':
            f->ev = mstrtod(arg);
            break;
        case 'c':
            ISET(f->cash, mstrtod(arg));
            break;
        case 'C':
            ISET(f->cfo, mstrtod(arg));
            break;
        case 'x':
            ISET(f->capex, mstrtod(arg));
            break;
        case 'X':
            ISET(f->tax, mstrtod(arg));
            break;
        case 'm':
            ISET(f->minority_int, mstrtod(arg));
            break;
        case ARGP_KEY_END:
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt};

void
check(int ok, char *message) {
    if (ok) return;
    printf("%s\n", message);
    exit(-1);
}

void
check_args(financials *f) {
    check(f->period >= 0 && f->period <= 4,
          "Reporting period must be between 1 and 4");
    // XXX TODO: check for all periods instead of just the first
    check(IV0(f->curr_liabilities) <= IV0(f->liabilities),
          "Current liabilities cannot exceed total liabilities");
    check(IV0(f->curr_assets) < IV0(f->assets),
          "Current assets cannot exceed total assets");
    check((IV0(f->stdebt) + IV0(f->ltdebt)) <= IV0(f->liabilities),
          "Long-term debt and short-term debt cannot exceed total liabilities");
}

double /* Sum of an array of doubles */
dsum(in_series *is) {
    double s = 0;
    for (int i = 0; i < is->e; ++i)
        s += is->v[i];
    return s;
}

double /* Average of an array of doubles */
davg(in_series *is) {
    return (is->e == 0) ? 0 : dsum(is)/is->e;
}

// TODO: In the set_* function we should use q to only take davg_yr
// for the right period
double /* Annualized average of series */
davg_yr(in_series *is, U16 period) {
    return (period == 0) ? 0 : (4.0/period)*davg(is);
}

void
set_ebitda(financials *f, metrics *m) {
    U16 q = 0;
    double ebitda = 0.0;
    if (IE(f->ebit) && IE(f->d_and_a)) {
        ebitda = davg_yr(&(f->ebit), f->period) + davg_yr(&(f->d_and_a), f->period);
        q = MINQTR2(f->period, f->ebit, f->d_and_a);
    }
    if (IE(f->ebitda)) {
        ebitda = davg_yr(&(f->ebitda), f->period);
        q = QTR(f, f->ebitda);
    }
    if (q == 0) return;
    MSET(m->ebitda, ebitda, q);
}

void
set_fcf(financials *f, metrics *m) {
    U16 q = 0;
    double fcf = 0.0;
    if (IE(f->ebitda) && IE(f->capex) && IE(f->tax)) {
        fcf = davg_yr(&(f->ebitda), f->period) - davg_yr(&(f->capex), f->period) -
              davg_yr(&(f->tax), f->period);
        q = MINQTR3(f->period, f->ebitda, f->capex, f->tax);
    }
    if (IE(f->ebit) && IE(f->d_and_a) && IE(f->capex) && IE(f->tax)) {
        fcf = davg_yr(&(f->ebit), f->period) + davg_yr(&(f->d_and_a), f->period) -
              davg_yr(&(f->capex), f->period) - davg_yr(&(f->tax), f->period);
        q = MINQTR4(f->period, f->ebit, f->d_and_a, f->capex, f->tax);
    }
    if (q == 0) return;
    MSET(m->fcf, fcf, q);
}

void
set_p_to_e(financials *f, metrics *m) {
    U16 q = 0;
    double pe = 0.0;
    if (IE(f->earn_ps) && !ZERO(f->price)) {
        pe = f->price/davg_yr(&(f->earn_ps), f->period);
        q = QTR(f, f->earn_ps);
    }
    if (IE(f->earn) && !ZERO(f->mktcap)) {
        pe = f->mktcap/davg_yr(&(f->earn), f->period);
        q = QTR(f, f->earn);
    }
    if (q == 0) return;
    MSET(m->p_to_e, pe, q);
}

void
set_div_yield(financials *f, metrics *m) {
    U16 q = 0;
    double dy = 0.0;
    if (!ZERO(f->mktcap) && IE(f->div)) {
        dy = davg_yr(&(f->div), f->period)/f->mktcap;
        q = QTR(f, f->div);
    }
    if (!ZERO(f->price) && IE(f->div_ps)) {
        dy = davg_yr(&(f->div_ps), f->period)/f->price;
        q = QTR(f, f->div_ps);
    }
    if (q == 0) return;
    MSET(m->div_yield, dy, q);
}

void
set_div_cover(financials *f, metrics *m) {
    U16 q = 0;
    double dc = 0.0;
    if (IE(f->earn) && IE(f->div)) {
        if (ZERO(dsum(&(f->div)))) {
            dc = 1.0;
        } else {
            dc = davg_yr(&(f->earn), f->period)/davg_yr(&(f->div), f->period);
        }
        q = MINQTR2(f->period, f->earn, f->div);
    }
    if (IE(f->earn_ps) && IE(f->div_ps)) {
        if (ZERO(dsum(&(f->div_ps)))) {
            dc = 1.0;
        } else {
            dc = davg_yr(&(f->earn_ps), f->period)/davg_yr(&(f->div_ps), f->period);
        }
        q = MINQTR2(f->period, f->earn_ps, f->div_ps);
    }
    if (q == 0) return;
    MSET(m->div_cover, dc, q);
}

double
book(financials *f, metrics *m) {
    if (IE(f->assets) && IE(f->intangibles) && IE(f->goodwill) &&
        IE(f->liabilities)) {
        return IV0(f->assets) - IV0(f->intangibles) - IV0(f->goodwill) -
               IV0(f->liabilities);
    }
    return UNS;
}

void
set_roc(financials *f, metrics *m) {
    if (IE(f->ebit) && IE(f->cash) && IE(f->stdebt) && IE(f->ltdebt) &&
        book(f, m) != UNS) {
        double debt = IV0(f->stdebt) + IV0(f->ltdebt);
        double equity = book(f, m);
        double roc = davg_yr(&(f->ebit), f->period)/(debt - equity - IV0(f->cash));
        U16 q = QTR(f, f->ebit);
        MSET(m->roc, roc, q);
    }
}

void
set_roe(financials *f, metrics *m) {
    if (IE(f->earn) && book(f, m) != UNS) {
        double roe = davg_yr(&(f->earn), f->period)/book(f, m);
        U16 q = QTR(f, f->earn);
        MSET(m->roe, roe, q);
    }
}

void
compute_metrics(financials *f, metrics *m) {
    /* EV */
    if (!ZERO(f->ev))
        m->ev = f->ev;
    else if (!ZERO(f->mktcap) && IE(f->stdebt) && IE(f->ltdebt) &&
             IE(f->minority_int) && IE(f->cash))
        m->ev = f->mktcap + IV(f->stdebt, 0) + IV(f->ltdebt, 0) +
                IV(f->minority_int, 0) - IV(f->cash, 0);

    /* earnings */
    if (IE(f->earn) && f->period)
        MSET(m->earnings, davg(&(f->earn)), QTR(f, f->earn));

    /* EBITDA */
    set_ebitda(f, m);

    /* EBITDA - capex */
    /* P/(EBITDA-capex) */
    set_fcf(f, m);
    if (!MBLANK(m->fcf) && !ZERO(f->mktcap))
        MSET(m->p_to_fcf, f->mktcap/MV(m->fcf), MQ(m->fcf));

    /* CFO */
    /* P/CFO */
    if (IE(f->cfo) && f->period)
        MSET(m->cfo, davg(&(f->cfo)), QTR(f, f->cfo));
    if (!MBLANK(m->cfo) && !ZERO(f->mktcap))
        MSET(m->p_to_cfo, f->mktcap/MV(m->cfo), MQ(m->cfo));

    /* EV/EBITDA */
    if (!MBLANK(m->ebitda) && m->ev)
        MSET(m->ev_to_ebitda, m->ev/MV(m->ebitda), QTR(f,f->ebitda));

    /* P/E */
    set_p_to_e(f, m);

    /* P/B */
    if (IE(f->assets) && IE(f->intangibles) && IE(f->goodwill) && IE(f->liabilities))
        MSET(m->book, IV0(f->assets) - IV0(f->goodwill) - IV0(f->intangibles) -
                      IV0(f->liabilities), 0);
    if (!ZERO(f->mktcap) && !MBLANK(m->book))
        MSET(m->p_to_b, f->mktcap/MV(m->book), 0);

    /* P/C */
    if (!ZERO(f->mktcap) && IE(f->cash))
        MSET(m->p_to_c, f->mktcap/IV0(f->cash), 0);

    /* Dividend yield */
    if (!ZERO(f->price) && IE(f->div_ps))
        MSET(m->div_yield, davg_yr(&(f->div_ps), f->period)/f->price, QTR(f, f->div_ps));
    if (!ZERO(f->mktcap) && IE(f->div))
        MSET(m->div_yield, davg_yr(&(f->div), f->period)/f->mktcap, QTR(f, f->div));

    /* Dividend coverage */
    set_div_cover(f, m);
    // XXX TODO: Make this payout coverage?

    /* Return on capital */
    set_roc(f, m);

    /* Return on equity */
    set_roe(f, m);

    /* Current ratio */
    if (IE(f->curr_assets) && IE(f->curr_liabilities))
        MSET(m->current, IV0(f->curr_assets)/IV0(f->curr_liabilities), 0);

    /* Quick ratio */
    if (IE(f->curr_assets) && IE(f->curr_liabilities) && IE(f->inventories))
        MSET(m->quick,
             (IV0(f->curr_assets) - IV0(f->inventories))/IV0(f->curr_liabilities),
             0);

    /* L/E */
    /* D/E */
    if (IE(f->liabilities) && !MBLANK(m->book))
        MSET(m->l_to_e, IV0(f->liabilities)/MV(m->book), 0);
    if (IE(f->stdebt) && IE(f->ltdebt) && !MBLANK(m->book))
        MSET(m->d_to_e, (IV0(f->stdebt) + IV0(f->ltdebt))/MV(m->book), 0);

    /* D/EBITDA */
    if (IE(f->stdebt) && IE(f->ltdebt) && !MBLANK(m->ebitda))
        MSET(m->l_to_ebitda, (IV0(f->stdebt)+IV0(f->ltdebt))/MV(m->ebitda), MQ(m->ebitda));

    /* STD/D */
    /* LTD/D */
    if (IE(f->stdebt) && IE(f->ltdebt)) {
        MSET(m->std_to_d, IV0(f->stdebt)/(IV0(f->stdebt) + IV0(f->ltdebt)), 0);
        MSET(m->ltd_to_d, IV0(f->ltdebt)/(IV0(f->stdebt) + IV0(f->ltdebt)), 0);
    }
}

void
print_str(char *name, char *str, U16 quarters) {
    if (!strlen(str)) return;
    if (quarters)
        printf("%-25s%-20s%uQ\n", name, str, quarters);
    else
        printf("%-25s%-20s\n", name, str);
}

void
print_money(char *name, metric m) {
    char buf[MAX_STRLEN+1];
    buf[0] = '$';
    strcpy(buf, "$");
    if (MBLANK(m) || dtomstr(MV(m), &(buf[1]))) return;
    print_str(name, buf, MQ(m));
}

void
print_ratio(char *name, metric m) {
    if (MBLANK(m)) return;
    if (!MQ(m))
        printf("%-25s%-20.3f\n", name, MV(m));
    else
        printf("%-25s%-20.3f%uQ\n", name, MV(m), MQ(m));
}

void
show_fundamentals(financials *f, metrics *m) {
    printf("\n");

    print_str("Company", f->name, 0);
    metric mktcap = {mstrtod(f->mktcap_str), 0, 1};
    print_money("Market cap", mktcap);
    metric ev = {m->ev, 0, 1};
    print_money("EV", ev);
    print_money("EBITDA", m->ebitda);
    print_money("Earnings", m->earnings);
    print_money("FCF", m->fcf);
    print_money("CFO", m->cfo);
    print_ratio("EV/EBITDA", m->ev_to_ebitda);
    print_ratio("P/E", m->p_to_e);
    print_ratio("P/FCF", m->p_to_fcf);
    print_ratio("P/CFO", m->p_to_cfo);
    print_ratio("P/B", m->p_to_b);
    print_ratio("P/C", m->p_to_c);
    print_ratio("Dividend yield", m->div_yield);
    print_ratio("Dividend coverage", m->div_cover);
    print_ratio("ROC", m->roc);
    print_ratio("ROE", m->roe);
    print_ratio("Current ratio", m->current);
    print_ratio("Quick ratio", m->quick);
    print_ratio("L/E", m->l_to_e);
    print_ratio("D/E", m->d_to_e);
    print_ratio("D/EBITDA", m->l_to_ebitda);
    print_ratio("STD/D", m->std_to_d);
    print_ratio("LTD/D", m->ltd_to_d);

    printf("\n");
}

void
init(financials *f, metrics *m) {
    memset(f, 0, sizeof(financials));
    memset(m, 0, sizeof(metrics));
}

int
main(int argc, char **argv) {
    financials f;
    metrics m;

    init(&f, &m);
    argp_parse(&argp, argc, argv, 0, 0, &f);
    check_args(&f);
    compute_metrics(&f, &m);
    show_fundamentals(&f, &m);
}

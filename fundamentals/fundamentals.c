#include <argp.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#define U16 unsigned short

#define MAX_STRLEN 30
#define MAX_PERIODS 12

#define UNS DBL_MIN /* Default double value */
#define DSET(d) (d != UNS)
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/* To add a new metric: (1) add a field to the metrics struct; (2) add
 * the computation to compute_metrics; (3) initialize it in main();
 * (4) add the metric to the show routine.
 *
 * To add a new input: (1) add a flag to the argp_option array; (2) add
 * a field to the financials struct; (3) parse the option in parse_opt;
 * (4) initialize the field in main(). */

static struct argp_option options[] = {
    {"name", 'n', "NAME", 0, "Company name"},
    {"mktcap", 'P', "NUM", 0,  "Market cap"},
    {"price",  'p', "NUM", 0,  "Share price"},
    {"div", 'D', "NUM", 0, "Dividend"},
    {"div-ps", 'd', "NUM", 0, "Dividend per share"},
    {"reporting-period", 'q', "NUM 1-4", 0, "Reporting period"},
    {"earn", 'e', "NUM", 0, "One period of earnings"},
    {"earn-ps", 'E', "NUM", 0, "One period of per-share earnings"},
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
    {"minority-interest", 'm', "NUM", 0, "Minority interest"},
    {"op_profit", 'o', "NUM", 0, "Operating profit"},
    {"depreciation", 'r', "NUM", 0, "Depreciation"},
    {"amortization", 'R', "NUM", 0, "Amortization"},
    {0}
};

struct financials {
    char name[MAX_STRLEN+1];
    char mktcap_str[MAX_STRLEN+1];
    double price;
    double mktcap;
    U16 period;
    double div_ps[MAX_PERIODS];
    U16 div_ps_entries;
    double div[MAX_PERIODS];
    U16 div_entries;
    double earn_ps[MAX_PERIODS];
    U16 earn_ps_entries;
    double earn[MAX_PERIODS];
    U16 earn_entries;
    double op_profit[MAX_PERIODS];
    U16 op_profit_entries;
    double depreciation[MAX_PERIODS];
    U16 depreciation_entries;
    double amortization[MAX_PERIODS];
    U16 amortization_entries;
    double assets;
    double intangibles;
    double goodwill;
    double liabilities;
    double stdebt;
    double ltdebt;
    double curr_assets;
    double curr_liabilities;
    double inventories;
    double cash;
    double minority_int;
};

struct metrics {
    double ev;
    double ebitda;
    double ev_to_ebitda;
    double p_to_e;
    double p_to_b;
    double div_yield;
    double div_cover;
    double current;
    double quick;
    double d_to_e;
    double std_to_e;
    double ltd_to_e;
    double std_to_d;
    double ltd_to_d;
};

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
    snprintf(s, MAX_STRLEN, "%.2f%c", g, c);
    return 0;
}

static error_t
parse_opt(int key, char *arg, struct argp_state *state) {
    struct financials *f = state->input;

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
            if (f->div_ps_entries < MAX_PERIODS)
                f->div_ps[f->div_ps_entries++] = mstrtod(arg);
            break;
        case 'D':
            if (f->div_entries < MAX_PERIODS)
                f->div[f->div_entries++] = mstrtod(arg);
            break;
        case 'q':
            f->period = (U16)strtoul(arg, (&arg+strlen(arg)), 10);
            break;
        case 'e':
            if (f->earn_ps_entries < MAX_PERIODS)
                f->earn_ps[f->earn_ps_entries++] = mstrtod(arg);
            break;
        case 'E':
            if (f->earn_entries < MAX_PERIODS)
                f->earn[f->earn_entries++] = mstrtod(arg);
            break;
        case 'o':
            if (f->op_profit_entries < MAX_PERIODS)
                f->op_profit[f->op_profit_entries++] = mstrtod(arg);
            break;
        case 'r':
            if (f->depreciation_entries < MAX_PERIODS)
                f->depreciation[f->depreciation_entries++] = mstrtod(arg);
            break;
        case 'R':
            if (f->amortization_entries < MAX_PERIODS)
                f->amortization[f->amortization_entries++] = mstrtod(arg);
            break;
        case 'A':
            f->assets = mstrtod(arg);
            break;
        case 'a':
            f->curr_assets = mstrtod(arg);
            break;
        case 'i':
            f->intangibles = mstrtod(arg);
            break;
        case 'I':
            f->inventories = mstrtod(arg);
            break;
        case 'g':
            f->goodwill = mstrtod(arg);
            break;
        case 'L':
            f->liabilities = mstrtod(arg);
            break;
        case 'l':
            f->curr_liabilities = mstrtod(arg);
            break;
        case 't':
            f->stdebt = mstrtod(arg);
            break;
        case 'T':
            f->ltdebt = mstrtod(arg);
            break;
        case 'c':
            f->cash = mstrtod(arg);
            break;
        case 'm':
            f->minority_int = mstrtod(arg);
            break;
        case ARGP_KEY_END:
            /* XXX: Could add argument checking here */
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
check_args(struct financials *f) {
    check(f->period >= 0 && f->period <= 4,
          "Reporting period must be between 1 and 4");
    check(f->curr_liabilities < f->liabilities,
          "Current liabilities cannot exceed total liabilities");
    check(f->curr_assets < f->assets,
          "Current assets cannot exceed total assets");
    check((f->stdebt + f->ltdebt) <= f->liabilities,
          "Long-term debt and short-term debt cannot exceed total liabilities");
}

/* Sum of an array of doubles */
double
dsum(double a[], U16 n) {
    double s = 0;
    for (int i = 0; i < n; ++i)
        s += a[i];
    return s;
}

/* Average of an array of doubles */
double
davg(double a[], U16 n) {
    return dsum(a, n)/n;
}

/* Annual average--if an array represents a period of more
 * than one year, get the annualized average */
double
davg_yr(double a[], U16 n, U16 period) {
    return (4.0/period)*davg(a, n);
}

void
compute_metrics(struct financials *f, struct metrics *m) {
    /* EV */
    if (DSET(f->mktcap) && DSET(f->stdebt) && DSET(f->ltdebt) &&
        DSET(f->minority_int) && DSET(f->cash))
        m->ev = f->mktcap + f->stdebt + f->ltdebt + f->minority_int - f->cash;

    /* EBITDA */
    if (f->op_profit_entries && f->depreciation_entries &&
        f->amortization_entries && f->period)
        m->ebitda = davg_yr(f->op_profit, f->op_profit_entries, f->period) +
                    davg_yr(f->depreciation, f->depreciation_entries, f->period) +
                    davg_yr(f->amortization, f->amortization_entries, f->period);

    /* EV/EBITDA */
    if (DSET(m->ev) && DSET(m->ebitda))
        m->ev_to_ebitda = m->ev/m->ebitda;

    /* P/E */
    if (DSET(f->mktcap) && f->earn_entries && f->period)
        m->p_to_e = f->mktcap/davg_yr(f->earn, f->earn_entries, f->period);
    if (DSET(f->price) && f->earn_ps_entries)
        m->p_to_e = f->price/davg_yr(f->earn_ps, f->earn_ps_entries, f->period);

    /* P/B */
    if (DSET(f->mktcap) && DSET(f->assets) && DSET(f->intangibles) &&
        DSET(f->goodwill) && DSET(f->liabilities)) {
        double b = f->assets - f->intangibles - f->goodwill - f->liabilities;
        m->p_to_b = f->mktcap/b;
    }

    /* Dividend yield */
    if (DSET(f->mktcap) && f->div_entries && f->period)
        m->div_yield = davg_yr(f->div, f->div_entries, f->period)/f->mktcap;
    if (DSET(f->price) && f->div_ps_entries && f->period)
        m->div_yield = davg_yr(f->div_ps, f->div_ps_entries, f->period)/f->price;

    /* Dividend coverage */
    if (f->earn_entries && f->div_entries) {
        m->div_cover = 1;
        int n = MIN(f->earn_entries, f->div_entries);
        if (dsum(f->div, n) > 0)
            m->div_cover = dsum(f->earn, n)/dsum(f->div, n);
    }
    if (f->earn_ps_entries && f->div_ps_entries) {
        m->div_cover = 1;
        int n = MIN(f->earn_ps_entries, f->div_ps_entries);
        if (dsum(f->div_ps, n) > 0)
            m->div_cover = dsum(f->earn_ps, n)/dsum(f->div_ps, n);
    }
    // XXX TODO: Make this payout coverage?

    /* Current ratio */
    if (DSET(f->curr_assets) && DSET(f->curr_liabilities))
        m->current = f->curr_assets/f->curr_liabilities;

    /* Quick ratio */
    if (DSET(f->curr_assets) && DSET(f->curr_liabilities) &&
        DSET(f->inventories))
        m->quick = (f->curr_assets - f->inventories)/f->curr_liabilities;

    /* D/E */
    if (DSET(f->liabilities) && DSET(f->assets) &&
        DSET(f->intangibles) && DSET(f->goodwill)) {
        double eq = f->assets - f->intangibles - f->goodwill - f->liabilities;
        m->d_to_e = f->liabilities/eq;
    }

    /* STD/E */
    if (DSET(f->stdebt) && DSET(f->assets) &&
        DSET(f->intangibles) && DSET(f->goodwill))
        m->std_to_e = f->stdebt/(f->assets - f->intangibles - f->goodwill);

    /* LTD/E */
    if (DSET(f->ltdebt) && DSET(f->assets) &&
        DSET(f->intangibles) && DSET(f->goodwill))
        m->ltd_to_e = f->ltdebt/(f->assets - f->intangibles - f->goodwill);

    /* STD/D */
    /* LTD/D */
    if (DSET(f->stdebt) && DSET(f->ltdebt)) {
        m->std_to_d = f->stdebt/(f->stdebt + f->ltdebt);
        m->ltd_to_d = f->ltdebt/(f->stdebt + f->ltdebt);
    }
}

void
show_fundamentals(struct financials *f, struct metrics *m) {
    char buf[MAX_STRLEN+1];
    printf("\n");

    if (strlen(f->name))
        printf("%-25s%-20s\n", "Company", f->name);
    if (strlen(f->mktcap_str))
        printf("%-25s$%-20s\n", "Market cap", f->mktcap_str);
    if (DSET(m->ev) && !dtomstr(m->ev, buf))
        printf("%-25s$%-20s\n", "EV", &buf[0]);
    if (DSET(m->ebitda) && !dtomstr(m->ebitda, buf))
        printf("%-25s$%-20s\n", "EBITDA", &buf[0]);
    if (DSET(m->ev) && DSET(m->ebitda))
        printf("%-25s%-20.3f\n", "EV/EBITDA", m->ev_to_ebitda);
    if (DSET(m->p_to_e))
        printf("%-25s%-20.3f\n", "P/E", m->p_to_e);
    if (DSET(m->p_to_b))
        printf("%-25s%-20.3f\n", "P/B", m->p_to_b);
    if (DSET(m->div_yield))
        printf("%-25s%-20.3f\n", "Dividend yield", m->div_yield);
    if (DSET(m->div_cover))
        printf("%-25s%-20.3f\n", "Dividend coverage", m->div_cover);
    if (DSET(m->current))
        printf("%-25s%-20.3f\n", "Current ratio", m->current);
    if (DSET(m->quick))
        printf("%-25s%-20.3f\n", "Quick ratio", m->quick);
    if (DSET(m->d_to_e))
        printf("%-25s%-20.3f\n", "D/E", m->d_to_e);
    if (DSET(m->std_to_e))
        printf("%-25s%-20.3f\n", "STD/E", m->std_to_e);
    if (DSET(m->ltd_to_e))
        printf("%-25s%-20.3f\n", "LTD/E", m->ltd_to_e);
    if (DSET(m->std_to_d))
        printf("%-25s%-20.3f\n", "STD/D", m->std_to_d);
    if (DSET(m->ltd_to_d))
        printf("%-25s%-20.3f\n", "LTD/D", m->ltd_to_d);

    printf("\n");
}

int
main(int argc, char **argv) {
    struct financials financials = {"", "", UNS, UNS, 0, {}, 0, {}, 0, {}, 0,
                                    {}, 0, {}, 0, {}, 0, {}, 0, UNS, UNS, UNS,
                                    UNS, UNS, UNS, UNS, UNS, UNS, UNS, UNS};
    struct metrics metrics = {UNS, UNS, UNS, UNS, UNS, UNS, UNS,
                              UNS, UNS, UNS, UNS, UNS, UNS, UNS};
    argp_parse(&argp, argc, argv, 0, 0, &financials);
    check_args(&financials);
    compute_metrics(&financials, &metrics);
    show_fundamentals(&financials, &metrics);
}

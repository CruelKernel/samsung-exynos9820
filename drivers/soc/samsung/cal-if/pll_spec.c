#include "cmucal.h"
#include "ra.h"

static struct pll_spec gpll1416X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1600*MHZ,	3200*MHZ,
	200*MHZ,	3200*MHZ,
	150,		0,
};

static struct pll_spec gpll1417X_spec = {
	1,		63,
	64,		1023,
	0,		5,
	0,		0,
	4*MHZ,		12*MHZ,
	1000*MHZ,	2000*MHZ,
	200*MHZ,	2000*MHZ,
	150,		0,
};

static struct pll_spec gpll1418X_spec = {
	1,		63,
	16,		511,
	0,		5,
	0,		0,
	2*MHZ,		8*MHZ,
	600*MHZ,	1200*MHZ,
	100*MHZ,	1200*MHZ,
	150,		0,
};

static struct pll_spec gpll1419X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1900*MHZ,	3800*MHZ,
	200*MHZ,	3800*MHZ,
	150,		0,
};

static struct pll_spec gpll1431X_spec = {
	1,		63,
	16,		511,
	0,		5,
	-32767,		32767,
	6*MHZ,		30*MHZ,
	400*MHZ,	800*MHZ,
	50*MHZ,		800*MHZ,
	150,		3000,
};

static struct pll_spec gpll1450X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1600*MHZ,	3304*MHZ,
	25*MHZ,		3304*MHZ,
	150,		0,
};

static struct pll_spec gpll1451X_spec = {
	1,		63,
	64,		1023,
	0,		5,
	0,		0,
	4*MHZ,		12*MHZ,
	900*MHZ,	1800*MHZ,
	30*MHZ,		1800*MHZ,
	150,		0,
};

static struct pll_spec gpll1452X_spec = {
	1,		63,
	16,		511,
	0,		5,
	0,		0,
	4*MHZ,		8*MHZ,
	500*MHZ,	1000*MHZ,
	16*MHZ,		1000*MHZ,
	150,		0,
};

static struct pll_spec gpll1460X_spec = {
	1,		63,
	16,		511,
	0,		5,
	-32767,		32767,
	6*MHZ,		30*MHZ,
	400*MHZ,	800*MHZ,
	12.5*MHZ,	800*MHZ,
	150,		3000,
};

static struct pll_spec gpll1050X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	2250*MHZ,	4500*MHZ,
	35.2*MHZ,	4500*MHZ,
	150,		0,
};

static struct pll_spec gpll1051X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1200*MHZ,	2400*MHZ,
	18.8*MHZ,	2400*MHZ,
	150,		0,
};

static struct pll_spec gpll1052X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	2*MHZ,		8*MHZ,
	600*MHZ,	1200*MHZ,
	9.5*MHZ,	1200*MHZ,
	150,		0,
};

static struct pll_spec gpll1061X_spec = {
	1,		63,
	16,		511,
	0,		6,
	-32767,		32767,
	6*MHZ,		30*MHZ,
	600*MHZ,	1200*MHZ,
	9.5*MHZ,	1200*MHZ,
	150,		500,
};

static struct pll_spec gpll1016X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4.5*MHZ,	12*MHZ,
	2250*MHZ,	4500*MHZ,
	35.2*MHZ,	4500*MHZ,
	150,		0,
};

static struct pll_spec gpll1017X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	950*MHZ,	2400*MHZ,
	18.8*MHZ,	2400*MHZ,
	150,		0,
};

static struct pll_spec gpll1018X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	2*MHZ,		8*MHZ,
	600*MHZ,	1200*MHZ,
	9.5*MHZ,	1200*MHZ,
	150,		0,
};

static struct pll_spec gpll1019X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1500*MHZ,	3000*MHZ,
	22.3*MHZ,	3000*MHZ,
	150,		0,
};

static struct pll_spec gpll1031X_spec = {
	1,		63,
	16,		511,
	0,		6,
	-32767,		32767,
	6*MHZ,		30*MHZ,
	600*MHZ,	1200*MHZ,
	9.5*MHZ,	1200*MHZ,
	150,		500,
};

static struct pll_spec gpll0831X_spec = {
	1,		63,
	16,		511,
	0,		6,
	-32767,		32767,
	6*MHZ,		30*MHZ,
	600*MHZ,	1200*MHZ,
	9.5*MHZ,	1200*MHZ,
	500,		500,
};

static struct pll_spec gdpll0817X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	950*MHZ,	2400*MHZ,
	14.8*MHZ,	2400*MHZ,
	300,		0,
};

static struct pll_spec gpll0820X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1500*MHZ,	3000*MHZ,
	40.63*MHZ,	6600*MHZ,
	150,		0,
};

static struct pll_spec gpll0821X_spec = {
	1,		63,
	64,		1023,
	0,		6,
	0,		0,
	4*MHZ,		12*MHZ,
	1500*MHZ,	3000*MHZ,
	35*MHZ,		5400*MHZ,
	150,		0,
};

struct pll_spec *pll_get_spec(struct cmucal_pll *pll)
{
	struct pll_spec *pll_spec;

	switch (pll->type) {
	case PLL_1416X:
		pll_spec = &gpll1416X_spec;
		break;
	case PLL_1417X:
		pll_spec = &gpll1417X_spec;
		break;
	case PLL_1418X:
		pll_spec = &gpll1418X_spec;
		break;
	case PLL_1419X:
		pll_spec = &gpll1419X_spec;
		break;
	case PLL_1431X:
		pll_spec = &gpll1431X_spec;
		break;
	case PLL_1450X:
		pll_spec = &gpll1450X_spec;
		break;
	case PLL_1451X:
		pll_spec = &gpll1451X_spec;
		break;
	case PLL_1452X:
		pll_spec = &gpll1452X_spec;
		break;
	case PLL_1460X:
		pll_spec = &gpll1460X_spec;
		break;
	case PLL_1050X:
		pll_spec = &gpll1050X_spec;
		break;
	case PLL_1051X:
		pll_spec = &gpll1051X_spec;
		break;
	case PLL_1052X:
		pll_spec = &gpll1052X_spec;
		break;
	case PLL_1061X:
		pll_spec = &gpll1061X_spec;
		break;
	case PLL_1016X:
		pll_spec = &gpll1016X_spec;
		break;
	case PLL_0817X:
	case PLL_0822X:
	case PLL_1017X:
		pll_spec = &gpll1017X_spec;
		break;
	case PLL_0818X:
	case PLL_1018X:
		pll_spec = &gpll1018X_spec;
		break;
	case PLL_1019X:
		pll_spec = &gpll1019X_spec;
		break;
	case PLL_1031X:
		pll_spec = &gpll1031X_spec;
		break;
	case PLL_0831X:
		pll_spec = &gpll0831X_spec;
		break;
	case DPL_L0817X:
		pll_spec = &gdpll0817X_spec;
		break;
	case PLL_0820X:
		pll_spec = &gpll0820X_spec;
		break;
	case PLL_0821X:
		pll_spec = &gpll0821X_spec;
		break;
	default:
		pll_spec = NULL;
	}

	return pll_spec;
}

static unsigned int __pll_find_m(unsigned int p, unsigned int s,
				 unsigned long long fin,
				 unsigned long long rate)
{
	rate *= p << s;
	do_div(rate, fin);

	return rate;
}

static int __pll_find_k(unsigned int p, unsigned int m, unsigned int s,
				 unsigned long long fin,
				 unsigned long long rate)
{
	rate <<= 16;
	rate *= p << s;
	do_div(rate, fin);

	return rate - (m << 16);
}

int pll_get_locktime(struct cmucal_pll *pll)
{
	struct pll_spec *pll_spec;

	pll_spec = pll_get_spec(pll);
	if (!pll_spec) {
		pr_err("un-support pll type\n");
		return -EVCLKINVAL;
	}

	pll->lock_time = pll_spec->lock_time;
	pll->flock_time = pll_spec->flock_time;

	return 0;
}
EXPORT_SYMBOL_GPL(pll_get_locktime);

int pll_find_table(struct cmucal_pll *pll,
		   struct cmucal_pll_table *table,
		   unsigned long long fin,
		   unsigned long long rate,
		   unsigned long long rate_hz)
{
	struct pll_spec *pll_spec;
	unsigned int p, m, s;
	int k;
	unsigned long long fref, fvco, fout;
	unsigned long long min_diff = 0, tmp;
	unsigned int min_p, min_m, min_s, min_fout;

	pll_spec = pll_get_spec(pll);
	if (!pll_spec) {
		pr_err("un-support pll type\n");
		return -EVCLKINVAL;
	}

	/* khz_to_hz() : for calculate precisely  */
	rate = rate_hz ? rate_hz : khz_to_hz(rate);

	for (p = pll_spec->pdiv_min; p <= pll_spec->pdiv_max; p++) {
		/* check fref  */
		fref = fin / p;
		if ((fref < pll_spec->fref_min) || (fref > pll_spec->fref_max))
			continue;
		for (s = pll_spec->sdiv_min; s <= pll_spec->sdiv_max; s++) {
			/* check m value  */
			m = __pll_find_m(p, s, fin, rate);
			if ((m < pll_spec->mdiv_min) ||
			    (m > pll_spec->mdiv_max))
				continue;

			if (is_normal_pll(pll)) {
				k = 0;
				fvco = fin * m;
			} else {
				/* check k value  */
				k = __pll_find_k(p, m, s, fin, rate);
				if ((k < pll_spec->kdiv_min) ||
				    (k > pll_spec->kdiv_max))
					continue;

				fvco = fin * ((m << 16) + k);
				fvco >>= 16;
			}

			/* check fvco */
			do_div(fvco, p);
			if ((fvco < pll_spec->fvco_min) ||
			    (fvco > pll_spec->fvco_max))
				continue;

			/* check fout */
			fout = fvco >> s;
			if ((fout >= pll_spec->fout_min) &&
			    (fout <= pll_spec->fout_max)) {
				/* hz_to_khz() : to change resolution */
				if (hz_to_khz(rate) == hz_to_khz(fout)) {
					table->rate = fout;
					table->pdiv = p;
					table->mdiv = m;
					table->sdiv = s;
					table->kdiv = k;

					return 0;
				}

				if (is_normal_pll(pll)) {
					tmp = abs(rate - fout);
					if (min_diff == 0 || tmp < min_diff) {
						min_diff = tmp;
						min_fout = fout;
						min_p = p;
						min_m = m;
						min_s = s;
					}
				}
			}
		}
	}

	if (min_diff) {
		table->rate = min_fout;
		table->pdiv = min_p;
		table->mdiv = min_m;
		table->sdiv = min_s;
		table->kdiv = 0;

		return 0;
	}

	return -EVCLKINVAL;
}
EXPORT_SYMBOL_GPL(pll_find_table);

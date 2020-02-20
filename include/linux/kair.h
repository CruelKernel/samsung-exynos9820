/**
 * Classfication and realization of KAIR concept using ECAVE - Elastic Capability
 * Vessel - dynamic resource control abstraction theory
 **/
#include <linux/sched.h>
#include <linux/bug.h>
#include <linux/list.h>
#include <linux/tfifo.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

/**
 *
 **/
#define KAIR_QUANT_STEP			(17)
#define KAIR_QUANT_SHIFT		(4)
#define KAIR_VARIANCE_SPAN		(64)
#define KAIR_VARIANCE_SPAN_SHIFT	(6)
#define KAIR_UNDETERMINED		(0xFFFFFFFF)
#define KAIR_DIVERGING			(-1)
#define KAIR_CONVERGING			(0)
#define KAIR_TPDF_CUMSUM		(512)
#define KAIR_TPDF_MINVAL		(1)
#define KAIR_TPDF_CASCADE_LEVEL		(1)
#define KAIR_ALIAS_LEN			(8)

#define KAIR_INF_TPDF_WEIGHT		(1)
#define KAIR_FIN_TPDF_WEIGHT		(2)
#if KAIR_TPDF_CASCADE_LEVEL > 1
#define KAIR_INST_TPDF_WEIGHT		(4)
#else
#define KAIR_INST_TPDF_WEIGHT		(8)
#endif

/**
 * Calculating Energy-Performance(EP) weight ratio, which is described
 * in theory as beta / alpha * upscaler.
 * where, beta is energy consuming panelty weight, alpha is bottleneck
 * panelty weight, and upscaler is 1024.
 **/
#define KAIR_BALANCED_EP_RATIO		(1024)

/**
 *
 **/
#define KAIR_WINDOW_INF			(0x8000000000000000UL)
#define KAIR_WINDOW_INF_MASK		(0x8000000000000000UL)
#define KAIR_WINDOW_NEED_RESCALE	(0x4000000000000000UL)
#define KAIR_WINDOW_SIZE_MASK		(0x0FFFFFFF00000000UL)
#define KAIR_WINDOW_SIZE_SHIFT		(32)
#define KAIR_WINDOW_CNT_MASK		(0x000000000FFFFFFFUL)

/**
 *
 **/
#define KAIR_DEF_RAND_NEUTRAL		(2)
#define KAIR_TRIGGER_THROTTLE		(3)
#define KAIR_FASTEN_THROTTLE		(-100)

/**
 * 1-dimensional discrete indexing degree of randomness which correspondingly
 * spans upto 64 skewness * 64 flatness long.
 * randomness bitfield : | <-- skewness --> | <-- flatness(variance) --> |
 **/
#define RAND_SKEW_SHIFT			(8)

typedef int randomness;

/**
 * Random integer variable for stochastic process
 **/
struct rand_var {
	int  		nval;
	unsigned int 	ubound;
	unsigned int  	lbound;
};

/**
 * Random integer variable coordinators
 **/
#define __RV_INITIALIZER()						\
{									\
	.nval		= 0, 						\
	.ubound		= 0,						\
	.lbound		= 0						\
}

#define RV_DECLARE(name)	struct rand_var name = __RV_INITIALIZER()

/**
 * @rv     : random variable structure
 * @num    : signed number
 * @ulimit : positive upper bound
 * @llimit : positive lower bound
 **/
#define RV_SET(rv, num, ulimit, llimit)					\
({									\
	rv.nval		= (int)(num);					\
	rv.ubound	= (unsigned int)(ulimit);			\
	rv.lbound	= (unsigned int)(llimit);			\
})

/* Stretching nval to fit given tpdf->qlvl and returning the proper index
 * within >= 0 and < tpdf->qlvl.
 */
#define RV_TAILORED(pdf, rv)						\
({									\
	typeof((pdf) + 1) __pdf	= (pdf);				\
	typeof((rv) + 1) __rv = (rv);					\
	int __nval;							\
	int __hscale = (int)__pdf->qlvl >> 1;				\
	int __ubound = (int)__rv->ubound;				\
	int __lbound = (int)__rv->lbound;				\
	__nval = (__rv->nval >= 0) ?					\
			(__ubound ?					\
				((__rv->nval * __hscale / __ubound) + __hscale) :	\
				 __hscale) :				\
			(__lbound ?					\
				((__rv->nval * __hscale / __lbound) + __hscale) :	\
				 __hscale);				\
	__nval;								\
})

#define RV_SPAN(rv)							\
({									\
	typeof((rv) + 1) __rv = (rv);					\
	unsigned int __span = __rv->ubound + __rv->lbound;		\
	__span;								\
})

/**
 * TPDF(Temporal Probability Density Function) container class
 **/
struct tpdf {
	struct list_head	pos;
	unsigned int		qlvl;
	u64			win_size;
	struct __tfifo		cache;
	unsigned int 		*qtbl;
	unsigned int		pc_cnt;

	/**
	 * """ TPDF container methods list """
	 * @tabling       : preparing qtbl
	 * @untabling     : removing qtbl
	 * @rv_register   : registration of corresponding rv
	 * @rv_unregister : moving rv
	 * @interpolator  : calculating moderate value
	 * @equilibrator  : rebalancing qtbl to maintain total PDF integral
	 * @rescaler      : rescaling the whole PDF to have 
	 **/
	int (*tabling)(struct tpdf *self);
	void (*untabling)(struct tpdf *self);
	void (*rv_register)(struct tpdf *self, struct rand_var *rv);
	void (*rv_unregister)(struct tpdf *self);
	unsigned int (*interpolator)(struct tpdf *self, unsigned int tpos,
				unsigned int rpos);
	void (*equilibrator)(struct tpdf *self);
	void (*rescaler)(struct tpdf *self);

	randomness		irand;
	char			alias[KAIR_ALIAS_LEN];
	struct rand_var		*rv;
	unsigned int		weight;
};

/**
 * """ KAIR statistics """
 * @choke_cnt    : counting how much the event so-called bottleneck or
 *		   momentary sluggish in ECAVE theory happened.
 * @save_total   : accumulating amount how much redundant capacity is saved.
 * @rand_neutral : an argument used for running betting algorithm.
 * @throttling   : +- accumulation of continuous choke/relief events.
 *		   0 is neutral.
 **/
struct kair_stats {
	unsigned int	choke_cnt;
	long long	save_total;
	unsigned int	rand_neutral;
	int		throttling;
};

/**
 * KAIR instance kobject created per each kair_obj_creator() call by
 * any client and will be used to register to sysfs.
 **/
struct krinst_obj {
	struct kobject		obj;
	struct kair_class	*inst;
};
#define to_krinst_obj(x)	container_of(x, struct krinst_obj, obj)

/**
 * KAIR attribute customized for krinst_obj.
 **/
struct krinst_attribute {
	struct attribute attr;
	ssize_t (*show)(struct krinst_obj *inst, struct krinst_attribute *attr, char *buf);
	ssize_t (*store)(struct krinst_obj *inst, struct krinst_attribute *attr, const char *buf, size_t count);
};
#define to_krinst_attr(x)	container_of(x, struct krinst_attribute, attr)

#define KRINST_ATTR_RO(__name) \
		static struct krinst_attribute __name##_attr = __ATTR_RO(__name)
#define KRINST_ATTR_RW(__name) \
		static struct krinst_attribute __name##_attr = __ATTR_RW(__name)

/**
 * TODO: more studies may be required to apply an external training.
 * Kairistics is experimentally determined, which describes the pair of integer
 * ratios in consideration of the fixed scaling factor heuristically used in each
 * client's legacy mechanism.
 **/
struct kairistics {
	unsigned int	gamma_numer:8;
	unsigned int	gamma_denom:8;
	unsigned int	theta_numer:8;
	unsigned int	theta_denom:8;
};

#define DECLARE_KAIRISTICS(alias, gnumer, gdenum, tnumer, tdenum)	\
	static struct kairistics kairistic_##alias = {			\
		.gamma_numer 	= gnumer,				\
		.gamma_denom 	= gdenum,				\
		.theta_numer 	= tnumer,				\
		.theta_denom 	= tdenum,				\
	}

/**
 * "" KAIR class description """
 * Which is virtually integrating Elastic Capacity Vessel abstraction model
 * applicable on arbitrary dynamic self resource control systems.
 * All real numbered properties must be handled with integers through adequate
 * conversion and quantization processes.
 **/
struct kair_class {
	struct list_head	tpdf_cascade;
	/**
 	 * bit-shift representation of KAIR capacity denominator
 	 * which is considered as the summation of each level's TPDF weight
 	 * multiplied by maximum randomness.
 	 **/
	unsigned int		capa_denom;

	/**
	 * """ KAIR methods list """:
	 * @initializer : self-initializer
	 * @stopper	: stopping to learn tpdf, cleaning up.
	 * @finalizer	: returning all resources.
	 * @job_learner : learner of capacity-probability-density which is
	 *		  actually conducting exclusive on-device learning
	 *		  algorithm on the given capacity-random variable.
	 * @job_inferer : returns randomness index of the TPDF which is most
	 *		  resembling given TPDF.
	 * @cap_bettor  : returns betting capacity estimated.
	 **/
	int (*initializer)(struct kair_class *self);
	void (*stopper)(struct kair_class *self);
	void (*finalizer)(struct kair_class *self);
	void (*job_learner)(struct kair_class *self, struct rand_var *v);
	int (*job_inferer)(struct kair_class *self);
	unsigned int (*cap_bettor)(struct kair_class *self, struct rand_var *v,
				unsigned int cap_legecy);

	struct kair_stats	stats;
	unsigned int		df_velocity;
	unsigned int		resilience;
	unsigned int		ep_ratio;
	unsigned int		max_capa;
	unsigned int		min_capa;
	unsigned int		epsilon;
	struct kairistics	kairistic;
	char			alias[KAIR_ALIAS_LEN];
	struct krinst_obj	*extif;
};

/**
 *
 *
 **/
extern int kair_tpdf_tabling(struct tpdf *self);
extern void kair_tpdf_untabling(struct tpdf *self);
extern void kair_tpdf_rv_register(struct tpdf *self, struct rand_var *rv);
extern void kair_tpdf_rv_unregister(struct tpdf *self);
extern unsigned int linear_interpolator(struct tpdf *self, unsigned int top_pos,
				 unsigned int raw_pos);
extern void kair_tpdf_equilibrator(struct tpdf *self);
extern void kair_tpdf_rescaler(struct tpdf *self);

#define tpdf_infinite_init(pdf, __tlevel)				\
({									\
	int __err = 0;							\
	typeof((pdf) + 1) __pdf = (pdf);				\
	sprintf(__pdf->alias, "%s_tpdf", #__tlevel);			\
	*__pdf = (struct tpdf) {					\
		.pos = LIST_HEAD_INIT(__pdf->pos),			\
		.qlvl = KAIR_QUANT_STEP,				\
		.win_size = KAIR_WINDOW_INF,				\
		.pc_cnt = 0,						\
		.irand = KAIR_DIVERGING,				\
		.weight = KAIR_INF_TPDF_WEIGHT,				\
		.cache = tfifo_init(&__pdf->cache),			\
		.tabling = kair_tpdf_tabling,				\
		.untabling = kair_tpdf_untabling,			\
		.rv_register = kair_tpdf_rv_register,			\
		.rv_unregister = kair_tpdf_rv_unregister,		\
		.interpolator = linear_interpolator,			\
		.equilibrator = kair_tpdf_equilibrator,			\
		.rescaler = kair_tpdf_rescaler,				\
	};								\
	__err = __pdf->tabling(__pdf);					\
	__err;								\
})

#define tpdf_finite_init(pdf, __tlevel, period)				\
({									\
	int __err = 0;							\
	typeof((pdf) + 1) __pdf = (pdf);				\
	sprintf(__pdf->alias, "%s_tpdf", #__tlevel);			\
	*__pdf = (struct tpdf) {					\
		.pos = LIST_HEAD_INIT(__pdf->pos),			\
		.qlvl = KAIR_QUANT_STEP,				\
		.win_size = ((u64)period << KAIR_WINDOW_SIZE_SHIFT) |	\
				KAIR_WINDOW_NEED_RESCALE,		\
		.pc_cnt = 0,						\
		.irand = KAIR_DIVERGING,				\
		.weight = KAIR_FIN_TPDF_WEIGHT,				\
		.cache = tfifo_init(&__pdf->cache),			\
		.tabling = kair_tpdf_tabling,				\
		.untabling = kair_tpdf_untabling,			\
		.rv_register = kair_tpdf_rv_register,			\
		.rv_unregister = kair_tpdf_rv_unregister,		\
		.interpolator = linear_interpolator,			\
		.equilibrator = kair_tpdf_equilibrator,			\
		.rescaler = kair_tpdf_rescaler,				\
	};								\
	__err = __pdf->tabling(__pdf);					\
	__err;								\
})

#define tpdf_instant_init(pdf, __tlevel, period)			\
({									\
	int __err = 0;							\
	typeof((pdf) + 1) __pdf	= (pdf);				\
	sprintf(__pdf->alias, "%s_tpdf", #__tlevel);			\
	*__pdf = (struct tpdf) {					\
		.pos = LIST_HEAD_INIT(__pdf->pos),			\
		.qlvl = KAIR_QUANT_STEP,				\
		.win_size = ((u64)period << KAIR_WINDOW_SIZE_SHIFT),	\
		.pc_cnt	= 0,						\
		.irand = KAIR_DIVERGING,				\
		.weight = KAIR_INST_TPDF_WEIGHT,			\
		.cache = tfifo_init(&__pdf->cache),			\
		.tabling = kair_tpdf_tabling,				\
		.untabling = kair_tpdf_untabling,			\
		.rv_register = kair_tpdf_rv_register,			\
		.rv_unregister = kair_tpdf_rv_unregister,		\
		.interpolator = linear_interpolator,			\
		.equilibrator = kair_tpdf_equilibrator,			\
		.rescaler = kair_tpdf_rescaler,				\
	};								\
	__err = __pdf->tabling(__pdf);					\
	__err;								\
})

/**
 * @kair_l1_pdf_init() : instant generative TPDF covering only about past 1 second.
 * @kair_l2_pdf_init() : generative TPDF convering only about past 1 minute.
 * @kair_l3_pdf_init() : ever lasting generative TPDF
 **/
#define kair_l1_pdf_init(pdf)	tpdf_instant_init((pdf), l1, KAIR_TPDF_CUMSUM)
#define kair_l2_pdf_init(pdf)	tpdf_finite_init((pdf), l2, KAIR_TPDF_CUMSUM << 1)
#define kair_l3_pdf_init(pdf)	tpdf_infinite_init((pdf), l3)

/**
 * KAIR window information extractors
 **/
#define is_kair_window_infty(pdf)					\
({									\
	typeof((pdf) + 1) __pdf = (pdf);				\
	(__pdf->win_size & KAIR_WINDOW_INF_MASK);			\
})

#define kair_window_size(pdf)						\
({									\
	typeof((pdf) + 1) __pdf = (pdf);				\
	((__pdf->win_size & KAIR_WINDOW_SIZE_MASK) >> KAIR_WINDOW_SIZE_SHIFT);	\
})

#define kair_windowing_cnt(pdf)						\
({									\
	typeof((pdf) + 1) __pdf = (pdf);				\
	(__pdf->win_size & KAIR_WINDOW_CNT_MASK);			\
})

#define kair_window_need_rescale(pdf)					\
({									\
	typeof((pdf) + 1) __pdf = (pdf);				\
	(__pdf->win_size & KAIR_WINDOW_NEED_RESCALE);			\
})

#define kair_window_rescale_done(pdf)					\
({									\
	typeof((pdf) + 1) __pdf = (pdf);				\
	__pdf->win_size &= ~KAIR_WINDOW_NEED_RESCALE;			\
})

/*.............................................................................
 *.............................................................................
 *................ """ KAIR Major External interfaces """ ....................
 *.............................................................................
 *...........................................................................*/

struct kair_class *kair_obj_creator(const char *alias,
				    unsigned int resilience,
				    unsigned int max_capa,
				    unsigned int min_capa,
				    struct kairistics *kairistic);
void kair_obj_destructor(struct kair_class *self);


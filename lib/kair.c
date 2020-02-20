/**
 *
 * KAIR - Library for Kernel perspective AI based Resource Control
 *
 * COPYRIGHT(C) Samsung Electronics Co., Ltd. All Right Reserved.
 *
 * Implementation of ECAVE - Elastic Capacity Vessel - theory to abstract of
 * arbitrary systems whose capacity is self regulated by purely observing the
 * behavior of object coming from outside of the system.
 *
 **/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/kair.h>

#include "d2u_decl_cmtpdf.h"

#define __INFTY			(0xEFEFEFEF)

#define POSITIVE_STRETCH	(0x80000000)
#define NEGATIVE_STRETCH	(0x40000000)
#define STRETCH_MASK		(0xC0000000)
#define HARMONIC_MASK		(0x3FFFFFFF)
#define ONE2TWO_RES		(64)

#define LOG_UPSCALE		(512)
#define LOG_UPSCALE_SHIFT	(9)

#define ASYMMETRIC_INFERENCE

const unsigned int __lb2[ONE2TWO_RES] =
	{ 11,  22,  33,  44,  54,  65,  75,  85,  95, 105, 115, 125, 134, 144,
	 153, 162, 171, 180, 189, 198, 206, 215, 223, 232, 240, 248, 256, 264,
	 272, 280, 288, 295, 303, 310, 318, 325, 332, 340, 347, 354, 361, 368,
	 375, 381, 388, 395, 401, 408, 414, 421, 427, 434, 440, 446, 452, 459,
	 465, 471, 477, 483, 488, 494, 500, 506};

typedef enum {
	KLD_LPLANE,
	KLD_RPLANE,
} kld_plane;

/*.............................................................................
 *.............................................................................
 *............. """ sysfs interfaces with an Kair instance """" ..............
 *.............................................................................
 *...........................................................................*/

/**
 * HTML skeletons for building Kair PDF description template
 **/
const char __hcmp_head[] = "<!DOCTYPE html><html><head><title>Kair TPDF description</title></head><body lang=\"en\"><table width=\"100\%\" cellpadding=\"4\" cellspacing=\"0\">";
const char __hcmp_entr[] = "<tr valign=i\"top\"><td><p>level ";
const char __hcmp_flr1[] = " TPDF shape</p><p><br></p><p>- randomness : ";
const char __hcmp_flr2[] = "</p><p>- observation interval : ";
const char __hcmp_flr3[] = "</p><p>- weight : ";
const char __hcmp_iter[] = "</p></td><td><p>";
const char __hcmp_clsr[] = "</p></td></tr>";
const char __hcmp_foot[] = "</table></body></html>\n";

/**
 * @abst_kobj  : /abstract entry direcly placed beneath /sys.
 **/
struct kobject *abst_kobj;

/**
 * KAIR instance can be dynamically created by anonymous clients.
 * SYSFS interface to the particular instance will be handled through an
 * exclusive form of kset in sysfs /sys/abst/kair, which means /kair is
 * defined as a primitive abstraction node.
 **/
static struct kset *krinst_set;

static unsigned int kair_cap_soft_bettor(struct kair_class *self,
					 struct rand_var *v,
					 unsigned int cap_legacy);
static unsigned int kair_cap_strict_bettor(struct kair_class *self,
					   struct rand_var *v,
					   unsigned int cap_legacy);

static ssize_t krinst_attr_show(struct kobject *kobj,
				struct attribute *attr,
				char *buf)
{
	struct krinst_attribute *attribute = to_krinst_attr(attr);
	struct krinst_obj *node = to_krinst_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(node, attribute, buf);
}

static ssize_t krinst_attr_store(struct kobject *kobj,
				 struct attribute *attr,
				 const char *buf, size_t len)
{
	struct krinst_attribute *attribute = to_krinst_attr(attr);
	struct krinst_obj *node = to_krinst_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(node, attribute, buf, len);
}

static const struct sysfs_ops krinst_sysfs_ops = {
	.show = krinst_attr_show,
	.store = krinst_attr_store,
};

static void krinst_release(struct kobject *kobj)
{
	struct krinst_obj *node = to_krinst_obj(kobj);

	kfree(node);
}

static ssize_t choke_cnt_show(struct krinst_obj *node,
			      struct krinst_attribute *attr,
			      char *buf)
{
	if (node->inst)
		return sprintf(buf, "%d\n", node->inst->stats.choke_cnt);
	else
		return sprintf(buf, "dangled reference\n");
}
KRINST_ATTR_RO(choke_cnt);

static ssize_t save_total_show(struct krinst_obj *node,
			       struct krinst_attribute *attr,
			       char *buf)
{
	if (node->inst)
		return sprintf(buf, "%lld\n", node->inst->stats.save_total);
	else
		return sprintf(buf, "dangled reference\n");
}
KRINST_ATTR_RO(save_total);

static ssize_t epsilon_show(struct krinst_obj *node,
			    struct krinst_attribute *attr,
			    char *buf)
{
	if (node->inst)
		return sprintf(buf, "%d\n", node->inst->epsilon);
	else
		return sprintf(buf, "dangled reference\n");
}

static ssize_t epsilon_store(struct krinst_obj *node,
			     struct krinst_attribute *attr,
			     const char *buf, size_t count)
{
	int retval = 0;

	if (node->inst) {
		retval = kstrtouint(buf, 10, &node->inst->epsilon);
		if (retval < 0)
			return retval;
	} else
		return -EFAULT;
		
	return count;
}
KRINST_ATTR_RW(epsilon);

static ssize_t resilience_show(struct krinst_obj *node,
			       struct krinst_attribute *attr,
			       char *buf)
{
	if (node->inst)
		return sprintf(buf, "%d\n", node->inst->resilience);
	else
		return sprintf(buf, "dangled reference\n");
}
KRINST_ATTR_RO(resilience);

static ssize_t rand_neutral_show(struct krinst_obj *node,
				 struct krinst_attribute *attr,
				 char *buf)
{
	if (node->inst)
		return sprintf(buf, "%d\n", node->inst->stats.rand_neutral);
	else
		return sprintf(buf, "dangled reference\n");
}

static ssize_t rand_neutral_store(struct krinst_obj *node,
				  struct krinst_attribute *attr,
				  const char *buf, size_t count)
{
	int retval = 0;

	if (node->inst) {
		retval = kstrtouint(buf, 10, &node->inst->stats.rand_neutral);
		if (retval < 0)
			return retval;
	} else
		return -EFAULT;
		
	return count;
}
KRINST_ATTR_RW(rand_neutral);

static ssize_t throttling_show(struct krinst_obj *node,
			       struct krinst_attribute *attr,
			       char *buf)
{
	if (node->inst)
		return sprintf(buf, "%d\n", node->inst->stats.throttling);
	else
		return sprintf(buf, "dangled reference\n");
}
KRINST_ATTR_RO(throttling);

static ssize_t bettor_select_show(struct krinst_obj *node,
				  struct krinst_attribute *attr,
				  char *buf)
{
	if (node->inst) {
		if (node->inst->cap_bettor == kair_cap_soft_bettor)
			return sprintf(buf, "soft\n");
		else
			return sprintf(buf, "strict\n");
	} else
		return sprintf(buf, "dangled reference\n");
}

static ssize_t bettor_select_store(struct krinst_obj *node,
				   struct krinst_attribute *attr,
				   const char *buf, size_t count)
{
	if (node->inst) {
		if (!strncmp(buf, "soft", 4))
			node->inst->cap_bettor = kair_cap_soft_bettor;
		else if (!strncmp(buf, "strict", 6))
			node->inst->cap_bettor = kair_cap_strict_bettor;
	} else
		return -EFAULT;

	return count;
}
KRINST_ATTR_RW(bettor_select); 

static ssize_t tpdf_cascade_show(struct krinst_obj *node,
				 struct krinst_attribute *attr,
				 char *buf)
{
	char *btemp = NULL;
	char *ptemp = NULL;
	struct tpdf *level;
	unsigned int inc, run;
	struct list_head *head;

	if (node->inst) {
		/**
		 * Allocating memory for HTML template
		 **/
		btemp = kzalloc(PAGE_SIZE, GFP_KERNEL);
		if (!btemp)
			return -1;
		else
			ptemp = btemp;

		ptemp += sprintf(ptemp, "%s", __hcmp_head);
		head = &node->inst->tpdf_cascade;

		for (level = list_first_entry(head, struct tpdf, pos), inc = 1;
		     &level->pos != head;
		     level = list_next_entry(level, pos), inc++) {
			ptemp += sprintf(ptemp, "%s", __hcmp_entr);
			ptemp += sprintf(ptemp, "%d", inc);
			ptemp += sprintf(ptemp, "%s", __hcmp_flr1);
			ptemp += sprintf(ptemp, "%d", level->irand);
			ptemp += sprintf(ptemp, "%s", __hcmp_flr2);
			ptemp += sprintf(ptemp, "%lld", kair_window_size(level));
			ptemp += sprintf(ptemp, "%s", __hcmp_flr3);
			ptemp += sprintf(ptemp, "x%d", level->weight);

			ptemp += sprintf(ptemp, "%s", __hcmp_iter);
			for (run = 0; run < level->qlvl; run++) {
				ptemp += sprintf(ptemp, "%d", level->qtbl[run]);
				ptemp += sprintf(ptemp, "%s", __hcmp_iter);
			}
			ptemp += sprintf(ptemp, "%s", __hcmp_clsr);
		}

		ptemp += sprintf(ptemp, "%s", __hcmp_foot);
		memcpy(buf, btemp, PAGE_SIZE);
		kfree(btemp);
	}

	return (ssize_t)(ptemp - btemp);
}
KRINST_ATTR_RO(tpdf_cascade);

static struct attribute *krinst_default_attrs[] = {
	&choke_cnt_attr.attr,
	&save_total_attr.attr,
	&epsilon_attr.attr,
	&resilience_attr.attr,
	&rand_neutral_attr.attr,
	&throttling_attr.attr,
	&bettor_select_attr.attr,
	&tpdf_cascade_attr.attr,
	NULL,
};

/**
 * KAIR exclusive ktype for our kobjects
 **/
static struct kobj_type krinst_ktype = {
	.sysfs_ops = &krinst_sysfs_ops,
	.release = krinst_release,
	.default_attrs = krinst_default_attrs,
};

static struct krinst_obj *create_krinst_obj(struct kair_class *src)
{
	struct krinst_obj *node;
	int retval;

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node)
		return NULL;

	/* Linking @ parent kset /sys/kair */
	node->obj.kset = krinst_set;
	node->inst = src;

	retval = kobject_init_and_add(&node->obj, &krinst_ktype,
				      &krinst_set->kobj, "%s",
				      (const char *)src->alias);
	if (retval) {
		kobject_put(&node->obj);
		return NULL;
	}

	/* Sending the uevent that the kobject was added to the system */
	kobject_uevent(&node->obj, KOBJ_ADD);

	return node;
}

static void destroy_krinst_obj(struct krinst_obj *node)
{
	node->inst = NULL;

	kobject_del(&node->obj);
	kobject_put(&node->obj);
}

static int __init kair_prepare(void)
{
	abst_kobj = kobject_create_and_add("abst", NULL);
	if (!abst_kobj)
		goto s1_error;

	krinst_set = kset_create_and_add("kair", NULL, abst_kobj);
	if (!krinst_set)
		goto s2_error;

	return 0;
s2_error:
	kobject_put(abst_kobj);
s1_error:
	return -ENOMEM;
}
postcore_initcall(kair_prepare);

static void __exit kair_clean(void)
{
	kset_unregister(krinst_set);

	kobject_put(abst_kobj);
}
__exitcall(kair_clean);

/*.............................................................................
 *.............................................................................
 *... """ KAIR-wise Temporal Probability Density Function Manipulators """ ....
 *.............................................................................
 *...........................................................................*/

/**
 * """ KAIR TPDF cascading method """
 **/
void kair_tpdf_cascading(struct kair_class *vessel, struct tpdf *element)
{
	list_add(&element->pos, &vessel->tpdf_cascade);
}
EXPORT_SYMBOL(kair_tpdf_cascading);

/**
 * """ KAIR TPDF cleaning method """
 **/
void kair_tpdf_cleaning(struct kair_class *vessel)
{
	struct tpdf *cur, *tmp;

	if (!list_empty(&vessel->tpdf_cascade)) {
		list_for_each_entry_safe(cur, tmp, &vessel->tpdf_cascade, pos) {
			list_del(&cur->pos);

			cur->untabling(cur);
			cur->rv_unregister(cur);

			kfree(cur);
		}
	}
}
EXPORT_SYMBOL(kair_tpdf_cleaning);

/**
 * """ KAIR default API to build 3-level TPDF cascading """
 **/
int kair_build_tpdf_cascade(struct kair_class *vessel)
{
	unsigned int weight_sum;

#if KAIR_TPDF_CASCADE_LEVEL > 1
	struct tpdf *tpdf_inf, *tpdf_fin, *tpdf_ins;

	tpdf_inf = kmalloc(sizeof(struct tpdf), GFP_KERNEL);
	if (!tpdf_inf)
		goto l3_nomem;

	tpdf_fin = kmalloc(sizeof(struct tpdf), GFP_KERNEL);
	if (!tpdf_fin)
		goto l2_nomem;

	tpdf_ins = kmalloc(sizeof(struct tpdf), GFP_KERNEL);
	if (!tpdf_ins)
		goto l1_nomem;

	kair_l3_pdf_init(tpdf_inf);
	kair_l2_pdf_init(tpdf_fin);
	kair_l1_pdf_init(tpdf_ins);

	kair_tpdf_cascading(vessel, tpdf_inf);
	kair_tpdf_cascading(vessel, tpdf_fin);
	kair_tpdf_cascading(vessel, tpdf_ins);

	weight_sum = tpdf_inf->weight + tpdf_fin->weight + tpdf_ins->weight;
	goto exit;

l1_nomem:
	kfree(tpdf_fin);
l2_nomem:
	kfree(tpdf_inf);
l3_nomem:
	return -ENOMEM;

exit:
#else
	struct tpdf *tpdf_ins;

	tpdf_ins = kmalloc(sizeof(struct tpdf), GFP_KERNEL);
	if (!tpdf_ins)
		return -ENOMEM;

	kair_l1_pdf_init(tpdf_ins);
	kair_tpdf_cascading(vessel, tpdf_ins);
	weight_sum = tpdf_ins->weight;
#endif

	vessel->capa_denom = ilog2(weight_sum + 1);
	return 0;
}
EXPORT_SYMBOL(kair_build_tpdf_cascade);

/**
 * """ KAIR TPDF availability checker """
 **/
static inline bool is_tpdf_prepared(struct tpdf *self)
{
	bool ret;

	if (is_kair_window_infty(self))
		ret = (kair_windowing_cnt(self) >= KAIR_TPDF_CUMSUM)
			? true : false;
	else
		ret = (kair_windowing_cnt(self) >= kair_window_size(self))
			? true : false;

	return ret;
}

/**
 * """ KAIR default TPDF total equilibrator """
 **/
void kair_tpdf_equilibrator(struct tpdf *self)
{
	unsigned int cursor;

	self->pc_cnt++;

	if (self->pc_cnt == KAIR_TPDF_CUMSUM) {
		for (cursor = 0; cursor < self->qlvl; cursor++)
			self->qtbl[cursor] >>= 1;
		self->pc_cnt = 0;
	}
}
EXPORT_SYMBOL(kair_tpdf_equilibrator);

/**
 * """ KAIR default TPDF rescaler """
 * The rescaling ratio is calculated 'kair_TPDF_CUMSUM / kair_window_size()'
 **/
void kair_tpdf_rescaler(struct tpdf *self)
{
	unsigned int cursor;
	unsigned int denom = (unsigned int)kair_window_size(self);

	if (kair_window_need_rescale(self)) {
		for (cursor = 0; cursor < self->qlvl; cursor++)
			self->qtbl[cursor] = mult_frac(self->qtbl[cursor],
						KAIR_TPDF_CUMSUM, denom);
		kair_window_rescale_done(self);
	}
}
EXPORT_SYMBOL(kair_tpdf_rescaler);

/**
 * """ KAIR default TPDF tabling procedure """
 * Which is mainly composed of;
 * - memory allocation
 * - copying the existing TPDF image from storage to runtime memory
 * - address mapping to self
 **/
int kair_tpdf_tabling(struct tpdf *self)
{
	self->qtbl = kzalloc(sizeof(int) * self->qlvl, GFP_KERNEL);
	if (!self->qtbl)
		return -ENOMEM;

	if (!is_kair_window_infty(self)) {
		if (tfifo_alloc(&self->cache, kair_window_size(self)) == false)
			return -ENOMEM;
	}
	return 0;
}
EXPORT_SYMBOL(kair_tpdf_tabling);

/**
 * """ KAIR default TPDF untabling procedure """
 * Which is mainly composed of;
 * - memory free
 * - store the built TPDF image to storage
 * - address unmapping
 **/
void kair_tpdf_untabling(struct tpdf *self)
{
	kfree(self->qtbl);

	tfifo_free(&self->cache);
}
EXPORT_SYMBOL(kair_tpdf_untabling);

void kair_tpdf_rv_register(struct tpdf *self, struct rand_var *rv)
{
	self->rv = rv;
}
EXPORT_SYMBOL(kair_tpdf_rv_register);

void kair_tpdf_rv_unregister(struct tpdf *self)
{
	self->rv = NULL;
}
EXPORT_SYMBOL(kair_tpdf_rv_unregister);

/*.............................................................................
 *.............................................................................
 *.................. """ Generic Mathematical Functions """ ...................
 *.............................................................................
 *...........................................................................*/

/**
 * """ Caculating Kullback-Liebler diversity """
 * Evaluating how much two random variables are similar. this is actually done
 * by taking full integral on the relative entropy.
 * @comparer : TPDF structure quantized by its own qlvl.
 * @comparee : comparee's raw integer TPDF table quantized by comparer's qlvl.
 **/
int kl_diversity(struct tpdf *comparer, unsigned int *comparee)
{
	int kl_div = 0;
	unsigned int idx;
	unsigned int p, q, pbase, qbase, entp, entq;
	int log2p, log2q;

	for (idx = 0; idx < comparer->qlvl; idx++) {
		p = comparer->qtbl[idx];
		q = comparee[idx];

		/* Q(i)=0 implies P(i)=0 */
		if (p == 0 || q == 0)
			continue;

		log2p = ilog2(p);
		log2q = ilog2(q);
		pbase = 1 << log2p;
		qbase = 1 << log2q;
		entp = (p == pbase) ? (p * log2p) :
			((p * log2p) +
				((p * __lb2[mult_frac(p - pbase, ONE2TWO_RES, pbase)])
					>> LOG_UPSCALE_SHIFT));
		entq = (q == qbase) ? (p * log2q) :
			((p * log2q) +
				((p * __lb2[mult_frac(q - qbase, ONE2TWO_RES, qbase)])
					>> LOG_UPSCALE_SHIFT));

		/**
		 * If the probability of comparer random variable at a scope is
		 * bigger than the one's of comparee, KL diversity will be increased
		 * as far as the relative entropy caculated differs, otherwise,
		 * KL diversity will be fairly decreased.
		 **/
		kl_div += (entp - entq);
	}

	return kl_div;
}
EXPORT_SYMBOL(kl_diversity);

/**
 * """ Caculating Asymmetric Kullback-Liebler diversity """
 * Evaluating how much two random variables are similar. this is actually done
 * by taking full integral on the relative entropy.
 * @comparer : TPDF structure quantized by its own qlvl.
 * @comparee : comparee's raw integer TPDF table quantized by comparer's qlvl.
 **/
int kl_asymm_diversity(struct tpdf *comparer, unsigned int *comparee, kld_plane side)
{
	int kl_div = 0;
	unsigned int idx;
	unsigned int mean = comparer->qlvl >> 1;
	unsigned int head = (side == KLD_LPLANE) ? 0 : mean;
	unsigned int foot = (side == KLD_LPLANE) ? mean : comparer->qlvl - 1;
	unsigned int p, q, pbase, qbase, entp, entq;
	int log2p, log2q;

	for (idx = head; idx <= foot; idx++) {
		p = comparer->qtbl[idx];
		q = comparee[idx];

		/* Q(i)=0 implies P(i)=0 */
		if (p == 0 || q == 0)
			continue;

		log2p = ilog2(p);
		log2q = ilog2(q);
		pbase = 1 << log2p;
		qbase = 1 << log2q;
		entp = (p == pbase) ? (p * log2p) :
			((p * log2p) +
				((p * __lb2[mult_frac(p - pbase, ONE2TWO_RES, pbase)])
					>> LOG_UPSCALE_SHIFT));
		entq = (q == qbase) ? (p * log2q) :
			((p * log2q) +
				((p * __lb2[mult_frac(q - qbase, ONE2TWO_RES, qbase)])
					>> LOG_UPSCALE_SHIFT));

		/**
		 * If the probability of comparer random variable at a scope is
		 * bigger than the one's of comparee, KL diversity will be increased
		 * as far as the relative entropy caculated differs, otherwise,
		 * KL diversity will be fairly decreased.
		 **/
		kl_div += (entp - entq);
	}

	return kl_div;
}
EXPORT_SYMBOL(kl_asymm_diversity);

/**
 * """ Default linear interpolator """
 * Returning interpolated value;
 * @self    : temporal probability density function
 * @top_pos : top position
 * @raw_pos : raw position
 **/
unsigned int linear_interpolator(struct tpdf *self, unsigned int top_pos,
				 unsigned int raw_pos)
{
	unsigned int psec = top_pos >> ilog2(self->qlvl);
	unsigned int lpos = raw_pos >> psec;
	unsigned int lval = self->qtbl[lpos];
	unsigned int rval = self->qtbl[lpos + 1];
	unsigned int delta = raw_pos - (lpos << psec);

	return (delta * lval + (top_pos - delta) * rval) >> psec;
}
EXPORT_SYMBOL(linear_interpolator);

/*.............................................................................
 *.............................................................................
 *................... """ KAIR capsulated functions """ ......................
 *.............................................................................
 *...........................................................................*/

/**
 * Under assuming the function kl-diversity to given TPDF-TPDF comparison is
 * quite noisy-convex, an alternative greedy gradient decsending algorithm
 * is exhibited here.
 * @targ  :
 * @mold  :
 **/
static inline randomness __gs_on_kld(struct tpdf *targ,
				     unsigned int **mold,
				     unsigned int max_var)
{
	int cur;
	int min_idx = 0;
	int min_kld = __INFTY;

	int lmost = 0;
	int rmost = (int)max_var - 1;

	int width = (int)max_var;
	int hop_total = ilog2((unsigned int)width);
	int hop_width = (1 << hop_total) / hop_total;
	int kld_ref[16];

	memset(kld_ref, 0xEF, sizeof(kld_ref));
	
	/**
	 * Complexity : O(N x log N * log(log N) x ...)
	 **/
	do {
		for (cur = lmost; cur <= (rmost + 1); cur += hop_width) {
			cur = cur > rmost ? rmost : cur;

#ifndef ASYMMETRIC_INFERENCE
			if (kld_ref[cur] == __INFTY)
				kld_ref[cur] = kl_diversity(targ,
					(unsigned int *)mold + cur * KAIR_QUANT_STEP);

			if (abs(kld_ref[cur]) < abs(min_kld)) {
				min_kld = kld_ref[cur];
				min_idx = cur;
			}
#else
			if (kld_ref[cur] == __INFTY) {
				kld_ref[cur] = kl_asymm_diversity(targ,
					(unsigned int *)mold + cur * KAIR_QUANT_STEP, KLD_RPLANE);
			}

			if (abs(kld_ref[cur]) < abs(min_kld)) {
				min_kld = kld_ref[cur];
				min_idx = cur;
			}
#endif
		}

		lmost = max(min_idx - hop_width + 1, lmost);
		rmost = min(min_idx + hop_width - 1, rmost);

		width = rmost - lmost + 1;
		hop_total = ilog2((unsigned int)width);
		hop_width = (1 << hop_total) / hop_total;
	} while (rmost - lmost > 2);

	return (randomness)min_idx;
}

/**
 * """ KAIR default searcher of the template TPDF """
 * Nearest resembling to given 
 **/
static int kair_search_nearest_job_tpdf(struct kair_class *self)
{
	struct tpdf *level;

	if (list_empty(&self->tpdf_cascade))
		return KAIR_DIVERGING;

	list_for_each_entry(level, &self->tpdf_cascade, pos) {
		if (!is_tpdf_prepared(level))
			continue;

		if (level->pc_cnt)
			continue;

		level->irand = __gs_on_kld(level,
					   (unsigned int **)d2u_decl_cmtpdf,
					   self->resilience);
	}

	level = list_first_entry(&self->tpdf_cascade, struct tpdf, pos);
	return level->irand;
}

/**
 *
 **/
static inline void kair_tpdf_carving_out(struct tpdf *self)
{
	unsigned int scope;
	unsigned int mult = kair_window_size(self) / KAIR_TPDF_CUMSUM;

	self->rescaler(self);

	self->pc_cnt++;

	scope = tfifo_out(&self->cache);
	self->qtbl[scope] = self->qtbl[scope] ? self->qtbl[scope] - 1 : 0;

	if (self->pc_cnt == kair_window_size(self) / mult)
		self->pc_cnt = 0;
}

/**
 * """ KAIR job probability collapse operator """
 * Which takes place of generative learning what temporal probability density
 * function of given job intensity looks like.
 **/
static void kair_job_probability_collapse(struct kair_class *self,
					  struct rand_var *v)
{
	struct tpdf *level;
	unsigned int scope;
	bool in_choke = (v->nval == v->ubound);

	if (list_empty(&self->tpdf_cascade))
		return;

	if (in_choke) {
		self->stats.choke_cnt++;
	} else {
		list_for_each_entry(level, &self->tpdf_cascade, pos) {
			scope = (unsigned int)RV_TAILORED(level, v);

			level->qtbl[scope]++;

			if (is_kair_window_infty(level)) {
				if (kair_windowing_cnt(level) >= KAIR_TPDF_CUMSUM)
					level->equilibrator(level);
				else
					level->win_size++;
			} else {
				tfifo_in(&level->cache, scope);

				if (kair_windowing_cnt(level) >= kair_window_size(level))
					kair_tpdf_carving_out(level);
				else
					level->win_size++;
			}
		}
	}
}

/**
 * taking integral on Sunchul PDF from given i to maximum
 **/
static inline unsigned int __integral_schi2m(randomness r, unsigned int i)
{
	unsigned int total = 0;
	unsigned int x;

	for (x = i; x < KAIR_QUANT_STEP; x++)
		total += d2u_decl_cmtpdf[(unsigned int)r][x];

	return total;
}

/**
 * """ KAIR default desirable capacity bettor """
 * Betting reasonable kair capacity per job intensity induced outside by
 * stochastic processing of job intensity's centered-mean TPDF.
 * @self
 * @v
 * @cap_legacy :
 **/
static unsigned int kair_cap_soft_bettor(struct kair_class *self,
					 struct rand_var *v,
					 unsigned int cap_legacy)
{
	struct tpdf *level;
	unsigned int min_cap = mult_frac(cap_legacy,
					 self->kairistic.theta_numer,
					 self->kairistic.theta_denom);
	unsigned int max_cap = mult_frac(cap_legacy,
					 self->kairistic.gamma_numer,
					 self->kairistic.gamma_denom);
	unsigned int new_cap = 0;

	if (list_empty(&self->tpdf_cascade))
		return cap_legacy;

	if (cap_legacy == 0)
		return 0;

	list_for_each_entry(level, &self->tpdf_cascade, pos) {
		if (level->irand == KAIR_DIVERGING) {
			new_cap = cap_legacy;
		} else {
			new_cap += level->weight *
					((level->irand > self->stats.rand_neutral) ?
					 max_cap >> self->capa_denom : min_cap >> self->capa_denom);
		}
	}

	self->stats.save_total += ((long long)cap_legacy - (long long)new_cap) / 10;

	return new_cap;
}

static unsigned int kair_cap_strict_bettor(struct kair_class *self,
					   struct rand_var *v,
					   unsigned int cap_legacy)
{
	struct tpdf *level;
	unsigned int new_cap = mult_frac(cap_legacy,
					 self->kairistic.theta_numer,
					 self->kairistic.theta_denom);
	unsigned int max_cap = mult_frac(cap_legacy,
					 self->kairistic.gamma_numer,
					 self->kairistic.gamma_denom);
	unsigned int skew;
	unsigned int run_hmny;
	unsigned int cap_unit = (max_cap - new_cap) >> self->capa_denom;

	s32 cap_delta = 0;

	if (list_empty(&self->tpdf_cascade))
		return cap_legacy;

	if (cap_legacy == 0)
		return 0;

	list_for_each_entry(level, &self->tpdf_cascade, pos) {
		unsigned int scope = RV_TAILORED(level, v);
		unsigned int min_hmny = __INFTY;
		unsigned int opt_skew = __INFTY;

		if (level->irand == KAIR_DIVERGING)
			continue;

		for (skew = 0; skew < self->resilience; skew++) {
			unsigned int __cap = cap_legacy + cap_unit * (level->irand - skew);

			run_hmny = self->epsilon * __integral_schi2m(skew, scope) + __cap;

			if (min_hmny > run_hmny) {
				min_hmny = run_hmny;
				opt_skew = skew;
			}
		}

		cap_delta = cap_unit * (level->irand - opt_skew);
		new_cap += cap_delta * level->weight;
	}

	self->stats.save_total += ((long long)cap_legacy - (long long)new_cap) / 10;

	return new_cap;
}

/**
 * """ KAIR default instance initializer """
 * Making stuffs inside of kair instance ready. for now, preparing
 * the multi-leveled TPDF cascade.
 * @self : instance itself
 **/
static int kair_initializer(struct kair_class *self)
{
	int retval = 0;

	retval = kair_build_tpdf_cascade(self);
	if (retval < 0)
		return retval;

	self->extif = create_krinst_obj(self);
	if (!self->extif)
		return -ENOMEM;

	return retval; 
}

/**
 * """ KAIR default instance stopper """
 * A part of suspend sequence. Reset TPDF as well.
 * @self : instance itself
 **/
static void kair_stopper(struct kair_class *self)
{
	struct tpdf *cur;

	if (!list_empty(&self->tpdf_cascade)) {
		list_for_each_entry(cur, &self->tpdf_cascade, pos) {
			if (!is_kair_window_infty(cur)) {
				memset(cur->qtbl, 0, sizeof(int) * cur->qlvl);

				tfifo_free(&cur->cache);
				tfifo_alloc(&cur->cache, kair_window_size(cur));

				cur->pc_cnt = 0;
			}
		}
	}
}

/**
 * """ KAIR default instance finalizer """
 * Making the kair instance ready for the removal. for now, releasing resources
 * occupied by the multi-leveled TPDF cascade structure.
 **/
static void kair_finalizer(struct kair_class *self)
{
	if (self->extif)
		destroy_krinst_obj(self->extif);

	kair_tpdf_cleaning(self);
}

/*.............................................................................
 *.............................................................................
 *................ """ KAIR Major External interfaces """ .....................
 *.............................................................................
 *...........................................................................*/

/**
 * """ kair_obj_creator() """
 * Creates an kair_class instance and returns the address.
 **/
struct kair_class *kair_obj_creator(const char *alias,
				    unsigned int resilience,
				    unsigned int max_capa,
				    unsigned int min_capa,
				    struct kairistics *kairistic)
{
	struct kair_class *vessel = kzalloc(sizeof(struct kair_class), GFP_ATOMIC);

	if (!vessel)
		return 0;

	*vessel = (struct kair_class) {
		.df_velocity	= KAIR_UNDETERMINED,
		.resilience	= resilience,
		.ep_ratio	= KAIR_BALANCED_EP_RATIO,
		.max_capa	= max_capa,
		.max_capa	= min_capa,
		.epsilon	= 43000,
		.kairistic	= *kairistic,

		.tpdf_cascade	= LIST_HEAD_INIT(vessel->tpdf_cascade),
		.capa_denom	= 0,
		.stats		= {0, 0L, KAIR_DEF_RAND_NEUTRAL, 0},

		.initializer	= kair_initializer,
		.stopper	= kair_stopper,
		.finalizer	= kair_finalizer,
		.job_learner	= kair_job_probability_collapse,
		.job_inferer	= kair_search_nearest_job_tpdf,
		.cap_bettor	= kair_cap_soft_bettor
	};

	memcpy(&vessel->alias, alias, KAIR_ALIAS_LEN);

	return vessel;
}
EXPORT_SYMBOL(kair_obj_creator);

/**
 * """ kair_obj_destructor() """
 * Releases the system resources occupied by the instance itself.
 * @self : the target kair_class instance
 **/
void kair_obj_destructor(struct kair_class *self)
{
	kfree(self);
}
EXPORT_SYMBOL(kair_obj_destructor);

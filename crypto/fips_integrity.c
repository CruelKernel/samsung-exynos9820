/*
 * Perform FIPS Integrity test on Kernel Crypto API
 *
 */

#include <linux/crypto.h>
#include <linux/kallsyms.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <crypto/hash.h>
#include "internal.h"

/*
 * This function return kernel offset.
 * If CONFIG_RELOCATABLE_KERNEL is set
 * Then the kernel will be placed into the random offset in memory
 * At the build time, we write self address of the "crypto_buildtime_address"
 * to the "crypto_buildtime_address" variable
 */
static __u64 get_kernel_offset(void)
{
	static __u64 runtime_address = (__u64) &crypto_buildtime_address;
	__u64 kernel_offset = abs(crypto_buildtime_address - runtime_address);
	return kernel_offset;
}

struct sdesc {
	struct shash_desc shash;
	char ctx[];
};

static struct sdesc *init_hash(void)
{
	struct crypto_shash *tfm = NULL;
	struct sdesc *sdesc = NULL;
	int size;
	int ret = -1;

	/* Same as build time */
	const unsigned char *key = "The quick brown fox jumps over the lazy dog";

	tfm = crypto_alloc_shash("hmac(sha256)", 0, 0);

	if (IS_ERR(tfm)) {
		pr_err("FIPS(%s): integ failed to allocate tfm %ld\n", __func__, PTR_ERR(tfm));
		return ERR_PTR(-EINVAL);
	}

	ret = crypto_shash_setkey(tfm, key, strlen(key));

	if (ret) {
		pr_err("FIPS(%s): fail at crypto_hash_setkey\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	size = sizeof(struct shash_desc) + crypto_shash_descsize(tfm);
	sdesc = kmalloc(size, GFP_KERNEL);
	if (!sdesc)
		return ERR_PTR(-ENOMEM);

	sdesc->shash.tfm = tfm;
	sdesc->shash.flags = 0x0;

	ret = crypto_shash_init(&sdesc->shash);

	if (ret) {
		pr_err("FIPS(%s): fail at crypto_hash_init\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	return sdesc;
}

static int
finalize_hash(struct shash_desc *desc, unsigned char *out, unsigned int out_size)
{
	int ret = -1;

	if (!desc || !desc->tfm || !out || !out_size) {
		pr_err("FIPS(%s): Invalid args\n", __func__);
		return ret;
	}

	if (crypto_shash_digestsize(desc->tfm) > out_size) {
		pr_err("FIPS(%s): Not enough space for digest\n", __func__);
		return ret;
	}

	ret = crypto_shash_final(desc, out);

	if (ret) {
		pr_err("FIPS(%s): crypto_hash_final failed\n", __func__);
		return -1;
	}

	return 0;
}

static int
update_hash(struct shash_desc *desc, unsigned char *start_addr, unsigned int size)
{
	int ret = -1;

	ret = crypto_shash_update(desc, start_addr, size);

	if (ret) {
		pr_err("FIPS(%s): crypto_hash_update failed\n", __func__);
		return -1;
	}

#ifdef CONFIG_CRYPTO_FIPS_FUNC_TEST
	if (!strcmp("integrity", get_fips_functest_mode())) {
		ret = crypto_shash_update(desc, start_addr, 4);

		if (ret) {
			pr_err("FIPS(%s): crypto_hash_update failed\n", __func__);
			return -1;
		}
	}
#endif

	return 0;
}

int
do_integrity_check(void)
{
	int i, rows;
	int err = -1;
	unsigned long start_addr = 0;
	unsigned long end_addr   = 0;
	unsigned char runtime_hmac[32];
	struct sdesc *sdesc = NULL;
	const char *builtime_hmac = 0;
	unsigned int size = 0;
#ifdef FIPS_DEBUG_INTEGRITY
	unsigned int covered = 0;
	unsigned int num_addresses = 0;
#endif

	sdesc = init_hash();

	if (IS_ERR(sdesc)) {
		pr_err("FIPS(%s) : init_hash failed\n", __func__);
		return -1;
	}

	rows = (unsigned int) ARRAY_SIZE(integrity_crypto_addrs);

	for (i = 0; integrity_crypto_addrs[i].first && i < rows; i++) {
		start_addr = integrity_crypto_addrs[i].first + get_kernel_offset();
		end_addr   = integrity_crypto_addrs[i].last  + get_kernel_offset();

/* Print addresses for HMAC calculation */
#ifdef FIPS_DEBUG_INTEGRITY
		pr_info("FIPS_DEBUG_INTEGRITY : first last %08llux %08llulx\n",
			integrity_crypto_addrs[i].first,
			integrity_crypto_addrs[i].last);
		covered += (end_addr - start_addr);
#endif

		size = end_addr - start_addr;
		err = update_hash(&sdesc->shash, (unsigned char *)start_addr, size);

		if (err) {
			pr_err("FIPS(%s) : Error to update hash\n", __func__);
			crypto_free_shash(sdesc->shash.tfm);
			kzfree(sdesc);
			return -1;
		}
	}

	/* Dump bytes for HMAC */
#ifdef FIPS_DEBUG_INTEGRITY
	num_addresses = i;
	for (i = 0; integrity_crypto_addrs[i].first && i < rows; i++) {

		start_addr = integrity_crypto_addrs[i].first + get_kernel_offset();
		end_addr   = integrity_crypto_addrs[i].last  + get_kernel_offset();
		size = end_addr - start_addr;
		print_hex_dump(KERN_INFO, "FIPS_DEBUG_INTEGRITY : bytes for HMAC = ", DUMP_PREFIX_NONE,
			16, 1,
			(char *)start_addr, size, false);
	}
#endif

	err = finalize_hash(&sdesc->shash, runtime_hmac, sizeof(runtime_hmac));

	crypto_free_shash(sdesc->shash.tfm);
	kzfree(sdesc);

	if (err) {
		pr_err("FIPS(%s) : Error in finalize\n", __func__);
		return -1;
	}

	builtime_hmac = builtime_crypto_hmac;

#ifdef FIPS_DEBUG_INTEGRITY
	pr_info("FIPS_DEBUG_INTEGRITY : %d bytes are covered, Address fragments (%d)", covered, num_addresses);
	print_hex_dump(KERN_INFO, "FIPS_DEBUG_INTEGRITY : runtime hmac  = ", DUMP_PREFIX_NONE,
		16, 1,
		runtime_hmac, sizeof(runtime_hmac), false);
	print_hex_dump(KERN_INFO, "FIPS_DEBUG_INTEGRITY : builtime_hmac = ", DUMP_PREFIX_NONE,
		16, 1,
		builtime_hmac, sizeof(runtime_hmac), false);
#endif

	if (!memcmp(builtime_hmac, runtime_hmac, sizeof(runtime_hmac)))
		return 0;
	else
		return -1;
}
EXPORT_SYMBOL_GPL(do_integrity_check);

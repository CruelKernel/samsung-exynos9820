/*
 Name        : kbkdf.c
 Copyright   : Samsung Electronics
 Description :
    The kbkdf.c library file.
   "NIST Special Publication 800-108
    Recommendation for Key Derivation
    Using Pseudorandom Functions"

    NIST SP 800-108 Key Derivation Function (KDF). Counter mode.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <crypto/kbkdf.h>
#include <crypto/hash.h>
#include "internal.h"

#define HMAC_SHA512_BACKEND_CRA_NAME "hmac(sha512-generic)"

typedef struct shash_desc hmac_ctx_t;

static int HMAC_CTX_init(hmac_ctx_t **hmac_ctx)
{
	struct crypto_shash *tfm = NULL;

	tfm = crypto_alloc_shash(HMAC_SHA512_BACKEND_CRA_NAME, 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	*hmac_ctx = kzalloc(sizeof(hmac_ctx_t) + crypto_shash_descsize(tfm),
			GFP_KERNEL);
	if (!*hmac_ctx) {
		crypto_free_shash(tfm);
		return -ENOMEM;
	}

	(*hmac_ctx)->tfm = tfm;
	(*hmac_ctx)->flags = 0;

	return 0;
}

static int HMAC_Init_ex(hmac_ctx_t *hmac_ctx, const uint8_t *Ki, unsigned int intKiLength)
{
	int ret_code = 0;

	ret_code = crypto_shash_setkey(hmac_ctx->tfm, Ki, intKiLength);
	if(ret_code)
		return ret_code;

	ret_code = crypto_shash_init(hmac_ctx);
	if(ret_code)
		return ret_code;

	return 0;
}

static int HMAC_Update(hmac_ctx_t *hmac_ctx, const uint8_t *data, unsigned int data_len)
{
	return crypto_shash_update(hmac_ctx, data, data_len);
}

static int HMAC_Final(hmac_ctx_t *hmac_ctx, uint8_t *data, unsigned int *data_len)
{
	int ret_code = 0;

	ret_code = crypto_shash_final(hmac_ctx, data);
	if (ret_code)
		return ret_code;

	*data_len = crypto_shash_digestsize(hmac_ctx->tfm);
	return 0;
}

static void HMAC_CTX_cleanup(hmac_ctx_t *hmac_ctx)
{
	if (hmac_ctx) {
		crypto_free_shash(hmac_ctx->tfm);
		kzfree(hmac_ctx);
	}
}

static void changeEndianess(uint8_t *variable, size_t length);

/*
 * @brief The KDF in Counter Mode, used HMAC with SHA512.
 *
 * @param [in]  mode        The mode of execution, see kbkdf.h header file.
 * @param [in]  rlen        The length for Iteration Counter (i) in bytes,
 *                          see kbkdf.h header file.
 * @param [in]  *Ki         Key derivation key, a key that is used as an
 *                          input to a key derivation function.
 * @param [in]  KiLength    The length of key derivation key in bytes.
 * @param [out] *Ko         Keying material output from a key derivation function.
 * @param [out] *L          The length of requested output keying material
 *                          in bytes, returned actual output keying material
 *                          length in bytes that may be higher or equal the
 *                          requested length. Max and min values of length specified:
 *                          MIN_KDF_OUTPUT_LENGTH_BYTES and MAX_KDF_OUTPUT_LENGTH_BYTES.
 * @param [in]  *Label      A string that identifies the purpose for the
 *                          derived keying material, which is encoded as
 *                          a binary string.
 * @param [in] LabelLength  The length of 'Label' in bytes.
 * @param [in] *Context     A binary string containing the information
 *                          related to the derived keying material.
 * @param [in] ContextLength  The length of 'Context' in bytes.
 *
 * @return                  0 on success, nonzero on error
 */
int crypto_calc_kdf_hmac_sha512_ctr(uint8_t mode, size_t rlen,
    const uint8_t *Ki, size_t KiLength, uint8_t *Ko, uint32_t *L,
    const uint8_t *Label, size_t LabelLength,
    const uint8_t *Context, size_t ContextLength)
{
    int             ret_code;
    uint32_t        i;
    hmac_ctx_t     *hmac_ctx = NULL;
    unsigned int    h_len;
    uint8_t        *fixed_data_buff  = NULL;
    uint8_t        *tmp_ptr;
    size_t          fixed_data_length;

#ifdef CONFIG_CRYPTO_FIPS
	if (unlikely(in_fips_err()))
		return -EACCES;
#endif

    if((mode != KDF_DEFAULT &&
        mode != KDF_TEST_CTRLOCATION_BEFORE_FIXED &&
        mode != KDF_TEST_CTRLOCATION_MIDDLE_FIXED &&
        mode != KDF_TEST_CTRLOCATION_AFTER_FIXED ) ||
       (rlen != KDF_RLEN_08BIT &&
        rlen != KDF_RLEN_16BIT &&
        rlen != KDF_RLEN_24BIT &&
        rlen != KDF_RLEN_32BIT) ||
        Ki == NULL || KiLength == 0 || Ko == NULL || L == NULL )
        return -EINVAL;

    // Initialize HMAC engine context structure.
    ret_code = HMAC_CTX_init(&hmac_ctx);
    if(ret_code)
    	return ret_code;

    ret_code = -EPERM;
    do {
        // Checking the minimal length (in bytes)
        // of the derived keying material (L).
        if( *L < MIN_KDF_OUTPUT_LENGTH_BYTES ) {
            *L = MIN_KDF_OUTPUT_LENGTH_BYTES;
        }
        else if( *L > MAX_KDF_OUTPUT_LENGTH_BYTES ) {
                 *L = MAX_KDF_OUTPUT_LENGTH_BYTES;
        }
        else {
            // Make L divisible by SHA512_DIGEST_LENGTH (Actual length)
            *L = round_up(*L, SHA512_DIGEST_LENGTH);
        }

        if( mode == KDF_DEFAULT ) {     // Default mode of KDF execution.
            // Calculate full fixed data length with all records:
            // i || Label || 0x00 || Context || L.
            fixed_data_length  = rlen +  LabelLength +  1 +
                    ContextLength +  sizeof(uint32_t);

            // Allocate memory for fixed data buffer.
            if((fixed_data_buff  = kzalloc(fixed_data_length, GFP_KERNEL)) == NULL ) {
                ret_code = -ENOMEM;
                break;
            }

            // Fill all required records of fixed data for NIST default mode.
            tmp_ptr  = fixed_data_buff;

            memset(tmp_ptr, 0, rlen);   // i
            tmp_ptr += rlen;

            if( LabelLength != 0 ) {    // Label
                memcpy(tmp_ptr, Label, LabelLength);
                tmp_ptr += LabelLength;
            }

           *tmp_ptr  = 0x00;            // 0x00 delimiter
            tmp_ptr += 1;
                                        // Context
            if( ContextLength != 0 ) {
                memcpy(tmp_ptr, Context, ContextLength);
                tmp_ptr += ContextLength;
            }

            i  = *L *  8;               // L in bits
            memcpy(tmp_ptr, (void *)&i, sizeof(uint32_t));
            tmp_ptr += sizeof(uint32_t);
        }
        else                            // All NIST test modes of KDF execution.
        if( mode == KDF_TEST_CTRLOCATION_BEFORE_FIXED ||
            mode == KDF_TEST_CTRLOCATION_MIDDLE_FIXED ||
            mode == KDF_TEST_CTRLOCATION_AFTER_FIXED  ) {
            // Calculate full fixed data length
            // for test modes
            fixed_data_length  = rlen +  LabelLength +  ContextLength;

            // Allocate memory for fixed data buffer.
            if((fixed_data_buff  = kzalloc(fixed_data_length, GFP_KERNEL)) == NULL ) {
                ret_code  = -ENOMEM;
                break;
            }
            // Fill all required records of fixed data for all test modes.
            tmp_ptr  = fixed_data_buff;
            if( mode == KDF_TEST_CTRLOCATION_BEFORE_FIXED ) {
                                        // i
                memset(tmp_ptr, 0, rlen);
                tmp_ptr += rlen;
                                        // Label
                if( LabelLength != 0 ) {
                    memcpy(tmp_ptr, Label, LabelLength);
                    tmp_ptr += LabelLength;
                }
            }
            else
            if( mode == KDF_TEST_CTRLOCATION_MIDDLE_FIXED ||
                mode == KDF_TEST_CTRLOCATION_AFTER_FIXED  ) {
                                        // Label
                if( LabelLength != 0 ) {
                    memcpy(tmp_ptr, Label, LabelLength);
                    tmp_ptr += LabelLength;
                }
                                        // i
                memset(tmp_ptr, 0, rlen);
                tmp_ptr += rlen;
                                        // Context
                if( ContextLength != 0 ) {
                    memcpy(tmp_ptr, Context, ContextLength);
                    tmp_ptr += ContextLength;
                }
            }
        }

        // Execute KDF in Counter Mode cycle.
        for(i = 1, h_len = 0;
            i <= (*L / SHA512_DIGEST_LENGTH);
            i++, Ko += h_len) {

            // Initialize HMAC engine context.
        	ret_code = HMAC_Init_ex(hmac_ctx, Ki, KiLength);
            if( ret_code != 0 ) {
                break;
            }
                                        // Default mode of KDF execution.
            if( mode == KDF_DEFAULT ) { // i
                memcpy(fixed_data_buff, (void *)&i, rlen);
            }
            else                        // All NIST test modes of KDF execution.
            if( mode == KDF_TEST_CTRLOCATION_BEFORE_FIXED ||
                mode == KDF_TEST_CTRLOCATION_MIDDLE_FIXED ||
                mode == KDF_TEST_CTRLOCATION_AFTER_FIXED  ) {
                if( mode == KDF_TEST_CTRLOCATION_BEFORE_FIXED ) {
                    tmp_ptr  = fixed_data_buff;
                }
                else
                if( mode == KDF_TEST_CTRLOCATION_MIDDLE_FIXED ||
                    mode == KDF_TEST_CTRLOCATION_AFTER_FIXED  ) {
                        tmp_ptr  = fixed_data_buff +  LabelLength;
                    }
                // Update Iteration Counter (i) for test modes.
                memcpy(tmp_ptr, (void *)&i, rlen);
                changeEndianess(tmp_ptr, rlen);
            }

            ret_code = HMAC_Update(hmac_ctx, fixed_data_buff, fixed_data_length);
            if( ret_code != 0 ) {
                break;
            }

            ret_code = HMAC_Final(hmac_ctx, Ko, &h_len);
            if( ret_code != 0 ) {
                break;
            }
        }
    } while(0);

    // Release allocated memory.
    if( fixed_data_buff != NULL )
    	kzfree((void*)fixed_data_buff);

    // Clenup HMAC engine.
    HMAC_CTX_cleanup(hmac_ctx);

    return ret_code;
}
EXPORT_SYMBOL_GPL(crypto_calc_kdf_hmac_sha512_ctr);

/*
 *  Change endianess for variable with specified length.
 */
static void changeEndianess(uint8_t *variable, size_t length)
{
    size_t     i;
    uint8_t     byte;
    uint8_t    *tail;

    if( variable == NULL || length <  2 )
        return;

    for(i  = 0, tail  = variable +  (length -  1);
            i <  length -  1; i ++, tail --, variable ++) {
        byte  = *variable;
        *variable  = *tail;
        *tail  = byte;
    }

    return;
}

static int __init kdf_hmac_sha512_ctr_module_init(void)
{
	/* TODO: driver register */
	return 0;
}

static void __exit kdf_hmac_sha512_ctr_module_exit(void)
{
	/* TODO: driver unregister */
	return;
}

module_init(kdf_hmac_sha512_ctr_module_init);
module_exit(kdf_hmac_sha512_ctr_module_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("KDF in Counter Mode, used HMAC with SHA512");
MODULE_ALIAS_CRYPTO("kdf_hmac(sha512)_ctr");

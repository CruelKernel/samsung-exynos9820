/*
 Name        : kbkdf.h
 Copyright   : Samsung Electronics
 Description :
    The kbkdf.h header file.
   "NIST Special Publication 800-108
    Recommendation for Key Derivation
    Using Pseudorandom Functions"

    NIST SP 800-108 Key Derivation Function (KDF). Counter mode.
*/

#ifndef KBKDF_H_
#define KBKDF_H_

#include <linux/kernel.h>

/*===========================================================================
 *  Definitions of constants.
  ===========================================================================*/


//  Default mode of KDF execution, all records in Fixed Input Data Block
//  used as specified in NIST Special Publication 800-108, (sp800-108.pdf,
//  i.5.1, 'process, i.4').
#define KDF_DEFAULT                             0
//  Test mode of KDF execution for test vectors from NIST KDFCTR_gen.txt,
//  Iteration Counter (i) with RLEN (8, 16, 32 bits), that passed in 'rlen',
//  saved before or after Fixed Data Block, that passed in 'Context' argument.
//  If 'KDF_TEST_CTRLOCATION_MIDDLE_FIXED' selected,
//  the 'DataBeforeCtrData' and 'DataBeforeCtrLen'
//  must be passed in 'Label' and 'LabelLength', and
//  the 'DataAfterCtrData' and 'DataAfterCtrLen'
//  must be passed in 'Context' and 'ContextLength'.
//  For any other test modes used only 'Label' and 'LabelLength' respectively.
#define KDF_TEST_CTRLOCATION_BEFORE_FIXED       1
#define KDF_TEST_CTRLOCATION_MIDDLE_FIXED       2
#define KDF_TEST_CTRLOCATION_AFTER_FIXED        3

//  The Iteration Counter (i) length (RLEN) in bytes for default and test modes.
#define KDF_RLEN_08BIT                          1
#define KDF_RLEN_16BIT                          2
#define KDF_RLEN_24BIT                          3
#define KDF_RLEN_32BIT                          4

/*
 * Min and Max output data length for KDF in bits and bytes (implementation dependent).
 * NIST Special Publication 800-108 restrict max output only by max possible counter value.
 */
#define MIN_KDF_OUTPUT_LENGTH_BITS  512
#define MAX_KDF_OUTPUT_LENGTH_BITS  4096 // For SHA512.

#define MIN_KDF_OUTPUT_LENGTH_BYTES (MIN_KDF_OUTPUT_LENGTH_BITS /  8)
#define MAX_KDF_OUTPUT_LENGTH_BYTES (MAX_KDF_OUTPUT_LENGTH_BITS /  8)

/* SHA512_DIGEST_LENGTH is the length of a SHA-512 digest. */
#define SHA512_DIGEST_LENGTH 64

/*
 * @brief The KDF in Counter Mode, used HMAC with SHA512.
 * Supports Keying material output with length divisible by 64 bytes.
 *
 * @param [in]  mode        The mode of execution.
 * @param [in]  rlen        The length for Iteration Counter (i) in bytes,
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
 *
 */
int crypto_calc_kdf_hmac_sha512_ctr(uint8_t mode, size_t rlen,
    const uint8_t *Ki, size_t KiLength, uint8_t *Ko, uint32_t *L,
    const uint8_t *Label, size_t LabelLength,
    const uint8_t *Context, size_t ContextLength);

#endif                                  // KBKDF_H_

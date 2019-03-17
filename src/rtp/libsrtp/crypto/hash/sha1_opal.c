/*
 * sha1_opal.c
 *
 * an implementation of the Secure Hash Algorithm v.1 (SHA-1),
 * specified in FIPS 180-1
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifndef OPENSSL

  #include "sha1.c"

#else

#include "sha1.h"

srtp_debug_module_t srtp_mod_sha1 = {
    0,      /* debugging is off by default */
    "sha-1" /* printable module name       */
};

void srtp_sha1(const uint8_t *msg, int octets_in_msg, uint32_t hash_value[5])
{
    srtp_sha1_ctx_t ctx;

    srtp_sha1_init(&ctx);
    srtp_sha1_update(&ctx, msg, octets_in_msg);
    srtp_sha1_final(&ctx, hash_value);
}

#endif
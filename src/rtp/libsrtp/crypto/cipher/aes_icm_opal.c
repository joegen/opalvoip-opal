/*
 * aes_icm_opal.c
 *
 * AES Integer Counter Mode
 */

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#ifdef OPENSSL
  #include "aes_icm_ossl.c"
  #include "aes_gcm_ossl.c"
#else
  #include "aes_icm.c"
#endif

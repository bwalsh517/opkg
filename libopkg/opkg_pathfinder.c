/* vi: set noexpandtab sw=4 sts=4: */
/* opkg_download.c - the opkg package management system

   Carl D. Worth

   Copyright (C) 2001 University of Southern California
   Copyright (C) 2008 OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/
#include "config.h"

#include <openssl/ssl.h>
#include <libpathfinder.h>
#include "includes.h"
#include "opkg_message.h"

#if defined(HAVE_SSLCURL)
#include <curl/curl.h>
#endif

#if defined(HAVE_SSLCURL) || defined(HAVE_OPENSSL)
/*
 *      This callback is called instead of X509_verify_cert to perform path
 *      validation on a certificate using pathfinder.
 *
 */
static int pathfinder_verify_callback(X509_STORE_CTX *ctx, void *arg)
{
    char *errmsg;
    const char *hex = "0123456789ABCDEF";
    size_t size = i2d_X509(ctx->cert, NULL);
    unsigned char *keybuf, *iend;
    iend = keybuf = malloc(size);
    i2d_X509(ctx->cert, &iend);
    char *certdata_str = malloc(size * 2 + 1);
    unsigned char *cp = keybuf;
    char *certdata_str_i = certdata_str;
    while (cp < iend)
    {
        unsigned char ch = *cp++;
        *certdata_str_i++ = hex[(ch >> 4) & 0xf];
        *certdata_str_i++ = hex[ch & 0xf];
    }
    *certdata_str_i = 0;
    free(keybuf);

    const char *policy = "2.5.29.32.0"; // anyPolicy
    int validated = pathfinder_dbus_verify(certdata_str, policy, 0, 0, &errmsg);

    if (!validated)
        fprintf(stderr, "curlcb_pathfinder: Path verification failed: %s", errmsg);

    free(certdata_str);
    free(errmsg);

    return validated;
}
#endif


#if defined(HAVE_OPENSSL)
int pkcs7_pathfinder_verify_signers(PKCS7* p7)
{
    STACK_OF(X509) *signers;
    int i;

    signers = PKCS7_get0_signers(p7, NULL, 0);

    for(i = 0; i < sk_X509_num(signers); i++){
	X509_STORE_CTX ctx = {
	    .cert = sk_X509_value(signers, i),
	};

	if(!pathfinder_verify_callback(&ctx, NULL))
	    return 0;
    }

    return 1;
}
#endif

#if defined(HAVE_SSLCURL)
CURLcode curl_ssl_ctx_function(CURL * curl, void * sslctx, void * parm) {

  SSL_CTX * ctx = (SSL_CTX *) sslctx;
  SSL_CTX_set_cert_verify_callback(ctx, pathfinder_verify_callback, parm);

  return CURLE_OK ;
}
#endif

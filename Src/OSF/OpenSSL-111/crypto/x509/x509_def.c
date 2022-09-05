/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
#include "internal/cryptlib.h"
#pragma hdrstop

const char * X509_get_default_private_dir() { return X509_PRIVATE_DIR; }
const char * X509_get_default_cert_area() { return X509_CERT_AREA; }
const char * X509_get_default_cert_dir() { return X509_CERT_DIR; }
const char * X509_get_default_cert_file() { return X509_CERT_FILE; }
const char * X509_get_default_cert_dir_env() { return X509_CERT_DIR_EVP; }
const char * X509_get_default_cert_file_env() { return X509_CERT_FILE_EVP; }

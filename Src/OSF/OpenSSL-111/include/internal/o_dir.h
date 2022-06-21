/*
 * Copyright 2004-2018 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at  https://www.openssl.org/source/license.html
 */

/*
 * This file is dual-licensed and is also available under the following
 * terms:
 *
 * Copyright (c) 2004, Richard Levitte <richard@levitte.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 */
#ifndef O_DIR_H
#define O_DIR_H

typedef struct OPENSSL_dir_context_st OPENSSL_DIR_CTX;

/*
 * returns NULL on error or end-of-directory. If it is end-of-directory,
 * errno will be zero
 */
const char *OPENSSL_DIR_read(OPENSSL_DIR_CTX **ctx, const char *directory);
/* returns 1 on success, 0 on error */
int OPENSSL_DIR_end(OPENSSL_DIR_CTX **ctx);

#endif /* LPDIR_H */

/* Copyright (C) Simo Sorce <simo@redhat.com>
 * See COPYING file for License */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>

#include <sasl.h>
#include <saslutil.h>

void s_error(const char *hdr, ssize_t ret, ssize_t len, int err);
void send_string(int sd, const char *s, unsigned int l);
void recv_string(int sd, char *buf, unsigned int *buflen);
void saslerr(int why, const char *what);
int getpath(void *context __attribute__((unused)), const char **path);
void parse_cb(sasl_channel_binding_t *cb, char *buf, unsigned max, char *in);

/* Copyright (C) Simo Sorce <simo@redhat.com>
 * See COPYING file for License */

#include <t_common.h>

void s_error(const char *hdr, ssize_t ret, ssize_t len, int err)
{
    fprintf(stderr, "%s l:%ld/%ld [%d] %s",
            hdr, ret, len, err, strerror(err));
    exit(-1);
}

void send_string(int sd, const char *s, unsigned int l)
{
    ssize_t ret;

    ret = send(sd, &l, sizeof(l), 0);
    if (ret != sizeof(l)) s_error("send size", ret, sizeof(l), errno);

    if (l == 0) return;

    ret = send(sd, s, l, 0);
    if (ret != l) s_error("send data", ret, l, errno);
}

void recv_string(int sd, char *buf, unsigned int *buflen)
{
    unsigned int l;
    ssize_t ret;

    ret = recv(sd, &l, sizeof(l), MSG_WAITALL);
    if (ret != sizeof(l)) s_error("recv size", ret, sizeof(l), errno);

    if (l == 0) {
        *buflen = 0;
        return;
    }

    if (*buflen < l) s_error("recv len", l, *buflen, E2BIG);

    ret = recv(sd, buf, l, 0);
    if (ret != l) s_error("recv data", ret, l, errno);

    *buflen = ret;
}

void saslerr(int why, const char *what)
{
    fprintf(stderr, "%s: %s", what, sasl_errstring(why, NULL, NULL));
}

int getpath(void *context __attribute__((unused)), const char **path)
{
    if (! path) {
        return SASL_BADPARAM;
    }

    *path = PLUGINDIR;
    return SASL_OK;
}

void parse_cb(sasl_channel_binding_t *cb, char *buf, unsigned max, char *in)
{
    unsigned len;
    int r;

    r = sasl_decode64(in, strlen(in), buf, max, &len);
    if (r != SASL_OK) {
        saslerr(r, "failed to parse channel bindings");
        exit(-1);
    }
    cb->name = "TEST BINDINGS";
    cb->critical = 0;
    cb->data = (unsigned char *)buf;
    cb->len = len;
}

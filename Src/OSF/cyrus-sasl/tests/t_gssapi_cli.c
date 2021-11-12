/* Copyright (C) Simo Sorce <simo@redhat.com>
 * See COPYING file for License */

#include "t_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <arpa/inet.h>
#include <saslplug.h>
#include <saslutil.h>

const char *testpass = NULL;

static int setup_socket(void)
{
    struct sockaddr_in addr;
    int sock, ret;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) s_error("socket", 0, 0, errno);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.9");
    addr.sin_port = htons(9000);

    ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) s_error("connect", 0, 0, errno);

    return sock;
}

static int get_user(void *context __attribute__((unused)),
                  int id,
                  const char **result,
                  unsigned *len)
{
    const char *testuser = "test@host.realm.test";

    if (! result)
        return SASL_BADPARAM;

    switch (id) {
    case SASL_CB_USER:
    case SASL_CB_AUTHNAME:
        *result = testuser;
        break;
    default:
        return SASL_BADPARAM;
    }

    if (len) *len = strlen(*result);

    return SASL_OK;
}

static int get_pass(sasl_conn_t *conn __attribute__((unused)),
          void *context __attribute__((unused)),
          int id,
          sasl_secret_t **psecret)
{
    size_t len;
    static sasl_secret_t *x;

    /* paranoia check */
    if (! conn || ! psecret || id != SASL_CB_PASS)
        return SASL_BADPARAM;

    len = strlen(testpass);

    x = (sasl_secret_t *) realloc(x, sizeof(sasl_secret_t) + len);

    if (!x) {
        return SASL_NOMEM;
    }

    x->len = len;
    strcpy((char *)x->data, testpass);

    *psecret = x;
    return SASL_OK;
}

int main(int argc, char *argv[])
{
    sasl_callback_t callbacks[4] = {};
    char buf[8192];
    const char *chosenmech;
    sasl_conn_t *conn;
    const char *data;
    unsigned int len;
    sasl_channel_binding_t cb = {0};
    char cb_buf[256];
    int sd;
    int c, r;
    const char *sasl_mech = "GSSAPI";
    int plain = 0;

    while ((c = getopt(argc, argv, "c:P:")) != EOF) {
        switch (c) {
        case 'c':
            parse_cb(&cb, cb_buf, 256, optarg);
            break;
        case 'P':
            plain = 1;
            testpass = optarg;
            break;
        default:
            break;
        }
    }

    /* initialize the sasl library */
    callbacks[0].id = SASL_CB_GETPATH;
    callbacks[0].proc = (sasl_callback_ft)&getpath;
    callbacks[0].context = NULL;
    callbacks[1].id = SASL_CB_LIST_END;
    callbacks[1].proc = NULL;
    callbacks[1].context = NULL;
    callbacks[2].id = SASL_CB_LIST_END;
    callbacks[2].proc = NULL;
    callbacks[2].context = NULL;
    callbacks[3].id = SASL_CB_LIST_END;
    callbacks[3].proc = NULL;
    callbacks[3].context = NULL;

    if (plain) {
        sasl_mech = "PLAIN";

        callbacks[1].id = SASL_CB_AUTHNAME;
        callbacks[1].proc = (sasl_callback_ft)&get_user;

        callbacks[2].id = SASL_CB_PASS;
        callbacks[2].proc = (sasl_callback_ft)&get_pass;
    }

    r = sasl_client_init(callbacks);
    if (r != SASL_OK) exit(-1);

    r = sasl_client_new("test", "host.realm.test", NULL, NULL, NULL, 0, &conn);
    if (r != SASL_OK) {
        saslerr(r, "allocating connection state");
        exit(-1);
    }

    if (cb.name) {
        sasl_setprop(conn, SASL_CHANNEL_BINDING, &cb);
    }

    r = sasl_client_start(conn, sasl_mech, NULL, &data, &len, &chosenmech);
    if (r != SASL_OK && r != SASL_CONTINUE) {
        saslerr(r, "starting SASL negotiation");
        printf("\n%s\n", sasl_errdetail(conn));
        exit(-1);
    }

    sd = setup_socket();

    while (r == SASL_CONTINUE) {
        send_string(sd, data, len);
        len = 8192;
        recv_string(sd, buf, &len);

        r = sasl_client_step(conn, buf, len, NULL, &data, &len);
        if (r != SASL_OK && r != SASL_CONTINUE) {
            saslerr(r, "performing SASL negotiation");
            printf("\n%s\n", sasl_errdetail(conn));
            exit(-1);
        }
    }

    if (r != SASL_OK) exit(-1);

    if (len > 0) {
        send_string(sd, data, len);
    }

    fprintf(stdout, "DONE\n");
    fflush(stdout);
    return 0;
}


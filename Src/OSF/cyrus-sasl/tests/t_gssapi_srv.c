/* Copyright (C) Simo Sorce <simo@redhat.com>,
 * Dmitry Belyavskiy <dbelyavs@redhat.com>
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

const char *sasldb_path = NULL,
      *auxprop_plugin = "sasldb",
      *pwcheck_method = "auxprop-hashed";

static int setup_socket(void)
{
    struct sockaddr_in addr;
    int sock, ret, sd;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) s_error("socket", 0, 0, errno);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.9");
    addr.sin_port = htons(9000);

    ret = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret != 0) s_error("bind", 0, 0, errno);

    ret = listen(sock, 1);
    if (ret != 0) s_error("listen", 0, 0, errno);

    /* signal we are ready */
    fprintf(stdout, "READY\n");
    fflush(stdout);

    /* block until the client connects */
    sd = accept(sock, NULL, NULL);
    if (sd < 0) s_error("accept", 0, 0, errno);

    close(sock);
    return sd;
}

static int test_getopt(void *context __attribute__((unused)),
                const char *plugin_name __attribute__((unused)),
                const char *option,
                const char **result,
                unsigned *len)
{
    if (sasldb_path && !strcmp(option, "sasldb_path")) {
        *result = sasldb_path;
        if (len)
            *len = (unsigned) strlen(sasldb_path);
        return SASL_OK;
    }

    if (sasldb_path && !strcmp(option, "auxprop_plugin")) {
        *result = auxprop_plugin;
        if (len)
            *len = (unsigned) strlen(auxprop_plugin);
        return SASL_OK;
    }

    if (sasldb_path && !strcmp(option, "pwcheck_method")) {
        *result = pwcheck_method;
        if (len)
            *len = (unsigned) strlen(pwcheck_method);
        return SASL_OK;
    }
    return SASL_FAIL;
}

int main(int argc, char *argv[])
{
    sasl_callback_t callbacks[3] = {};
    char buf[8192];
    sasl_conn_t *conn;
    const char *data;
    unsigned int len;
    sasl_channel_binding_t cb = {0};
    unsigned char cb_buf[256];
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
            sasldb_path = optarg;
            break;
        default:
            break;
        }
    }

    /* initialize the sasl library */
    callbacks[0].id = SASL_CB_GETPATH;
    callbacks[0].proc = (sasl_callback_ft)&getpath;
    callbacks[0].context = NULL;
    callbacks[1].id = SASL_CB_GETOPT;
    callbacks[1].proc = (sasl_callback_ft)&test_getopt;
    callbacks[1].context = NULL;
    callbacks[2].id = SASL_CB_LIST_END;
    callbacks[2].proc = NULL;
    callbacks[2].context = NULL;

    r = sasl_server_init(callbacks, "t_gssapi_srv");
    if (r != SASL_OK) exit(-1);

    r = sasl_server_new("test", "host.realm.test", NULL, NULL, NULL,
                        callbacks, 0, &conn);
    if (r != SASL_OK) {
        saslerr(r, "allocating connection state");
        exit(-1);
    }

    if (cb.name) {
        sasl_setprop(conn, SASL_CHANNEL_BINDING, &cb);
    }

    if (plain) {
        sasl_mech = "PLAIN";
    }

    sd = setup_socket();

    len = 8192;
    recv_string(sd, buf, &len);

    r = sasl_server_start(conn, sasl_mech, buf, len, &data, &len);
    if (r != SASL_OK && r != SASL_CONTINUE) {
        saslerr(r, "starting SASL negotiation");
        printf("\n%s\n", sasl_errdetail(conn));
        exit(-1);
    }

    while (r == SASL_CONTINUE) {
        send_string(sd, data, len);
        len = 8192;
        recv_string(sd, buf, &len);

        r = sasl_server_step(conn, buf, len, &data, &len);
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


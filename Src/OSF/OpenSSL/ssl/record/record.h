/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */
// 
// These structures should be considered PRIVATE to the record layer. No
// non-record layer code should be using these structures in any way.
// 
typedef struct ssl3_buffer_st {
    unsigned char *buf; /* at least SSL3_RT_MAX_PACKET_SIZE bytes, see ssl3_setup_buffers() */
    size_t default_len; /* default buffer size (or 0 if no default set) */
    size_t len; /* buffer size */
    int offset; /* where to 'copy from' */
    int left; /* how many bytes left */
} SSL3_BUFFER;

#define SEQ_NUM_SIZE   8

typedef struct ssl3_record_st {
    /* Record layer version */
    /* r */
    int rec_version;
    /* type of record */
    /* r */
    int type;
    /* How many bytes available */
    /* rw */
    unsigned int length;
    /*
     * How many bytes were available before padding was removed? This is used
     * to implement the MAC check in constant time for CBC records.
     */
    /* rw */
    unsigned int orig_len;
    /* read/write offset into 'buf' */
    /* r */
    unsigned int off;
    /* pointer to the record data */
    /* rw */
    unsigned char *data;
    /* where the decode bytes are */
    /* rw */
    unsigned char *input;
    /* only used with decompression - malloc()ed */
    /* r */
    unsigned char *comp;
    /* Whether the data from this record has already been read or not */
    /* r */
    unsigned int read;
    /* epoch number, needed by DTLS1 */
    /* r */
    unsigned long epoch;
    /* sequence number, needed by DTLS1 */
    /* r */
    unsigned char seq_num[SEQ_NUM_SIZE];
} SSL3_RECORD;

typedef struct dtls1_bitmap_st {
    unsigned long map; /* Track 32 packets on 32-bit systems and 64 - on 64-bit systems */
    unsigned char max_seq_num[SEQ_NUM_SIZE]; /* Max record number seen so far, 64-bit value in big-endian encoding */
} DTLS1_BITMAP;

typedef struct record_pqueue_st {
    unsigned short epoch;
    struct pqueue_st *q;
} record_pqueue;

typedef struct dtls1_record_data_st {
    unsigned char *packet;
    unsigned int packet_length;
    SSL3_BUFFER rbuf;
    SSL3_RECORD rrec;
#ifndef OPENSSL_NO_SCTP
    struct bio_dgram_sctp_rcvinfo recordinfo;
#endif
} DTLS1_RECORD_DATA;

typedef struct dtls_record_layer_st {
    /*
     * The current data and handshake epoch.  This is initially
     * undefined, and starts at zero once the initial handshake is
     * completed
     */
    unsigned short r_epoch;
    unsigned short w_epoch;
    DTLS1_BITMAP bitmap; /* records being received in the current epoch */
    DTLS1_BITMAP next_bitmap; /* renegotiation starts a new set of sequence numbers */
    record_pqueue unprocessed_rcds; /* Received handshake records (processed and unprocessed) */
    record_pqueue processed_rcds;
    /*
     * Buffered application records. Only for records between CCS and
     * Finished to prevent either protocol violation or unnecessary message loss.
     */
    record_pqueue buffered_app_data;
    /*
     * storage for Alert/Handshake protocol data received but not yet
     * processed by ssl3_read_bytes:
     */
    unsigned char alert_fragment[DTLS1_AL_HEADER_LENGTH];
    unsigned int alert_fragment_len;
    unsigned char handshake_fragment[DTLS1_HM_HEADER_LENGTH];
    unsigned int handshake_fragment_len;
    /* save last and current sequence numbers for retransmissions */
    unsigned char last_write_sequence[8];
    unsigned char curr_write_sequence[8];
} DTLS_RECORD_LAYER;
// 
// This structure should be considered "opaque" to anything outside of the 
// record layer. No non-record layer code should be accessing the members of  this structure.
// 
typedef struct record_layer_st {
    SSL *s; /* The parent SSL structure */
    int read_ahead; // Read as many input bytes as possible (for non-blocking reads)
    int rstate; /* where we are when reading */
    unsigned int numrpipes; /* How many pipelines can be used to read data */
    unsigned int numwpipes; /* How many pipelines can be used to write data */
    SSL3_BUFFER rbuf; /* read IO goes into here */
    SSL3_BUFFER wbuf[SSL_MAX_PIPELINES]; /* write IO goes into here */
    SSL3_RECORD rrec[SSL_MAX_PIPELINES]; /* each decoded record goes in here */
    unsigned char *packet; /* used internally to point at a raw packet */
    unsigned int packet_length;
    unsigned int wnum; /* number of bytes sent so far */
    /*
     * storage for Alert/Handshake protocol data received but not yet
     * processed by ssl3_read_bytes:
     */
    unsigned char alert_fragment[2];
    unsigned int alert_fragment_len;
    unsigned char handshake_fragment[4];
    unsigned int handshake_fragment_len;
    unsigned int empty_record_count; /* The number of consecutive empty records we have received */
    /* partial write - check the numbers match */
    /* number bytes written */
    int wpend_tot;
    int wpend_type;
    int wpend_ret; /* number of bytes submitted */
    const unsigned char *wpend_buf;
    unsigned char read_sequence[SEQ_NUM_SIZE];
    unsigned char write_sequence[SEQ_NUM_SIZE];
    unsigned int is_first_record; /* Set to true if this is the first record in a connection */
    unsigned int alert_count; /* Count of the number of consecutive warning alerts received */
    DTLS_RECORD_LAYER *d;
} RECORD_LAYER;
// 
// The following macros/functions represent the libssl internal API to the
// record layer. Any libssl code may call these functions/macros
// 
#define MIN_SSL2_RECORD_LEN     9

#define RECORD_LAYER_set_read_ahead(rl, ra)     ((rl)->read_ahead = (ra))
#define RECORD_LAYER_get_read_ahead(rl)         ((rl)->read_ahead)
#define RECORD_LAYER_get_packet(rl)             ((rl)->packet)
#define RECORD_LAYER_get_packet_length(rl)      ((rl)->packet_length)
#define RECORD_LAYER_add_packet_length(rl, inc) ((rl)->packet_length += (inc))
#define DTLS_RECORD_LAYER_get_w_epoch(rl)       ((rl)->d->w_epoch)
#define DTLS_RECORD_LAYER_get_processed_rcds(rl) ((rl)->d->processed_rcds)
#define DTLS_RECORD_LAYER_get_unprocessed_rcds(rl) ((rl)->d->unprocessed_rcds)

void RECORD_LAYER_init(RECORD_LAYER *rl, SSL *s);
void RECORD_LAYER_clear(RECORD_LAYER *rl);
void RECORD_LAYER_release(RECORD_LAYER *rl);
int RECORD_LAYER_read_pending(const RECORD_LAYER *rl);
int RECORD_LAYER_write_pending(const RECORD_LAYER *rl);
int RECORD_LAYER_set_data(RECORD_LAYER *rl, const unsigned char *buf, int len);
void RECORD_LAYER_reset_read_sequence(RECORD_LAYER *rl);
void RECORD_LAYER_reset_write_sequence(RECORD_LAYER *rl);
int RECORD_LAYER_is_sslv2_record(RECORD_LAYER *rl);
unsigned int RECORD_LAYER_get_rrec_length(RECORD_LAYER *rl);
__owur int ssl3_pending(const SSL *s);
__owur int ssl3_write_bytes(SSL *s, int type, const void *buf, int len);
__owur int do_ssl3_write(SSL *s, int type, const unsigned char *buf, unsigned int *pipelens, unsigned int numpipes, int create_empty_fragment);
__owur int ssl3_read_bytes(SSL *s, int type, int *recvd_type, unsigned char *buf, int len, int peek);
__owur int ssl3_setup_buffers(SSL *s);
__owur int ssl3_enc(SSL *s, SSL3_RECORD *inrecs, unsigned int n_recs, int send);
__owur int n_ssl3_mac(SSL *ssl, SSL3_RECORD *rec, unsigned char *md, int send);
__owur int ssl3_write_pending(SSL *s, int type, const unsigned char *buf, unsigned int len);
__owur int tls1_enc(SSL *s, SSL3_RECORD *recs, unsigned int n_recs, int send);
__owur int tls1_mac(SSL *ssl, SSL3_RECORD *rec, unsigned char *md, int send);
int DTLS_RECORD_LAYER_new(RECORD_LAYER *rl);
void DTLS_RECORD_LAYER_free(RECORD_LAYER *rl);
void DTLS_RECORD_LAYER_clear(RECORD_LAYER *rl);
void DTLS_RECORD_LAYER_set_saved_w_epoch(RECORD_LAYER *rl, unsigned short e);
void DTLS_RECORD_LAYER_clear(RECORD_LAYER *rl);
void DTLS_RECORD_LAYER_resync_write(RECORD_LAYER *rl);
void DTLS_RECORD_LAYER_set_write_sequence(RECORD_LAYER *rl, unsigned char *seq);
__owur int dtls1_read_bytes(SSL *s, int type, int *recvd_type, unsigned char *buf, int len, int peek);
__owur int dtls1_write_bytes(SSL *s, int type, const void *buf, int len);
__owur int do_dtls1_write(SSL *s, int type, const unsigned char *buf, unsigned int len, int create_empty_fragement);
void dtls1_reset_seq_numbers(SSL *s, int rw);

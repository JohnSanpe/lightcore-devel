/* SPDX-License-Identifier: GPL-2.0 */

#include "../../../lib/compress/zlib_inflate/inftrees.c"
#include "../../../lib/compress/zlib_inflate/inffast.c"
#include "../../../lib/compress/zlib_inflate/inflate.c"

#ifdef CONFIG_ZLIB_DFLTCC
#include "zlib_dfltcc/dfltcc.c"
#include "zlib_dfltcc/dfltcc_inflate.c"
#endif

#define GZIP_IOBUF_SIZE (16 * 1024)

static long nofill(void *buffer, unsigned long len)
{
    return -1;
}

/* Included from initramfs et al code */
static int decompress(unsigned char *buf, long len,
            long (*fill)(void*, unsigned long),
            long (*flush)(void*, unsigned long),
            unsigned char *out_buf, long out_len,
            long *pos) {
    uint8_t *zbuf;
    struct z_stream_s *strm;
    int rc;

    rc = -1;

    if (flush) {
        out_len = 0x8000; /* 32 K */
        out_buf = malloc(out_len);
    } else {
        if (!out_len)
            out_len = ((size_t)~0) - (size_t)out_buf; /* no limit */
    }

    if (!out_buf) {
        pr_boot("out of memory while allocating output buffer\n");
        goto gunzip_nomem1;
    }

    if (buf)
        zbuf = buf;
    else {
        zbuf = malloc(GZIP_IOBUF_SIZE);
        len = 0;
    }

    if (!zbuf) {
        pr_boot("out of memory while allocating input buffer\n");
        goto gunzip_nomem2;
    }

    strm = malloc(sizeof(*strm));
    if (strm == NULL) {
        pr_boot("out of memory while allocating z_stream\n");
        goto gunzip_nomem3;
    }

    strm->workspace = malloc(flush ? zlib_inflate_workspacesize() :
#ifdef CONFIG_ZLIB_DFLTCC
    /* Always allocate the full workspace for DFLTCC */
                zlib_inflate_workspacesize());
#else
                sizeof(struct inflate_state));
#endif
    if (strm->workspace == NULL) {
        pr_boot("out of memory while allocating workspace\n");
        goto gunzip_nomem4;
    }

    if (!fill)
        fill = nofill;

    if (len == 0)
        len = fill(zbuf, GZIP_IOBUF_SIZE);

    /* verify the gzip header */
    if (len < 10 ||
    zbuf[0] != 0x1f || zbuf[1] != 0x8b || zbuf[2] != 0x08) {
        if (pos)
            *pos = 0;
        pr_boot("not a gzip file\n");
        goto gunzip_5;
    }

    /* skip over gzip header (1f,8b,08... 10 bytes total +
     * possible asciz filename)
     */
    strm->next_in = zbuf + 10;
    strm->avail_in = len - 10;
    /* skip over asciz filename */
    if (zbuf[3] & 0x8) {
        do {
            /*
             * If the filename doesn't fit into the buffer,
             * the file is very probably corrupt. Don't try
             * to read more data.
             */
            if (strm->avail_in == 0) {
                pr_boot("header error\n");
                goto gunzip_5;
            }
            --strm->avail_in;
        } while (*strm->next_in++);
    }

    strm->next_out = out_buf;
    strm->avail_out = out_len;

    rc = zlib_inflateInit2(strm, -MAX_WBITS);

#ifdef CONFIG_ZLIB_DFLTCC
    /* Always keep the window for DFLTCC */
#else
    if (!flush) {
        WS(strm)->inflate_state.wsize = 0;
        WS(strm)->inflate_state.window = NULL;
    }
#endif

    while (rc == Z_OK) {
        if (strm->avail_in == 0) {
            /* TODO: handle case where both pos and fill are set */
            len = fill(zbuf, GZIP_IOBUF_SIZE);
            if (len < 0) {
                rc = -1;
                pr_boot("read error\n");
                break;
            }
            strm->next_in = zbuf;
            strm->avail_in = len;
        }
        rc = zlib_inflate(strm, 0);

        /* Write any data generated */
        if (flush && strm->next_out > out_buf) {
            long l = strm->next_out - out_buf;
            if (l != flush(out_buf, l)) {
                rc = -1;
                pr_boot("write error\n");
                break;
            }
            strm->next_out = out_buf;
            strm->avail_out = out_len;
        }

        /* after Z_FINISH, only Z_STREAM_END is "we unpacked it all" */
        if (rc == Z_STREAM_END) {
            rc = 0;
            break;
        } else if (rc != Z_OK) {
            pr_boot("uncompression error\n");
            rc = -1;
        }
    }

    zlib_inflateEnd(strm);
    if (pos)
        /* add + 8 to skip over trailer */
        *pos = strm->next_in - zbuf+8;

gunzip_5:
    free(strm->workspace);
gunzip_nomem4:
    free(strm);
gunzip_nomem3:
    if (!buf)
        free(zbuf);
gunzip_nomem2:
    if (flush)
        free(out_buf);
gunzip_nomem1:
    return rc; /* returns Z_OK (0) if successful */
}

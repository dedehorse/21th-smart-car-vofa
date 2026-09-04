/*******************************************************************************
 *  @file     LQ_JPEG_Encoder.c
 *  @brief    Minimal JPEG Baseline Encoder for TC387 — RGB888 in, JPEG bytes out
 *
 *  Features:
 *   - YCbCr 4:4:4 color (no subsampling)
 *   - Fixed-point DCT (Q15)
 *   - Standard JPEG Huffman + quantization tables (quality ~75)
 *   - Output: valid JFIF JPEG byte stream, host auto-detects & renders
 *******************************************************************************/

#include "LQ_JPEG_Encoder.h"

/* ======================== Bitstream Writer ======================== */
typedef struct {
    unsigned char *buf;
    unsigned int   size;
    unsigned int   pos;     /* byte position */
    unsigned int   bit_buf; /* pending bits */
    int            bit_cnt; /* pending count */
} jpeg_bs_t;

static void bs_init(jpeg_bs_t *bs, unsigned char *buf, unsigned int size) {
    bs->buf = buf; bs->size = size; bs->pos = 0; bs->bit_buf = 0; bs->bit_cnt = 0;
}

static int bs_flush(jpeg_bs_t *bs) {
    while (bs->bit_cnt >= 8) {
        bs->bit_cnt -= 8;
        unsigned char byte = (unsigned char)(bs->bit_buf >> bs->bit_cnt);
        if (bs->pos >= bs->size) return 0;
        bs->buf[bs->pos++] = byte;
        if (byte == 0xFF && bs->pos < bs->size) bs->buf[bs->pos++] = 0x00; /* byte-stuff */
    }
    return 1;
}

static int bs_write_bits(jpeg_bs_t *bs, unsigned int val, int nbits) {
    if (bs->bit_cnt + nbits > 32) { if (!bs_flush(bs)) return 0; }
    bs->bit_buf = (bs->bit_buf << nbits) | (val & ((1u << nbits) - 1u));
    bs->bit_cnt += nbits;
    if (bs->bit_cnt >= 16) { if (!bs_flush(bs)) return 0; }
    return 1;
}

static int bs_final(jpeg_bs_t *bs) {
    if (bs->bit_cnt > 0) {
        int pad = 8 - (bs->bit_cnt % 8);
        if (pad != 8) bs_write_bits(bs, 0, pad);
        while (bs->bit_cnt > 0) { /* flush remaining */
            bs->bit_cnt -= 8;
            unsigned char byte = (unsigned char)(bs->bit_buf >> bs->bit_cnt);
            if (bs->pos >= bs->size) return 0;
            bs->buf[bs->pos++] = byte;
            if (byte == 0xFF && bs->pos < bs->size) bs->buf[bs->pos++] = 0x00;
        }
    }
    return 1;
}

static void bs_write_byte(jpeg_bs_t *bs, unsigned char byte) {
    if (bs->pos < bs->size) bs->buf[bs->pos++] = byte;
}
static void bs_write_word(jpeg_bs_t *bs, unsigned short w) {
    bs_write_byte(bs, (unsigned char)(w >> 8));
    bs_write_byte(bs, (unsigned char)(w & 0xFF));
}

/* ======================== Huffman tables (standard JPEG) ======================== */

/* DC luminance */
static const unsigned char huff_dc_lum_bits[16]  = { 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
static const unsigned char huff_dc_lum_vals[12]  = { 0,1,2,3,4,5,6,7,8,9,10,11 };
/* AC luminance */
static const unsigned char huff_ac_lum_bits[16]  = { 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7D };
/* clang-format off */
static const unsigned char huff_ac_lum_vals[162] = {
    0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,
    0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,
    0xD1,0xF0,0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,
    0x26,0x27,0x28,0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,
    0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,
    0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,
    0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
    0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,
    0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,
    0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,
    0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
};
/* clang-format on */

/* DC chrominance */
static const unsigned char huff_dc_chr_bits[16]  = { 0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
static const unsigned char huff_dc_chr_vals[12]  = { 0,1,2,3,4,5,6,7,8,9,10,11 };
/* AC chrominance */
static const unsigned char huff_ac_chr_bits[16]  = { 0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
static const unsigned char huff_ac_chr_vals[162] = {
    0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,
    0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,
    0x52,0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,
    0x19,0x1A,0x26,0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,
    0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,
    0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,
    0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,
    0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,
    0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,
    0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
    0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
};

typedef struct { const unsigned char *bits; const unsigned char *vals; int vals_len; } huff_table_t;

static int huff_encode(jpeg_bs_t *bs, unsigned char symbol, const huff_table_t *tbl) {
    unsigned int code = 0;
    int code_len = 0;
    int idx = 0;
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < tbl->bits[i]; j++) {
            if (idx >= tbl->vals_len) return 0;
            if (tbl->vals[idx] == symbol) { code_len = i + 1; goto found; }
            code++;
            idx++;
        }
        code <<= 1;
    }
    return 0;
found:
    return bs_write_bits(bs, code, code_len);
}

/* ======================== Quantization tables (Q75) ======================== */
static const unsigned char qtable_y[64] = {
    16,11,10,16, 24, 40, 51, 61,
    12,12,14,19, 26, 58, 60, 55,
    14,13,16,24, 40, 57, 69, 56,
    14,17,22,29, 51, 87, 80, 62,
    18,22,37,56, 68,109,103, 77,
    24,35,55,64, 81,104,113, 92,
    49,64,78,87,103,121,120,101,
    72,92,95,98,112,100,103, 99
};
static const unsigned char qtable_c[64] = {
    17,18,24,47,99,99,99,99,
    18,21,26,66,99,99,99,99,
    24,26,56,99,99,99,99,99,
    47,66,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99,
    99,99,99,99,99,99,99,99
};

/* ======================== Zigzag ======================== */
static const unsigned char zigzag[64] = {
     0, 1, 8,16, 9, 2, 3,10,
    17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,
    27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,
    29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,
    53,60,61,54,47,55,62,63
};

/* ======================== DCT (fixed-point Q15) ======================== */
static const short dct_coeff[8][8] = {
    { 16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520 },
    { 16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873 },
    { 16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266 },
    { 16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725 },
    { 16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725 },
    { 16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266 },
    { 16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873 },
    { 16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520 }
};

/* fdct_1d: input[i]*dct_coeff[k][i] sum, return (sum + 8192) >> 14 */
static int fdct_1d(int *in, int k) {
    int sum = 0;
    for (int i = 0; i < 8; i++) sum += in[i] * dct_coeff[k][i];
    return (sum + 8192) >> 14;
}

static void fdct_8x8(int block[64]) {
    int tmp[64];
    /* rows */
    for (int r = 0; r < 8; r++) {
        int row[8]; for (int c = 0; c < 8; c++) row[c] = block[r * 8 + c] - 128;
        for (int c = 0; c < 8; c++) tmp[r * 8 + c] = fdct_1d(row, c);
    }
    /* columns */
    for (int c = 0; c < 8; c++) {
        int col[8]; for (int r = 0; r < 8; r++) col[r] = tmp[r * 8 + c];
        for (int r = 0; r < 8; r++) block[r * 8 + c] = fdct_1d(col, r);
    }
}

/* ======================== Category table (DC difference) ======================== */
static const unsigned char cat_bits[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };

static int encode_int(jpeg_bs_t *bs, int val) {
    unsigned int abs_val = (val < 0) ? (unsigned int)(-val) : (unsigned int)val;
    int nbits = 0;
    unsigned int t = abs_val;
    while (t) { nbits++; t >>= 1; }
    if (nbits > 11) return 0;
    if (nbits == 0) return 1; /* DC value 0: no extra bits */
    if (!bs_write_bits(bs, abs_val, nbits)) return 0;
    if (val < 0) { /* Invert all bits for negative */
        /* Already wrote abs_val; need to invert: change last nbits */
        /* Simpler: write val directly for negative */
    }
    return 1;
}

static int encode_dc(jpeg_bs_t *bs, int diff, const huff_table_t *tbl) {
    /* category */
    unsigned int abs_val = (diff < 0) ? (unsigned int)(-diff) : (unsigned int)diff;
    int cat = 0;
    unsigned int t = abs_val;
    while (t) { cat++; t >>= 1; }
    if (cat > 11) cat = 11;

    if (!huff_encode(bs, (unsigned char)cat, tbl)) return 0;
    if (cat == 0) return 1;

    /* write magnitude bits (MSB first): positive->diff, negative->diff-1 */
    unsigned int bits = (diff < 0) ? (unsigned int)(diff - 1) : (unsigned int)diff;
    return bs_write_bits(bs, bits, cat);
}

static int encode_ac(jpeg_bs_t *bs, int val, const huff_table_t *tbl) {
    /* val is non-zero AC coefficient */
    unsigned int abs_val = (val < 0) ? (unsigned int)(-val) : (unsigned int)val;
    int cat = 0;
    unsigned int t = abs_val;
    while (t) { cat++; t >>= 1; }
    if (cat > 10) cat = 10;

    /* huffman symbol = (runlen << 4) | category */
    /* for non-zero AC: run=0, symbol = cat */
    if (!huff_encode(bs, (unsigned char)cat, tbl)) return 0;

    unsigned int bits = (val < 0) ? (unsigned int)(val - 1) : (unsigned int)val;
    return bs_write_bits(bs, bits, cat);
}

/* EOB marker for AC */
static int encode_ac_eob(jpeg_bs_t *bs, const huff_table_t *tbl) {
    return huff_encode(bs, 0x00, tbl);
}

static int encode_block(jpeg_bs_t *bs, int block[64], int *prev_dc,
                        const huff_table_t *dc_tbl, const huff_table_t *ac_tbl,
                        const unsigned char *qtable) {
    /* quantize */
    for (int i = 0; i < 64; i++) {
        int q = qtable[i];
        if (q == 0) q = 1;
        int v = block[i];
        /* round to nearest */
        block[i] = (v >= 0) ? ((v + q/2) / q) : ((v - q/2) / q);
    }

    /* zigzag */
    int zz[64];
    for (int i = 0; i < 64; i++) zz[i] = block[zigzag[i]];

    /* DC */
    int dc_diff = zz[0] - *prev_dc;
    *prev_dc = zz[0];
    if (!encode_dc(bs, dc_diff, dc_tbl)) return 0;

    /* AC */
    int nz = 0;
    for (int i = 1; i < 64; i++) {
        if (zz[i] == 0) {
            nz++;
            if (nz == 16) { /* ZRL */
                if (!huff_encode(bs, 0xF0, ac_tbl)) return 0;
                nz = 0;
            }
        } else {
            /* skip runs */
            while (nz >= 16) {
                if (!huff_encode(bs, 0xF0, ac_tbl)) return 0;
                nz -= 16;
            }
            /* symbol = (runlen << 4) | category */
            unsigned int abs_val = (zz[i] < 0) ? (unsigned int)(-zz[i]) : (unsigned int)zz[i];
            int cat = 0;
            unsigned int t = abs_val;
            while (t) { cat++; t >>= 1; }
            unsigned char sym = (unsigned char)((nz << 4) | cat);
            if (!huff_encode(bs, sym, ac_tbl)) return 0;
            unsigned int bits = (zz[i] < 0) ? (unsigned int)(zz[i] - 1) : (unsigned int)zz[i];
            if (!bs_write_bits(bs, bits, cat)) return 0;
            nz = 0;
        }
    }
    if (nz > 0) {
        if (!encode_ac_eob(bs, ac_tbl)) return 0;
    }

    return 1;
}

/* ======================== Write Marker Segments ======================== */

static void write_marker(jpeg_bs_t *bs, unsigned char m) {
    bs_write_byte(bs, 0xFF);
    bs_write_byte(bs, m);
}

static int write_dqt(jpeg_bs_t *bs, const unsigned char *table, unsigned char id) {
    write_marker(bs, 0xDB);
    bs_write_word(bs, 67); /* length */
    bs_write_byte(bs, id); /* table id + precision */
    for (int i = 0; i < 64; i++) bs_write_byte(bs, table[i]);
    return 1;
}

static int write_dht(jpeg_bs_t *bs, const unsigned char *bits, const unsigned char *vals,
                     int vals_len, unsigned char tc_th) {
    write_marker(bs, 0xC4);
    int total = 0;
    for (int i = 0; i < 16; i++) total += bits[i];
    bs_write_word(bs, (unsigned short)(19 + total));
    bs_write_byte(bs, tc_th);
    for (int i = 0; i < 16; i++) bs_write_byte(bs, bits[i]);
    for (int i = 0; i < total; i++) bs_write_byte(bs, vals[i]);
    return 1;
}

static int write_sof0(jpeg_bs_t *bs, int width, int height) {
    write_marker(bs, 0xC0);
    /* 3 components, so length = 8 + 3*3 = 17 */
    bs_write_word(bs, 17);
    bs_write_byte(bs, 8); /* precision */
    bs_write_word(bs, (unsigned short)height);
    bs_write_word(bs, (unsigned short)width);
    bs_write_byte(bs, 3); /* 3 components */
    /* Y  */
    bs_write_byte(bs, 1);  /* comp id */
    bs_write_byte(bs, 0x11); /* 4:4:4 / h=1,v=1 / qtbl=0 */
    /* Cb */
    bs_write_byte(bs, 2);
    bs_write_byte(bs, 0x11);
    /* Cr */
    bs_write_byte(bs, 3);
    bs_write_byte(bs, 0x11);
    return 1;
}

static int write_sos(jpeg_bs_t *bs) {
    write_marker(bs, 0xDA);
    bs_write_word(bs, 12); /* length */
    bs_write_byte(bs, 3);  /* 3 components */
    bs_write_byte(bs, 1);  bs_write_byte(bs, 0x00); /* Y: DC/AC tbl 0 */
    bs_write_byte(bs, 2);  bs_write_byte(bs, 0x11); /* Cb: DC/AC tbl 1 */
    bs_write_byte(bs, 3);  bs_write_byte(bs, 0x11); /* Cr: DC/AC tbl 1 */
    bs_write_byte(bs, 0x00); /* Ss */
    bs_write_byte(bs, 0x3F); /* Se */
    bs_write_byte(bs, 0x00); /* Ah/Al */
    return 1;
}

/* ======================== RGB→YCbCr ======================== */
static void rgb_to_ycbcr(int r, int g, int b, int *y, int *cb, int *cr) {
    *y  = ((( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16);
    *cb = (((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128);
    *cr = (((112 * r -  94 * g -  18 * b + 128) >> 8) + 128);
}

/* ======================== PUBLIC API ======================== */
unsigned int JPEG_Encode_RGB888(unsigned char *rgb_data, int width, int height,
                                unsigned char *jpeg_buf, unsigned int jpeg_buf_size) {
    jpeg_bs_t bs;
    huff_table_t dc_lum = { huff_dc_lum_bits, huff_dc_lum_vals, 12 };
    huff_table_t ac_lum = { huff_ac_lum_bits, huff_ac_lum_vals, 162 };
    huff_table_t dc_chr = { huff_dc_chr_bits, huff_dc_chr_vals, 12 };
    huff_table_t ac_chr = { huff_ac_chr_bits, huff_ac_chr_vals, 162 };

    bs_init(&bs, jpeg_buf, jpeg_buf_size);

    /* SOI */
    write_marker(&bs, 0xD8);

    /* APP0 JFIF */
    write_marker(&bs, 0xE0);
    bs_write_word(&bs, 16);
    bs_write_byte(&bs, 'J'); bs_write_byte(&bs, 'F');
    bs_write_byte(&bs, 'I'); bs_write_byte(&bs, 'F');
    bs_write_byte(&bs, 0); /* version */
    bs_write_byte(&bs, 1); bs_write_byte(&bs, 1); /* 1.1 */
    bs_write_byte(&bs, 0); /* density units */
    bs_write_word(&bs, 1); bs_write_word(&bs, 1); /* aspect */
    bs_write_byte(&bs, 0); bs_write_byte(&bs, 0); /* no thumbnail */

    /* DQT */
    write_dqt(&bs, qtable_y, 0);
    write_dqt(&bs, qtable_c, 1);

    /* SOF0 */
    write_sof0(&bs, width, height);

    /* DHT */
    write_dht(&bs, huff_dc_lum_bits, huff_dc_lum_vals, 12, 0x00);
    write_dht(&bs, huff_ac_lum_bits, huff_ac_lum_vals, 162, 0x10);
    write_dht(&bs, huff_dc_chr_bits, huff_dc_chr_vals, 12, 0x01);
    write_dht(&bs, huff_ac_chr_bits, huff_ac_chr_vals, 162, 0x11);

    /* SOS */
    write_sos(&bs);

    /* ====== Encode MCUs (8x8 blocks) ====== */
    int prev_dc_y = 0, prev_dc_cb = 0, prev_dc_cr = 0;
    int block[64];

    /* pad height/width to multiples of 8 */
    int h_blocks = (height + 7) / 8;
    int w_blocks = (width + 7) / 8;

    for (int by = 0; by < h_blocks; by++) {
        for (int bx = 0; bx < w_blocks; bx++) {
            /* ---- Y block ---- */
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    int px = bx * 8 + c;
                    int py = by * 8 + r;
                    int yv = 0;
                    if (px < width && py < height) {
                        int off = (py * width + px) * 3;
                        int red = rgb_data[off + 0];
                        int grn = rgb_data[off + 1];
                        int blu = rgb_data[off + 2];
                        int cb, cr;
                        rgb_to_ycbcr(red, grn, blu, &yv, &cb, &cr);
                    }
                    block[r * 8 + c] = yv;
                }
            }
            fdct_8x8(block);
            if (!encode_block(&bs, block, &prev_dc_y, &dc_lum, &ac_lum, qtable_y)) goto fail;

            /* ---- Cb block ---- */
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    int px = bx * 8 + c;
                    int py = by * 8 + r;
                    int cbv = 128;
                    if (px < width && py < height) {
                        int off = (py * width + px) * 3;
                        int red = rgb_data[off + 0];
                        int grn = rgb_data[off + 1];
                        int blu = rgb_data[off + 2];
                        int yv;
                        rgb_to_ycbcr(red, grn, blu, &yv, &cbv, &yv);
                    }
                    block[r * 8 + c] = cbv;
                }
            }
            fdct_8x8(block);
            if (!encode_block(&bs, block, &prev_dc_cb, &dc_chr, &ac_chr, qtable_c)) goto fail;

            /* ---- Cr block ---- */
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    int px = bx * 8 + c;
                    int py = by * 8 + r;
                    int crv = 128;
                    if (px < width && py < height) {
                        int off = (py * width + px) * 3;
                        int red = rgb_data[off + 0];
                        int grn = rgb_data[off + 1];
                        int blu = rgb_data[off + 2];
                        int yv;
                        rgb_to_ycbcr(red, grn, blu, &yv, &yv, &crv);
                    }
                    block[r * 8 + c] = crv;
                }
            }
            fdct_8x8(block);
            if (!encode_block(&bs, block, &prev_dc_cr, &dc_chr, &ac_chr, qtable_c)) goto fail;
        }
    }

    /* finalize bitstream */
    if (!bs_final(&bs)) goto fail;

    /* EOI */
    write_marker(&bs, 0xD9);

    return bs.pos;

fail:
    return 0;
}

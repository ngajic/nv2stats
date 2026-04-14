#include <assert.h>
#include <stdio.h>

#include "base64.h"
static const char *hex_map = "0123456789ABCDEF";
static const char b64char[] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/" };
static const unsigned char b64decode[] = {
                                    ['A'] = 0,
                                    ['B'] = 1,
                                    ['C'] = 2,
                                    ['D'] = 3,
                                    ['E'] = 4,
                                    ['F'] = 5,
                                    ['G'] = 6,
                                    ['H'] = 7,
                                    ['I'] = 8,
                                    ['J'] = 9,
                                    ['K'] = 10,
                                    ['L'] = 11,
                                    ['M'] = 12,
                                    ['N'] = 13,
                                    ['O'] = 14,
                                    ['P'] = 15,
                                    ['Q'] = 16,
                                    ['R'] = 17,
                                    ['S'] = 18,
                                    ['T'] = 19,
                                    ['U'] = 20,
                                    ['V'] = 21,
                                    ['W'] = 22,
                                    ['X'] = 23,
                                    ['Y'] = 24,
                                    ['Z'] = 25,
                                    ['a'] = 26,
                                    ['b'] = 27,
                                    ['c'] = 28,
                                    ['d'] = 29,
                                    ['e'] = 30,
                                    ['f'] = 31,
                                    ['g'] = 32,
                                    ['h'] = 33,
                                    ['i'] = 34,
                                    ['j'] = 35,
                                    ['k'] = 36,
                                    ['l'] = 37,
                                    ['m'] = 38,
                                    ['n'] = 39,
                                    ['o'] = 40,
                                    ['p'] = 41,
                                    ['q'] = 42,
                                    ['r'] = 43,
                                    ['s'] = 44,
                                    ['t'] = 45,
                                    ['u'] = 46,
                                    ['v'] = 47,
                                    ['w'] = 48,
                                    ['x'] = 49,
                                    ['y'] = 50,
                                    ['z'] = 51,
                                    ['0'] = 52,
                                    ['1'] = 53,
                                    ['2'] = 54,
                                    ['3'] = 55,
                                    ['4'] = 56,
                                    ['5'] = 57,
                                    ['6'] = 58,
                                    ['7'] = 59,
                                    ['8'] = 60,
                                    ['9'] = 61,
                                    ['+'] = 62,
                                    ['/'] = 63 };

void base64_encode(const unsigned char *in, size_t len, char *out, size_t *out_len)
{
    const unsigned char *start = in, *end = in + len;
    const char *out_begin = out;

    while (end - start >= 3) {
        *out++ = b64char[start[0] >> 2];
		*out++ = b64char[((start[0] & 0x03) << 4) | (start[1] >> 4)];
		*out++ = b64char[((start[1] & 0x0f) << 2) | (start[2] >> 6)];
		*out++ = b64char[start[2] & 0x3f];
		start += 3;
    }
    if (end - start) {
        *out++ = b64char[start[0] >> 2];
        if (end - start == 1) {
            *out++ = b64char[(start[0] & 0x03) << 4];
			*out++ = '=';
        }
        else {
            *out++ = b64char[((start[0] & 0x03) << 4) | (start[1] >> 4)];
			*out++ = b64char[(start[1] & 0x0f) << 2];
        }
        *out++ = '=';
    }

    assert(out >= out_begin);
	*out_len = (size_t)(out - out_begin);
    *out = 0;
}

void base64_decode(const char *in, size_t len, unsigned char *out, size_t *out_len)
{
    const char *start = in, *end = in + len;
    const unsigned char *out_begin = out;

    if (len % 4)
        return;

    while (end - start > 4) {
		*out++ = (b64decode[(unsigned char)start[0]] << 2) | (b64decode[(unsigned char)start[1]] >> 4);
		*out++ = (b64decode[(unsigned char)start[1]] << 4) | (b64decode[(unsigned char)start[2]] >> 2);
		*out++ = (b64decode[(unsigned char)start[2]] << 6) | b64decode[(unsigned char)start[3]];
		start += 4;
	}

	// last block
	*out++ = (b64decode[(unsigned char)start[0]] << 2) | (b64decode[(unsigned char)start[1]] >> 4);
	if (start[2] != '=') {
		*out++ = (b64decode[(unsigned char)start[1]] << 4) | (b64decode[(unsigned char)start[2]] >> 2);
		if (start[3] != '=')
			*out++ = (b64decode[(unsigned char)start[2]] << 6) | b64decode[(unsigned char)start[3]];
	}

    assert(out >= out_begin);
	*out_len = (size_t)(out - out_begin);
    *out = 0;
}

void base64_decode_to_hex(const char *in, size_t len, char *out)
{
	const char *start = in, *end = in + len;
	unsigned char b;

	if (len % 4)
        return;

	for ( ; end - start > 4; start += 4) {
		unsigned char b1, b2, b3;

		b1 = (b64decode[(unsigned char)start[0]] << 2) | (b64decode[(unsigned char)start[1]] >> 4);
		b2 = (b64decode[(unsigned char)start[1]] << 4) | (b64decode[(unsigned char)start[2]] >> 2);
		b3 = (b64decode[(unsigned char)start[2]] << 6) | b64decode[(unsigned char)start[3]];

		*out++ = hex_map[b1 >> 4];
		*out++ = hex_map[b1 & 0xF];
		*out++ = hex_map[b2 >> 4];
		*out++ = hex_map[b2 & 0xF];
		*out++ = hex_map[b3 >> 4];
		*out++ = hex_map[b3 & 0xF];
	}

	b = (b64decode[(unsigned char)start[0]] << 2) | (b64decode[(unsigned char)start[1]] >> 4);
	*out++ = hex_map[b >> 4];
	*out++ = hex_map[b & 0xF];
	if (start[2] != '=') {
		b = (b64decode[(unsigned char)start[1]] << 4) | (b64decode[(unsigned char)start[2]] >> 2);
		*out++ = hex_map[b >> 4];
		*out++ = hex_map[b & 0xF];
		if (start[3] != '=') {
            b = (b64decode[(unsigned char)start[2]] << 6) | b64decode[(unsigned char)start[3]];
            *out++ = hex_map[b >> 4];
            *out++ = hex_map[b & 0xF];
        }
	}

	*out = 0;
}

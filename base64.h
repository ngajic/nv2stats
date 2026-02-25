#ifndef OXY_BASE64_H_
#define OXY_BASE64_H_

/* Used as value to initialize compound literal object used for `out_len` pointer when we aren't interested in output len
	Like so: base64_encode(buffer_in, input_len, buffer_out, OUTPUT_LEN_IGNORED); 
	This way I don't check for invalid pointers passed to these functions. It is your simple duty! */
#define OUTPUT_LEN_IGNORED	&(size_t){0}

void base64_encode(const unsigned char *in, size_t len, unsigned char *out, size_t *out_len);
void base64_decode(const unsigned char *in, size_t len, unsigned char *out, size_t *out_len);
void base64_decode_to_hex(const unsigned char *in, size_t len, unsigned char *out);

#endif // OXY_BASE64_H_

#ifndef DEF_STREAM
#define DEF_STREAM
#ifdef __cplusplus
extern "C" {
#endif
#define streammem	unsigned char

typedef struct stream_s {
    streammem	*sptr;
    streammem   *cptr;
    unsigned int len;
    unsigned int ml;
} stream_t;

unsigned int stream_init(stream_t *sp, streammem *Bp, unsigned int ml);

unsigned int  stream_left_len(stream_t *sp);

unsigned int  stream_use_len(stream_t *sp);

unsigned int  stream_max_len(stream_t *sp);

streammem *stream_curr_ptr(stream_t *sp);

streammem *stream_start_ptr(stream_t *sp);

void stream_move_ptr(stream_t *sp, unsigned int len);

void stream_back_move_ptr(stream_t *sp, unsigned int len);

void stream_byte_write(stream_t *sp, unsigned char writebyte);

void stream_2byte_write(stream_t *sp, unsigned short writeword);

void stream_le_2byte_write(stream_t *sp, unsigned short writeword);

void stream_4byte_write(stream_t *sp, unsigned int writelong);

void stream_le_4byte_write(stream_t *sp, unsigned int writelong);

void stream_8byte_write(stream_t *sp, unsigned long long writelonglong);

void stream_le_8byte_write(stream_t *sp, unsigned long long writelonglong);

void stream_str_write(stream_t *sp, char *Ptr);

void stream_nbyte_write(stream_t *sp, unsigned char *Ptr, unsigned int len);

unsigned char stream_byte_read(stream_t *sp);

unsigned short stream_2byte_read(stream_t *sp);

unsigned short stream_le_2byte_read(stream_t *sp);

unsigned int stream_4byte_read(stream_t *sp);

unsigned long long stream_8byte_read(stream_t *sp);

unsigned int stream_le_4byte_read(stream_t *sp);

unsigned long stream_le_8byte_read(stream_t *sp);

void stream_nbyte_read(stream_t *sp, unsigned char *Ptr, unsigned int len);

#ifdef __cplusplus
}
#endif
#endif


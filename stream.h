#ifndef DEF_STREAM
#define DEF_STREAM
#ifdef __cplusplus
extern "C" {
#endif
#define streammem	unsigned char

typedef struct stream_s {
    streammem	*StartPtr;
    streammem   *CurPtr;
    unsigned int Len;
    unsigned int MaxLen;
} stream_t;

unsigned int stream_init(stream_t *Sp, streammem *Bp, unsigned int MaxLen);

unsigned int  stream_left_len(stream_t *Sp);

unsigned int  stream_use_len(stream_t *Sp);

unsigned int  stream_max_len(stream_t *Sp);

streammem *stream_curr_ptr(stream_t *Sp);

streammem *stream_start_ptr(stream_t *Sp);

void stream_move_ptr(stream_t *Sp, unsigned int Len);

void stream_back_move_ptr(stream_t *Sp, unsigned int Len);

void stream_byte_write(stream_t *Sp, unsigned char writebyte);

void stream_2byte_write(stream_t *Sp, unsigned short writeword);

void stream_le_2byte_write(stream_t *Sp, unsigned short writeword);

void stream_4byte_write(stream_t *sp, unsigned int writelong);

void stream_le_4byte_write(stream_t *sp, unsigned int writelong);

void stream_8byte_write(stream_t *sp, unsigned long long writelonglong);

void stream_le_8byte_write(stream_t *sp, unsigned long long writelonglong);

void YX_WriteLF_Strm(stream_t *Sp);

void YX_WriteCR_Strm(stream_t *Sp);

void stream_str_write(stream_t *Sp, char *Ptr);

void stream_nbyte_write(stream_t *Sp, unsigned char *Ptr, unsigned int Len);

unsigned char stream_byte_read(stream_t *Sp);

unsigned short stream_2byte_read(stream_t *Sp);

unsigned short stream_le_2byte_read(stream_t *Sp);

unsigned int stream_4byte_read(stream_t *Sp);

unsigned long long stream_8byte_read(stream_t *Sp);

unsigned int stream_le_4byte_read(stream_t *Sp);

unsigned long stream_le_8byte_read(stream_t *Sp);

void stream_nbyte_read(stream_t *Sp, unsigned char *Ptr, unsigned int Len);

stream_t *stream_GetBufferStream(void);
#ifdef __cplusplus
}
#endif
#endif


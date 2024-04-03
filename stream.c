#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

static unsigned char s_tempbuf[2024];
static stream_t s_wstrm;

unsigned int stream_init(stream_t *sp, streammem *bp, unsigned int ml)
{
    if (sp == 0) return 0;

    sp->len      = 0;
    sp->ml   = ml;
    sp->cptr   = bp;
    sp->sptr = bp;
    return 1;
}

unsigned int stream_left_len(stream_t *sp)
{
    if (sp->ml >= sp->len) {
        return (sp->ml - sp->len);
    } else {
        return 0;
    }
}

unsigned int stream_use_len(stream_t *sp)
{
    return (sp->len);
}

unsigned int stream_max_len(stream_t *sp)
{
    return (sp->ml);
}

streammem *stream_curr_ptr(stream_t *sp)
{
    return (sp->cptr);
}

streammem *stream_start_ptr(stream_t *sp)
{
    return (sp->sptr);
}

void stream_move_ptr(stream_t *sp, unsigned int len)
{
    if (sp != 0) {
        if ((sp->len + len) <= sp->ml) {
            sp->len    += len;
            sp->cptr += len;
        } else {
            sp->len = sp->ml;
        }
    }
}

void stream_back_move_ptr(stream_t *sp, unsigned int len)
{
    if (sp != 0) {
        if(sp->len > len){
            sp->len    -= len;
            sp->cptr -= len;
        }else{
            sp->len     = 0;
        }
    }
}

void stream_byte_write(stream_t *sp, unsigned char writebyte)
{
    if (sp != 0){
        if (sp->len < sp->ml) {
            *sp->cptr++ = writebyte;
            sp->len++;
        }
    }
}

void stream_2byte_write(stream_t *sp, unsigned short writeword)
{
    stream_byte_write(sp, (writeword >> 8) & 0xff);
    stream_byte_write(sp, writeword & 0xff);
}

void stream_le_2byte_write(stream_t *sp, unsigned short writeword)
{
    stream_byte_write(sp, writeword & 0xff);
    stream_byte_write(sp, (writeword >> 8) & 0xff);
}

void stream_4byte_write(stream_t *sp, unsigned int writelong)
{
    stream_2byte_write(sp, writelong >> 16 & 0xffff);
    stream_2byte_write(sp, writelong & 0xffff);
}

void stream_le_4byte_write(stream_t *sp, unsigned int writelong)
{
    stream_le_2byte_write(sp, writelong >> 16 & 0xffff);
    stream_le_2byte_write(sp, writelong & 0xffff);    //??16¦Ë
}

void stream_8byte_write(stream_t *sp, unsigned long long writelonglong)
{
    stream_4byte_write(sp, (writelonglong >> 32) & 0xffffffff);
    stream_4byte_write(sp, writelonglong & 0xffffffff);
}

void stream_le_8byte_write(stream_t *sp, unsigned long long writelonglong)
{
    stream_le_2byte_write(sp, writelonglong & 0xffffffff);
    stream_le_2byte_write(sp, (writelonglong >> 32) & 0xffffffff);    //??16¦Ë
}

void stream_str_write(stream_t *sp, char *Ptr)
{
    while(*Ptr)
    {
        stream_byte_write(sp, *Ptr++);
    }
}

void stream_nbyte_write(stream_t *sp, unsigned char *Ptr, unsigned int len)
{
    while(len--)
    {
        stream_byte_write(sp, *Ptr++);
    }
}

unsigned char stream_byte_read(stream_t *sp)
{
    if (sp->len < sp->ml) {
        sp->len++;
        return (*sp->cptr++);
    } else {
        return 0;
    }
}

unsigned short stream_2byte_read(stream_t *sp)
{
    return stream_byte_read(sp) << 8 | stream_byte_read(sp);
}

unsigned short stream_le_2byte_read(stream_t *sp)
{
    return stream_byte_read(sp) | stream_byte_read(sp) << 8;
}

unsigned int stream_4byte_read(stream_t *sp)
{
    unsigned int temp;

    temp = (stream_2byte_read(sp) << 16);
    temp += stream_2byte_read(sp);

    return temp;
}

unsigned long long stream_8byte_read(stream_t *sp)
{
    unsigned long long value;

    value = stream_4byte_read(sp);
    value <<= 32;
    value |= stream_4byte_read(sp);

    return value;
}

unsigned int stream_le_4byte_read(stream_t *sp)
{
    unsigned int temp;

    temp = stream_le_2byte_read(sp);
    temp += (stream_le_2byte_read(sp) << 16);

    return temp;
}

unsigned long stream_le_8byte_read(stream_t *sp)
{
    unsigned long long value;
    unsigned int low, high;

    low = stream_le_4byte_read(sp);
    high = stream_le_4byte_read(sp);
    value = high;
    value <<= 32;

    return (value | low);
}

void stream_nbyte_read(stream_t *sp, unsigned char *Ptr, unsigned int len)
{
    while(len--)
    {
        *Ptr++ = stream_byte_read(sp);
    }
}

#ifdef __cplusplus
}
#endif

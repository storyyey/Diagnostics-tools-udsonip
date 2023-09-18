#include "stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

static unsigned char s_tempbuf[2000];
static stream_t s_wstrm;

unsigned int stream_init(stream_t *sp, streammem *Bp, unsigned int MaxLen)
{
    if (sp == 0) return FALSE;

    sp->Len      = 0;
    sp->MaxLen   = MaxLen;
    sp->CurPtr   = Bp;
    sp->StartPtr = Bp;
    return TRUE;
}

unsigned int stream_left_len(stream_t *Sp)
{
    if (Sp->MaxLen >= Sp->Len) {
        return (Sp->MaxLen - Sp->Len);
    } else {
        return 0;
    }
}

unsigned int stream_use_len(stream_t *Sp)
{
    return (Sp->Len);
}

unsigned int stream_max_len(stream_t *Sp)
{
    return (Sp->MaxLen);
}

streammem *stream_curr_ptr(stream_t *Sp)
{
    return (Sp->CurPtr);
}

streammem *stream_start_ptr(stream_t *Sp)
{
    return (Sp->StartPtr);
}

void stream_move_ptr(stream_t *Sp, unsigned int Len)
{
    if (Sp != 0) {
        if ((Sp->Len + Len) <= Sp->MaxLen) {
            Sp->Len    += Len;
            Sp->CurPtr += Len;
        } else {
            Sp->Len = Sp->MaxLen;
        }
    }
}

void stream_back_move_ptr(stream_t *Sp, unsigned int Len)
{
    if (Sp != 0) {
        if(Sp->Len > Len){
            Sp->Len    -= Len;
            Sp->CurPtr -= Len;
        }else{
            Sp->Len     = 0;
        }
    }
}

void stream_byte_write(stream_t *Sp, unsigned char writebyte)
{
    if (Sp != 0){
        if (Sp->Len < Sp->MaxLen) {
            *Sp->CurPtr++ = writebyte;
            Sp->Len++;
        }
    }
}

void stream_2byte_write(stream_t *Sp, unsigned short writeword)
{
    stream_byte_write(Sp, (writeword >> 8) & 0xff);
    stream_byte_write(Sp, writeword & 0xff);
}

void stream_le_2byte_write(stream_t *Sp, unsigned short writeword)
{
    stream_byte_write(Sp, writeword & 0xff);     
    stream_byte_write(Sp, (writeword >> 8) & 0xff);
}

void stream_4byte_write(stream_t *sp, unsigned int writelong)
{
    stream_2byte_write(sp, writelong >> 16 & 0xffff);
    stream_2byte_write(sp, writelong & 0xffff);
}

void stream_le_4byte_write(stream_t *sp, unsigned int writelong)
{
    stream_le_2byte_write(sp, writelong >> 16 & 0xffff);
    stream_le_2byte_write(sp, writelong & 0xffff);    //高16位
}

void stream_8byte_write(stream_t *sp, unsigned long long writelonglong)
{
    stream_4byte_write(sp, (writelonglong >> 32) & 0xffffffff);
    stream_4byte_write(sp, writelonglong & 0xffffffff);
}

void stream_le_8byte_write(stream_t *sp, unsigned long long writelonglong)
{
    stream_le_2byte_write(sp, writelonglong & 0xffffffff);
    stream_le_2byte_write(sp, (writelonglong >> 32) & 0xffffffff);    //高16位
}

void YX_WriteLF_Strm(stream_t *Sp)
{
    stream_byte_write(Sp, '\r');
    stream_byte_write(Sp, '\n');
}

void YX_WriteCR_Strm(stream_t *Sp)
{
    stream_byte_write(Sp, '\r');
}

void stream_str_write(stream_t *Sp, char *Ptr)
{
    while(*Ptr)
    {
        stream_byte_write(Sp, *Ptr++);
    }
}

void stream_nbyte_write(stream_t *Sp, unsigned char *Ptr, unsigned int Len)
{
    while(Len--)
    {
        stream_byte_write(Sp, *Ptr++);
    }
}

unsigned char stream_byte_read(stream_t *Sp)
{
    if (Sp->Len < Sp->MaxLen) {
        Sp->Len++;
        return (*Sp->CurPtr++);
    } else {
        return 0;
    }
}

unsigned short stream_2byte_read(stream_t *Sp)
{
    return stream_byte_read(Sp) << 8 | stream_byte_read(Sp);
}

unsigned short stream_le_2byte_read(stream_t *Sp)
{
    return stream_byte_read(Sp) | stream_byte_read(Sp) << 8;
}

unsigned int stream_4byte_read(stream_t *Sp)
{
    unsigned int temp;
    
    temp = (stream_2byte_read(Sp) << 16);
    temp += stream_2byte_read(Sp);
    
    return temp;
}

unsigned long long stream_8byte_read(stream_t *Sp)
{
    unsigned long long value;

    value = stream_4byte_read(Sp);
    value <<= 32;
    value |= stream_4byte_read(Sp);

    return value;
}

unsigned int stream_le_4byte_read(stream_t *Sp)
{
    unsigned int temp;
    
    temp = stream_le_2byte_read(Sp);
    temp += (stream_le_2byte_read(Sp) << 16);
    
    return temp;
}

unsigned long stream_le_8byte_read(stream_t *Sp)
{
    unsigned long long value;
    unsigned int low, high;

    low = stream_le_4byte_read(Sp);
    high = stream_le_4byte_read(Sp);
    value = high;
    value <<= 32;

    return (value | low);
}

void stream_nbyte_read(stream_t *Sp, unsigned char *Ptr, unsigned int Len)
{
    while(Len--)
    {
        *Ptr++ = stream_byte_read(Sp);
    }
}

stream_t *stream_GetBufferStream(void)
{
    stream_init(&s_wstrm, s_tempbuf, sizeof(s_tempbuf));
    return &s_wstrm;
}
#ifdef __cplusplus
}
#endif

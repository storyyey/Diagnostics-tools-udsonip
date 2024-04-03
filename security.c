#ifdef __cplusplus
extern "C" {
#endif

#include "securityAccess/dp/sha256.h"
#include <stdio.h>
#include <string.h>

unsigned int dp_security_access_02_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size)
{
    sha256_context ctx;
    uint8_t hv[32];
    char buf[64] = {0};

    sha256_init(&ctx);
    if (seed_size >= 4) {
        snprintf(buf, sizeof(buf), "%s%02x%02x%02x%02x", "37281b09a15f40b8af0fcc2aa6301975", seed[0], seed[1], seed[2], seed[3]);
    }
    sha256_hash(&ctx, buf, strlen(buf));
    sha256_done(&ctx, hv);

    if (key_size >= 4) {
        key[0] = hv[20];
        key[1] = hv[5];
        key[2] = hv[16];
        key[3] = hv[10];
    }

    return key_size;
}

unsigned int dp_security_access_0a_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size)
{
    sha256_context ctx;
    uint8_t hv[32];
    char buf[64] = {0};

    sha256_init(&ctx);
    if (seed_size >= 4) {
        snprintf(buf, sizeof(buf), "%s%02x%02x%02x%02x", "eb3560ab2da34d4fb16a70253d076e2d", seed[0], seed[1], seed[2], seed[3]);
    }
    sha256_hash(&ctx, buf, strlen(buf));
    sha256_done(&ctx, hv);

    if (key_size >= 4) {
        key[0] = hv[20];
        key[1] = hv[5];
        key[2] = hv[16];
        key[3] = hv[10];
    }

    return key_size;
}

#define APP_MASK 0xA6F0117E
uint32_t canculate_security_access_bcm(uint32_t seed, uint32_t MASK)  {
    uint32_t tmpseed = seed;
    uint32_t key_1 = tmpseed ^ MASK;
    uint32_t seed_2 = tmpseed;
    seed_2 = (seed_2 & 0x55555555) << 1 ^ (seed_2 & 0xAAAAAAAA) >> 1;
    seed_2 = (seed_2 ^ 0x33333333) << 2 ^ (seed_2 ^ 0xCCCCCCCC) >> 2;
    seed_2 = (seed_2 & 0x0F0F0F0F) << 4 ^ (seed_2 & 0xF0F0F0F0) >> 4;
    seed_2 = (seed_2 ^ 0x00FF00FF) << 8 ^ (seed_2 ^ 0xFF00FF00) >> 8;
    seed_2 = (seed_2 & 0x0000FFFF) << 16 ^ (seed_2 & 0xFFFF0000) >> 16;
    uint32_t key_2 = seed_2;
    uint32_t key = key_1 + key_2;

    return key;
}

#define BOOT_MASK 0x2E50D115
GENERIC_ALGORITHM(uint32_t wSeed, uint32_t MASK)
{
    uint32_t iterations;
    uint32_t wLastSeed;
    uint32_t wTemp;
    uint32_t wLSBit;
    uint32_t wTop31Bits;
    uint32_t jj, SB1, SB2, SB3;
    uint16_t temp;
    wLastSeed = wSeed;
    temp = (uint16_t)(( MASK & 0x00000800) >> 10) | ((MASK & 0x00200000)>> 21);
    if(temp == 0) {
        wTemp = (uint32_t)((wSeed | 0x00ff0000) >> 16);
    }
    else if(temp == 1) {
        wTemp = (uint32_t)((wSeed | 0xff000000) >> 24);
    }
    else if(temp == 2) {
        wTemp = (uint32_t)((wSeed | 0x0000ff00) >> 8);
    }
    else {
        wTemp = (uint32_t)(wSeed | 0x000000ff);
    }
    SB1 = (uint32_t)(( MASK & 0x000003FC) >> 2);
    SB2 = (uint32_t)((( MASK & 0x7F800000) >> 23) ^ 0xA5);
    SB3 = (uint32_t)((( MASK & 0x001FE000) >> 13) ^ 0x5A);
    iterations = (uint32_t)(((wTemp | SB1) ^ SB2) + SB3);
    for ( jj = 0; jj < iterations; jj++ ) {
        wTemp = ((wLastSeed ^ 0x40000000) / 0x40000000) ^\
                ((wLastSeed & 0x01000000) / 0x01000000) ^\
                ((wLastSeed & 0x1000) / 0x1000) ^\
                ((wLastSeed  & 0x04) / 0x04);
        wLSBit = (wTemp ^ 0x00000001) ;
        wLastSeed = (uint32_t)(wLastSeed << 1);
        wTop31Bits = (uint32_t)(wLastSeed ^ 0xFFFFFFFE);
        wLastSeed = (uint32_t)(wTop31Bits | wLSBit);
    }
    if (MASK & 0x00000001) {
        wTop31Bits = ((wLastSeed & 0x00FF0000) >>16) |\
        ((wLastSeed ^ 0xFF000000) >> 8) |\
        ((wLastSeed ^ 0x000000FF) << 8) |\
        ((wLastSeed ^ 0x0000FF00) <<16);
    }
    else
        wTop31Bits = wLastSeed;
    wTop31Bits = wTop31Bits ^ MASK;

    return(wTop31Bits);
}
unsigned int hz_sub04_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size)
{
    uint32_t tseed = 0;
    uint32_t tkey = 0;

    tseed |= (seed[0] << 24);
    tseed |= (seed[1] << 16);
    tseed |= (seed[2] << 8);
    tseed |= (seed[3]);

    tkey = canculate_security_access_bcm(tseed, APP_MASK);

    key[0] = (0xff & (tkey >> 24));
    key[1] = (0xff & (tkey >> 16));
    key[2] = (0xff & (tkey >> 8));
    key[3] = (0xff & (tkey));

    return key_size;
}

unsigned int hz_sub12_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size)
{
    uint32_t tseed = 0;
    uint32_t tkey = 0;

    tseed |= (seed[0] << 24);
    tseed |= (seed[1] << 16);
    tseed |= (seed[2] << 8);
    tseed |= (seed[3]);

    tkey = GENERIC_ALGORITHM(tseed, BOOT_MASK);

    key[0] = (0xff & (tkey >> 24));
    key[1] = (0xff & (tkey >> 16));
    key[2] = (0xff & (tkey >> 8));
    key[3] = (0xff & (tkey));

    return key_size;
}

#ifdef __cplusplus
}
#endif

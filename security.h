#ifndef SECURITY_H
#define SECURITY_H
#ifdef __cplusplus
extern "C" {
#endif

unsigned int dp_security_access_02_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size);
unsigned int dp_security_access_0a_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size);

unsigned int hz_sub04_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size);
unsigned int hz_sub12_generate_key(unsigned char *seed, unsigned char seed_size, unsigned char *key, unsigned char key_size);
#ifdef __cplusplus
}
#endif
#endif // SECURITY_H

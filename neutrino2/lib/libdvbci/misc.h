#ifndef __MISC_H_
#define __MISC_H_

void hexdump(const uint8_t *data, unsigned int len);

int parseLengthField(const unsigned char *pkt, int *len);

int get_random(unsigned char *dest, int len);

int add_padding(uint8_t *dest, unsigned int len, unsigned int blocklen);

void str2bin(uint8_t *dst, char *data, int len);

uint32_t UINT32(const uint8_t *in, unsigned int len);

int BYTE32(uint8_t *dest, uint32_t val);
int BYTE16(uint8_t *dest, uint16_t val);

#endif

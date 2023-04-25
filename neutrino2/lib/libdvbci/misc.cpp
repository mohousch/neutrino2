#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

void hexdump(const uint8_t *data, unsigned int len)
{
	while (len--)
		printf("%02x ", *data++);
	printf("\n");
}

int get_random(unsigned char *dest, int len)
{
	int fd;
	const char *urnd = "/dev/urandom";

	fd = open(urnd, O_RDONLY);
	if (fd <= 0)
	{
		printf("cannot open %s\n", urnd);
		return -1;
	}

	if (read(fd, dest, len) != len)
	{
		printf("cannot read from %s\n", urnd);
		close(fd);
		return -2;
	}

	close(fd);

	return len;
}

int parseLengthField(const unsigned char *pkt, int *len)
{
	int i;

	*len = 0;
	if (!(*pkt & 0x80))
	{
		*len = *pkt;
		return 1;
	}
	for (i = 0; i < (pkt[0] & 0x7F); ++i)
	{
		*len <<= 8;
		*len |= pkt[i + 1];
	}
	return (pkt[0] & 0x7F) + 1;
}

int add_padding(uint8_t *dest, unsigned int len, unsigned int blocklen)
{
	uint8_t padding = 0x80;
	int count = 0;

	while (len & (blocklen - 1))
	{
		*dest++ = padding;
		++len;
		++count;
		padding = 0;
	}

	return count;
}

static int get_bin_from_nibble(int in)
{
	if ((in >= '0') && (in <= '9'))
		return in - 0x30;

	if ((in >= 'A') && (in <= 'Z'))
		return in - 0x41 + 10;

	if ((in >= 'a') && (in <= 'z'))
		return in - 0x61 + 10;

	printf("fixme: unsupported chars in hostid\n");

	return 0;
}

void str2bin(uint8_t *dst, char *data, int len)
{
	int i;

	for (i = 0; i < len; i += 2)
		* dst++ = (get_bin_from_nibble(data[i]) << 4) | get_bin_from_nibble(data[i + 1]);
}

uint32_t UINT32(const unsigned char *in, unsigned int len)
{
	uint32_t val = 0;
	unsigned int i;

	for (i = 0; i < len; i++)
	{
		val <<= 8;
		val |= *in++;
	}

	return val;
}

int BYTE32(unsigned char *dest, uint32_t val)
{
	*dest++ = val >> 24;
	*dest++ = val >> 16;
	*dest++ = val >> 8;
	*dest++ = val;

	return 4;
}

int BYTE16(unsigned char *dest, uint16_t val)
{
	*dest++ = val >> 8;
	*dest++ = val;
	return 2;
}

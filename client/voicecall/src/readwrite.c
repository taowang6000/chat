
#include "readwrite.h"


void write_be64(uint8_t *p, uint64_t v)
{
	p[0] = (v >> 56) & 0xFF;
	p[1] = (v >> 48) & 0xFF;
	p[2] = (v >> 40) & 0xFF;
	p[3] = (v >> 32) & 0xFF;
	p[4] = (v >> 24) & 0xFF;
	p[5] = (v >> 16) & 0xFF;
	p[6] = (v >>  8) & 0xFF;
	p[7] = (v      ) & 0xFF;
}

void write_be32(uint8_t *p, uint32_t v)
{
	p[0] = (v >> 24) & 0xFF;
	p[1] = (v >> 16) & 0xFF;
	p[2] = (v >>  8) & 0xFF;
	p[3] = (v      ) & 0xFF;
}

void write_be16(uint8_t *p, uint16_t v)
{
	p[0] = (v >>  8) & 0xFF;
	p[1] = (v      ) & 0xFF;
}


void write_le64(uint8_t *p, uint64_t v)
{
	p[0] = (v      ) & 0xFF;
	p[1] = (v >>  8) & 0xFF;
	p[2] = (v >> 16) & 0xFF;
	p[3] = (v >> 24) & 0xFF;
	p[4] = (v >> 32) & 0xFF;
	p[5] = (v >> 40) & 0xFF;
	p[6] = (v >> 48) & 0xFF;
	p[7] = (v >> 56) & 0xFF;
}

void write_le32(uint8_t *p, uint32_t v)
{
	p[0] = (v      ) & 0xFF;
	p[1] = (v >>  8) & 0xFF;
	p[2] = (v >> 16) & 0xFF;
	p[3] = (v >> 24) & 0xFF;
}

void write_le16(uint8_t *p, uint16_t v)
{
	p[0] = (v      ) & 0xFF;
	p[1] = (v >>  8) & 0xFF;
}


/* read functions, be */

uint64_t read_be64(const uint8_t *p)
{
	int i;
	uint64_t r = 0;

	for (i = 0; i < 8; i++)
		r |= p[i] << (7-i)*8;

	return r;
}

uint32_t read_be32(const uint8_t *p)
{
	return ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

uint16_t read_be16(const uint8_t *p)
{
	return ((p[0] << 8) | p[1]);
}

/* read functions, le */

uint64_t read_le64(const uint8_t *p)
{
	int i;
	uint64_t r = 0;

	for (i = 0; i < 8; i++)
		r |= p[i] << i*8;

	return r;
}

uint32_t read_le32(const uint8_t *p)
{
	return ((p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]);
}

uint16_t read_le16(const uint8_t *p)
{
	return ((p[1] << 8) | p[0]);
}

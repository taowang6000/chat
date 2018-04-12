
#include "xtea.h"
#include "readwrite.h"

static void xtea_encipher(uint8_t block[8], const uint32_t key[4])
{
	uint32_t i, v0, v1, sum = 0, delta = 0x9E3779B9;

	v0 = read_be32(block);
	v1 = read_be32(block+4);
	for (i = 0; i < 32; i++) {
		v0  += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		sum += delta;
		v1  += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
	}
	write_be32(block,   v0);
	write_be32(block+4, v1);
}

static void xtea_decipher(uint8_t block[8], const uint32_t key[4])
{
	uint32_t i, v0, v1, delta = 0x9E3779B9, sum = delta*32;

	v0 = read_be32(block);
	v1 = read_be32(block+4);
	for (i = 0; i < 32; i++) {
		v1  -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
		sum -= delta;
		v0  -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
	}
	write_be32(block,   v0);
	write_be32(block+4, v1);
}

void xtea_encrypt_buffer_ecb(uint8_t *buf, size_t len, const uint32_t key[4])
{
	int i;

	for (i = 0; i < len; i+=8)
		xtea_encipher(buf + i, key);
}

void xtea_decrypt_buffer_ecb(uint8_t *buf, size_t len, const uint32_t key[4])
{
	int i;

	for (i = 0; i < len; i+=8)
		xtea_decipher(buf + i, key);
}

void xtea_encrypt_buffer_cbc(uint8_t *buf, size_t len, const uint32_t key[4], const uint32_t iv[2])
{
	int i;
	uint32_t iiv[2];

	iiv[0] = iv[0];
	iiv[1] = iv[1];
	for (i = 0; i < len; i+=8) {
		write_be32(buf + i,     read_be32(buf + i)     ^ iiv[0]);
		write_be32(buf + i + 4, read_be32(buf + i + 4) ^ iiv[1]);
		xtea_encipher(buf + i, key);
		iiv[0] = read_be32(buf + i);
		iiv[1] = read_be32(buf + i + 4);
	}
}

void xtea_decrypt_buffer_cbc(uint8_t *buf, size_t len, const uint32_t key[4], const uint32_t iv[2])
{
	int i;
	uint32_t iiv[2], cpt[2];

	iiv[0] = iv[0];
	iiv[1] = iv[1];
	for (i = 0; i < len; i+=8) {
		cpt[0] = read_be32(buf + i);
		cpt[1] = read_be32(buf + i + 4);
		xtea_decipher(buf + i, key);
		write_be32(buf + i,     read_be32(buf + i)     ^ iiv[0]);
		write_be32(buf + i + 4, read_be32(buf + i + 4) ^ iiv[1]);
		iiv[0] = cpt[0];
		iiv[1] = cpt[1];
	}
}

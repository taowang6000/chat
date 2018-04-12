
#ifndef _READWRITE_H_
#define _READWRITE_H_

#include <stdint.h>

/* These functions write an integer to the memory location pointed by
 * p. We have two group of functions: _be functions write in big endian
 * order, while _le functions write in little endian order.
 */

void write_be64(uint8_t *p, uint64_t v);
void write_be32(uint8_t *p, uint32_t v);
void write_be16(uint8_t *p, uint16_t v);

void write_le64(uint8_t *p, uint64_t v);
void write_le32(uint8_t *p, uint32_t v);
void write_le16(uint8_t *p, uint16_t v);

/* These functions read an integer from the memory location pointed by
 * p. Again two groups: _be for big endian and _le for little endian.
 */

uint64_t read_be64(const uint8_t *p);
uint32_t read_be32(const uint8_t *p);
uint16_t read_be16(const uint8_t *p);

uint64_t read_le64(const uint8_t *p);
uint32_t read_le32(const uint8_t *p);
uint16_t read_le16(const uint8_t *p);

#endif

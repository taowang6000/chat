
#include <stdlib.h>
#include <string.h>
#include "oggopus.h"
#include "readwrite.h"

#define DEBUGOGG 0

#if DEBUGOGG
static void print_opacket_info(ogg_packet *opacket)
{
	fprintf(stderr, "P(%5ld) %s%s no=0x%016lx gpos=0x%016lx\n", opacket->bytes,
			opacket->b_o_s ? "B" : " ", opacket->e_o_s ? "E" : " ",
			opacket->packetno, opacket->granulepos);
}

static void print_opage_info(ogg_page *opage)
{
	fprintf(stderr, "  page: serialno=0x%08x %s%s no=%08ld C%d P%02d gpos=0x%016lx len=%ld\n",
			ogg_page_serialno(opage), ogg_page_bos(opage) ? "B" : " ", ogg_page_eos(opage) ? "E" : " ",
			ogg_page_pageno(opage), ogg_page_continued(opage), ogg_page_packets(opage),
			ogg_page_granulepos(opage), opage->header_len + opage->body_len);
}
#endif

ogg_packet *make_opus_header0_oggpacket(unsigned char channels, unsigned int samplerate)
{
	int size = 19;
	ogg_packet *op;
	unsigned char *data;

	op   = malloc(sizeof(*op));
	data = malloc(size);
	if (!op || !data)
		return NULL;

	memcpy(data, "OpusHead", 8);     /* identifier */
	data[8] = 1;                     /* version */
	data[9] = channels;              /* channels */
	write_le16(data+10, 0);          /* pre-skip */
	write_le32(data+12, samplerate); /* original sample rate */
	write_le16(data+16, 0);          /* gain */
	data[18] = 0;                    /* channel mapping family */

	op->packet     = data;
	op->bytes      = size;
	op->b_o_s      = 1;
	op->e_o_s      = 0;
	op->granulepos = 0;
	op->packetno   = 0;

	return op;
}

ogg_packet *make_opus_header1_oggpacket(char *vendor)
{
	char *identifier = "OpusTags";
//	char *vendor     = "test";
	int size = strlen(identifier) + 4 + strlen(vendor) + 4;
	ogg_packet *op;
	unsigned char *data;

	op   = malloc(sizeof(*op));
	data = malloc(size);
	if (!op || !data)
		return NULL;

	memcpy(data, identifier, 8);
	write_le32(data+8, strlen(vendor));
	memcpy(data+12, vendor, strlen(vendor));
	write_le32(data+12+strlen(vendor), 0);

	op->packet     = data;
	op->bytes      = size;
	op->b_o_s      = 0;
	op->e_o_s      = 0;
	op->granulepos = 0;
	op->packetno   = 1;

	return op;
}

int write_ogg_packet(ogg_stream_state *ostream, ogg_packet *opacket, FILE *fp, int flush)
{
	ogg_page opage;

#if DEBUGOGG
	print_opacket_info(opacket);
#endif
	/* Submit a raw packet to the streaming layer */
	if (ogg_stream_packetin(ostream, opacket) == 0) {
		/* Output a completed page if the stream contains enough packets to form a full page. */
		while (flush ? ogg_stream_flush(ostream, &opage) : ogg_stream_pageout(ostream, &opage)) {
#if DEBUGOGG
			print_opage_info(&opage);
#endif
			fwrite(opage.header, 1, opage.header_len, fp);
			fwrite(opage.body  , 1, opage.body_len,   fp);
		}
	} else {
		fprintf(stderr, "Error on ogg_stream_packetin().\n");
		return -1;
	}
	return 0;
}

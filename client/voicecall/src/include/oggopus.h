
#ifndef OGGOPUS_H
#define OGGOPUS_H

#include <stdio.h>
#include <ogg/ogg.h>

ogg_packet *make_opus_header0_oggpacket(unsigned char channels, unsigned int samplerate);
ogg_packet *make_opus_header1_oggpacket(char *vendor);
int write_ogg_packet(ogg_stream_state *ostream, ogg_packet *opacket, FILE *fp, int flush);

#endif

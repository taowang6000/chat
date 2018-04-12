
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <opus.h>
#include <ogg/ogg.h>
#include "common.h"
#include "audio.h"
#include "readwrite.h"
#include "xtea.h"
#include "oggopus.h"
#include "notes.h"

#define VERSION "0.10beta4"
#define DEBUG    0

#define UDP_PACKET_CLR_HEADER_LEN  4
#define UDP_PACKET_ENC_HEADER_LEN  12
#define UDP_PACKET_PAYLOAD_LEN 4000
#define UDP_PACKET_MAXLEN      (UDP_PACKET_ENC_HEADER_LEN + UDP_PACKET_PAYLOAD_LEN)
#define EOS_MARKER             0xFFFFFFFF


struct udp_packet_st {
	struct sockaddr_in addr;
	uint8_t            data[UDP_PACKET_MAXLEN];
	size_t             len;
};

static volatile sig_atomic_t keep_going;
static volatile sig_atomic_t exit_child;

static void signal_handler(int sig)
{
	if (sig == SIGINT)
		keep_going = 0;
	if (sig == SIGUSR1)
		exit_child = 1;
}

static void check_opus_error_or_die(int error)
{
	if (error != OPUS_OK) {
		snprintf(msgbuf, MBS, "opus error: %s\n", opus_strerror(error));
		die(msgbuf, 1);
	}
}

static opus_int32 convert_opus_bw(opus_int32 bandwidth)
{
	switch (bandwidth) {
	case OPUS_BANDWIDTH_NARROWBAND:
		return 4000;
	case OPUS_BANDWIDTH_MEDIUMBAND:
		return 6000;
	case OPUS_BANDWIDTH_WIDEBAND:
		return 8000;
	case OPUS_BANDWIDTH_SUPERWIDEBAND:
		return 12000;
	case OPUS_BANDWIDTH_FULLBAND:
		return 20000;
	default: /* AUTO */
		return 0;
	}
}


static int resolve_address(char *host, int port, struct sockaddr_in *address)
{
	struct hostent *hostinfo = NULL;
	char **ppchar;

	/* let's see if we already have a valid ip adress */
	if (inet_aton(host, &address->sin_addr)) {
//		fprintf(stderr, "Host: %s\n", host);
	} else {
		/* get host info */
		hostinfo = gethostbyname(host);
		if (!hostinfo) {
			fprintf(stderr, "Cannot get info for host: %s\n", host);
			return -1;
		}

		/* print host info */
		//fprintf(stderr, "Host: %s\n", host);
		//fprintf(stderr, "  Name: %s\n", hostinfo->h_name);
		//fprintf(stderr, "  Aliases:");
		for (ppchar = hostinfo->h_aliases; *ppchar; ppchar++)
//			fprintf(stderr, " %s", *ppchar);
		//fprintf(stderr, "\n");
		//fprintf(stderr, "  Type: %s\n", hostinfo->h_addrtype == AF_INET ? "AF_INET" : "AF_INET6");
		//fprintf(stderr, "  Address list:");
		for (ppchar = hostinfo->h_addr_list; *ppchar; ppchar++)
	//		fprintf(stderr, " %s", inet_ntoa(*(struct in_addr *)*ppchar) );
//		fprintf(stderr, "\n");

		if (hostinfo->h_addrtype == AF_INET6) {
	//		fprintf(stderr, "AF_INET6 not supported.\n");
			return -1;
		}
	}

//	fprintf(stderr, "Port: %d\n\n", port);

	/* fill in address */
	address->sin_family   = AF_INET;
	if (hostinfo)
		address->sin_addr = *(struct in_addr *) *hostinfo->h_addr_list;
	address->sin_port     = htons(port);

	return 0;
}

static void send_udp_packet(int sockfd, struct udp_packet_st *udp_packet)
{
	ssize_t nw;

	nw = sendto(sockfd, udp_packet->data, udp_packet->len, 0,
		        (struct sockaddr *)&udp_packet->addr, sizeof(udp_packet->addr));
	if (nw == -1) {
		snprintf(msgbuf, MBS, "sendto(): %s\n", strerror(errno));
		die(msgbuf, 1);
	}
	if (nw < udp_packet->len) {
		snprintf(msgbuf, MBS, "sendto() returned %zd, expected %zd\n", nw, udp_packet->len);
		print_msg_nl(msgbuf);
	}
}

static void receive_udp_packet(int sockfd, struct udp_packet_st *udp_packet)
{
	ssize_t   nr;
	socklen_t addr_len;

	udp_packet->len = UDP_PACKET_MAXLEN;
	addr_len = sizeof(udp_packet->addr);
	nr = recvfrom(sockfd, udp_packet->data, udp_packet->len, 0,
		          (struct sockaddr *)&udp_packet->addr, &addr_len);
	if (nr == -1) {
		snprintf(msgbuf, MBS, "recvfrom(): %s\n", strerror(errno));
		die(msgbuf, 1);
	}

	udp_packet->len = nr;
}

static int is_same_addr(struct sockaddr_in a, struct sockaddr_in b)
{
	if (a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port)
		return 1;
	else
		return 0;
}

static pid_t play_ringtone(char *audiodevice, int verbose)
{
	pid_t pid;
	struct sigaction act;
	int ret;
	const int rate = 48000, channels = 2;
	struct audio_data ad_playback;
#define D 500
	struct note  {
		double freq;
		int    duration; //in milliseconds
	} const song[] = {   //nokia ringtone
		{NOTE_E6,  D/2},
		{NOTE_D6,  D/2},
		{NOTE_F5d, D},
		{NOTE_G5d, D},
		{NOTE_C6d, D/2},
		{NOTE_B5,  D/2},
		{NOTE_D5,  D},
		{NOTE_E5,  D},
		{NOTE_B5,  D/2},
		{NOTE_A5,  D/2},
		{NOTE_C5d, D},
		{NOTE_E5,  D},
		{NOTE_A5,  2*D+D},
		{  0.0,    0}
	};
#undef D
	int n, i, samples, pos;
	int16_t *buf, *start;
	int      buflen;
	double duration = 0.0, v;

	pid = fork();
	if (pid == -1) {
		perror("fork()");
		exit(1);
	}

	if (pid == 0) {
		/* install signal handler */
		act.sa_handler = signal_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_RESTART;
		ret = sigaction(SIGUSR1, &act, NULL);
		if (ret == -1) {
			fprintf(stderr, "Error: sigaction() failed.\n");
			exit(1);
		}

		/* set global variable */
		exit_child = 0;

		/* compute ringtone duration */
		for (n = 0; song[n].duration != 0; n++)
			duration += song[n].duration;
		duration /= 1000.0;
		duration += 2; /* +2s silence */

		/* allocate memory for the samples buffer */
		buflen = ceil(duration * rate);
		buf    = calloc(channels * sizeof(*buf), buflen);
		if (!buf) {
			fprintf(stderr, "Error on calloc()\n");
			exit(1);
		}

		/* synthesize ringtone */
		pos = 0;
		for (n = 0; song[n].duration != 0; n++) {
			samples = rate * song[n].duration / 1000;
			for (i = 0; i < samples; i++) {
				v = 32000.0 * exp(-(double)i / (1.0 * samples)) * sin(2.0 * M_PI * song[n].freq * i / (double)rate);
				if (channels == 1) {
					buf[i + pos] = lrint(v);
				} else {
					buf[channels*i + pos    ] = lrint(v);
					buf[channels*i + pos + 1] = lrint(v);
				}
			}
			pos += channels*samples;
		}

		/* init audio */
		ret = audio_init(&ad_playback, audiodevice, SND_PCM_STREAM_PLAYBACK,
			             rate, channels, NULL, verbose >= 2 ? 1 : 0);
		if (ret == -1)
			exit(1);

		/* play ringtone until interrupted */
		n = 0;
		while (1) {
			start   = buf + channels*n;
			samples = (buflen - n) > (rate/10) ? (rate/10) : (buflen - n);
			ret = audio_write(&ad_playback, start, samples, 0);
			if (ret < 0)
				exit(1);
			if (exit_child)
				break;
			n += rate/10;
			if (n >= buflen)
				n = 0;
		}

		/* clean up and exit */
		ret = audio_close(&ad_playback);
		free(buf);
		exit(0);
	}

	return pid;
}

static uint8_t hex2num(uint8_t c)
{
	if ('0' <= c && c <= '9')
		return (c - '0');
	else if ('a' <= c && c <= 'f')
		return (c - 'a' + 10);
	else if ('A' <= c && c <= 'F')
		return (c - 'A' + 10);
	else
		return 0;
}

static int read_key(uint32_t key[4], char *keyfile)
{
	int i;
	char buf[128], *s;
	size_t len;
	FILE *fp;

	buf[0] = '\0';
	if (!keyfile) {
		fprintf(stderr, "Insert hex key (32 chars max): ");
		s = fgets(buf, 128, stdin);
	} else {
		fp = fopen(keyfile, "r");
		if (!fp)
			return -1;
		fprintf(stderr, "Reading key from file '%s'\n", keyfile);
		s = fgets(buf, 128, fp);
		fclose(fp);
	}

	len = strlen(buf);
	if (len == 0 || s == NULL)
		return -1;

	/* delete '\n' */
	if (buf[len-1] == '\n')
		buf[len-1] = '\0';
	len = strlen(buf);
	if (len == 0)
		return -1;

	/* key must be 128bit = 16 bytes = 32 hex characters.
	   if we have less characters, fill with '0' */
	if (len < 32) {
		for (i = len; i < 32; i++)
			buf[i] = '0';
	}
	buf[32] = '\0';

	/* convert key from hex and store it to key[] array */
	key[0] = key[1] = key[2] = key[3] = 0;
	for (i = 0; i < 32; i++)
		key[i/8] |= hex2num(buf[i]) << 4*(7 - i%8);

	return 0;
}

static void print_key(uint32_t key[4])
{
	int i;

	fprintf(stderr, "Key: ");
	for (i = 0; i < 4; i++)
		fprintf(stderr, "%08x", key[i]);
	fprintf(stderr, "\n");
}


static void usage()
{
	fprintf(stderr,
"Usage: parole [options] [-l | -c host]\n"
"Options:\n"
"       -h             print help\n"
"       -v             increment verbosity level\n"
"       -p port        listening port            (8110)\n"
"       -b bitrate     set bitrate in bit/s      (16000)\n"
"       -d audiodev    use audio device audiodev (hw:0,0)\n"
"       -a             autoaccept calls\n"
"       -e             encrypt call with xtea-cbc\n"
"       -k keyfile     read key from keyfile\n"
"       -r             record call in ogg/opus\n"
"Modes:\n"
"       -l             listen for calls\n"
"       -c host[:port] call host\n"
	);
}

int main(int argc, char *argv[])
{
	int                  ret, opt, i, error;
	int                  verbose = 0, listen = 0, call = 0, autoaccept = 0;
	struct sigaction     act;
	struct pollfd        pfds[6];
	int                  nfds, pollret;
	struct timeval       tea0, tea1; /* encode audio */
	struct timeval       tda0, tda1; /* decode audio */
	long int             t_enca, t_deca;
	int                  ic, ip, timeout;

	/* encrypt */
	int                  encrypt = 0;
	uint32_t             key[4];
	uint32_t             iv[2];
	uint8_t              padding_size;
	char                *keyfile = NULL;

	/* udp socket */
	int                  sockfd;
	int                  local_port = 8110;
	struct sockaddr_in   local_addr;
	char                *remote_host = NULL;
	int                  remote_port = 8110;
	struct sockaddr_in   remote_addr;
	struct udp_packet_st udp_packet;
	uint32_t             sequence_number;
	uint32_t             last_sequence_number;
	size_t               total_bytes_out;
	size_t               total_bytes_in;

	/* audio data */
	struct audio_data    ad_capture, ad_playback;
	char                *audiodevice = "hw:0,0";
	const unsigned int   rate = 48000, channels = 2;
	float                peak_percent;
	int                  keep_going_capture, overruns, underruns;

	/* opus encoder */
	OpusEncoder         *enc;
	opus_int32           bitrate = 16000, bandwidth, complexity;
//	opus_int16           inbuffer[2*960];	/* 2ch, 20ms at 48kHz (FS * 0.02) */
	uint8_t              opus_enc_packet[4000];
	opus_int32           opus_enc_packetlen;

	/* opus decoder */
	OpusDecoder         *dec;
	uint8_t             *opus_dec_packet;
	opus_int32           opus_dec_packetlen;
	opus_int16           pcm[2*5760];   /* 2ch, 120ms at 48kHz */
	opus_int32           pcmlen;
	int                  packets_lost;

	/* record in ogg/opus */
	int                  record = 0;
	FILE                *fp_rec_local  = NULL;
	FILE                *fp_rec_remote = NULL;
	char                 filename_rec_local[64];
	char                 filename_rec_remote[64];
	ogg_stream_state     ostream_rec_local;
	ogg_stream_state     ostream_rec_remote;
	ogg_packet          *opacket_opus_header[2];
	ogg_packet           opacket;
	ogg_page             opage;

	srand(time(NULL));
	//fprintf(stderr, "parole, ver. %s\n\n", VERSION);
	newline = 1;

	while ((opt = getopt(argc, argv, "hvp:b:d:aek:rlc:")) != -1) {
		char *pc;

		switch (opt) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'v':
			verbose++;
			break;
		case 'p':
			local_port = atoi(optarg);
			break;
		case 'b':
			bitrate = atoi(optarg);
			break;
		case 'd':
			audiodevice = optarg;
			break;
		case 'a':
			autoaccept = 1;
			break;
		//case 'e':

		//	encrypt = 1;
		//	break;
		//case 'k':
		//	keyfile = optarg;
		//	break;
		//case 'r':
		//	record = 0; //record the 
		//	break;
		case 'l':
			listen = 1;
			break;
		case 'c':
			call = 1;
			pc = strchr(optarg, ':');
			if (pc) {
				*pc = '\0';
				pc++;
				if (*pc)
					remote_port = atoi(pc);
			}
			remote_host = optarg;
			break;
		default:
			fprintf(stderr, "\n");
			usage();
			exit(1);
		}
	}
#if 0
	fprintf(stderr, "verbose=%d, local_port=%d, bitrate=%d, audiodev=%s, autoaccept=%d, encrypt=%d, "
	        "keyfile=%s, record=%d, listen=%d, call=%d, remote_host=%s, remote_port=%d\n",
	        verbose, local_port, bitrate, audiodevice, autoaccept, encrypt, keyfile==NULL ? "NULL" : keyfile,
	        record, listen, call, remote_host==NULL ? "NULL" : remote_host, remote_port);
#endif

	if (!listen && !call)
		die("Please select one option in {-l,-c}.\n", 1);

	if (listen && call)
		die("Please select only one option in {-l,-c}.\n", 1);

	/* This loop will be executed only once if we are in "call" mode,
	   or until the user presses CTRL-C in "listen" mode */
	while (1) {
		/* set global variable */
		keep_going = 1;

		/* create socket */
		sockfd = socket(PF_INET, SOCK_DGRAM, 0);
		if (sockfd == -1) {
			perror("socket()");
			exit(1);
		}

		/* make a call */
		if (call) {
			/* resolve remote address */
			//fprintf(stderr, "Resolving address...\n");
			ret = resolve_address(remote_host, remote_port, &remote_addr);
			if (ret == -1)
				exit(1);

			/* read key if the user requested encryption */
			if (encrypt) {
				ret = read_key(key, keyfile);
				if (ret == -1)
					die("Error: cannot read key.\n", 1);
				if (verbose)
					print_key(key);
			}

			/* send a call request */
			if (!encrypt) {
				udp_packet.len = 8;
				memcpy(udp_packet.data, "CALL-CLR", udp_packet.len);
			} else {
				uint32_t a0, a1;

				a0 = rand();
				a1 = rand();

				udp_packet.len = 24;
				memcpy(udp_packet.data, "CALL-ENC", 8);
				write_be32(udp_packet.data + 8,  a0);
				write_be32(udp_packet.data + 12, a1);
				write_be32(udp_packet.data + 16, ~a0);
				write_be32(udp_packet.data + 20, ~a1);

				iv[0] = iv[1] = 0;
				xtea_encrypt_buffer_cbc(udp_packet.data + 8, udp_packet.len - 8, key, iv);
			}
			udp_packet.addr = remote_addr;
			send_udp_packet(sockfd, &udp_packet);

			/* receive packet from remote host */
			//fprintf(stderr, "Call request sent, waiting for reply...\n");
			do {
				receive_udp_packet(sockfd, &udp_packet);
			} while (!is_same_addr(udp_packet.addr, remote_addr));

			/* check if the other side accepted the call */
			if (udp_packet.len == 2 && strncmp((char *)udp_packet.data, "OK", 2) == 0)
				fprintf(stderr, "Call established!\n");
			else
				die("Call refused.\n", 1);
		}

		/* wait for a call */
		if (listen) {
			/* name socket (bind it to local_port) */
			local_addr.sin_family      = AF_INET;
			local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			local_addr.sin_port        = htons(local_port);
			ret = bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr));
			if (ret == -1) {
				perror("bind()");
				exit(1);
			}

			while (1) {
				char answer;

				/* listen for a call request (this call will block) */
				//fprintf(stderr, "Listening for incoming calls on udp port %d...\n", local_port);
				receive_udp_packet(sockfd, &udp_packet);
				remote_addr = udp_packet.addr;

				/* is it a valid CALL packet? */
				if        (udp_packet.len == 8  && strncmp((char *)udp_packet.data, "CALL-CLR", 8) == 0) {
					encrypt = 0; /* ok, valid clear call */
				} else if (udp_packet.len == 24 && strncmp((char *)udp_packet.data, "CALL-ENC", 8) == 0) {
					encrypt = 1; /* ok, valid encrypted call */
				} else {
					fprintf(stderr, "Invalid CALL packet received\n");
					continue;
				}

				/* yes, valid call. ask the user if we can accept it */
				//fprintf(stderr, "Call from:  %s:%hu\n", inet_ntoa(remote_addr.sin_addr), ntohs(remote_addr.sin_port));
				//fprintf(stderr, "Encryption: %s\n", encrypt ? "on" : "off");
				//fprintf(stderr, "Accept call? [y/n] ");

				if (!autoaccept) {
					pid_t child, retpid;
					char buf[128], *s;

					child = play_ringtone(audiodevice, verbose);

					s = fgets(buf, 128, stdin);
					answer = (s == NULL) ? 'n' : buf[0];

					ret = kill(child, SIGUSR1);
					if (ret == -1) {
						perror("kill()");
						exit(1);
					}
					retpid = wait(NULL);
					if (retpid == -1) {
						perror("wait()");
						exit(1);
					}
				} else {
					answer = 'y';
					//fprintf(stderr, "%c\n", answer);
				}

				if (answer == 'y' && encrypt) {
					uint32_t a0, a1, b0, b1;

					ret = read_key(key, keyfile);
					if (ret == -1)
						die("Error: cannot read key.\n", 1);
					if (verbose)
						print_key(key);

					iv[0] = iv[1] = 0;
					xtea_decrypt_buffer_cbc(udp_packet.data + 8, udp_packet.len - 8, key, iv);

					a0 = read_be32(udp_packet.data + 8);
					a1 = read_be32(udp_packet.data + 12);
					b0 = read_be32(udp_packet.data + 16);
					b1 = read_be32(udp_packet.data + 20);
					if (!(a0 == ~b0 && a1 == ~b1)) {
						answer = 'n';
						fprintf(stderr, "Error: wrong key.\n");
					}
				}

				fprintf(stderr, "\n");

				/* send response */
				udp_packet.len = 2;
				memcpy(udp_packet.data, answer == 'y' ? "OK" : "NO", udp_packet.len);
				udp_packet.addr = remote_addr;
				send_udp_packet(sockfd, &udp_packet);

				if (answer == 'y')
					break;
			}
		}


		/* file descriptor for socket is always at pfds[0] */
		pfds[0].fd     = sockfd;
		pfds[0].events = POLLIN;
		nfds = 1;

		/* Init alsa audio capture */
		ret = audio_init(&ad_capture, audiodevice, SND_PCM_STREAM_CAPTURE,
			             rate, channels, pfds+nfds, verbose >= 2 ? 1 : 0);
		if (ret == -1)
			exit(1);
		nfds += ad_capture.nfds;

		/* Init alsa audio playback */
		ret = audio_init(&ad_playback, audiodevice, SND_PCM_STREAM_PLAYBACK,
			             rate, channels, pfds+nfds, verbose >= 2 ? 1 : 0);
		if (ret == -1)
			exit(1);
		/* we won't poll this device */
		//nfds += ad_playback.nfds;


		/* create encoder state */
		if (verbose)
			fprintf(stderr, "libopus: %s\n", opus_get_version_string());
		enc = opus_encoder_create(rate, channels, OPUS_APPLICATION_VOIP, &error);
		check_opus_error_or_die(error);
#if 0
		/* set maximum bandwidth */
		bandwidth = OPUS_BANDWIDTH_WIDEBAND;
		error = opus_encoder_ctl(enc, OPUS_SET_MAX_BANDWIDTH(bandwidth));
		check_opus_error_or_die(error);
#endif
		/* set bitrate */
		error = opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate));
		check_opus_error_or_die(error);
#if 0
		/* set complexity */
		complexity = 10;
		error = opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
		check_opus_error_or_die(error);
#endif
		/* set signal type */
		error = opus_encoder_ctl(enc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
		check_opus_error_or_die(error);

		/* create decoder state */
		dec = opus_decoder_create(rate, channels, &error);
		check_opus_error_or_die(error);

        
        record = 0; // forbit the record function
		if (record) {
			time_t now;

			now = time(NULL);
			sprintf(filename_rec_local,  "rec-local-%d.opus",  (int)(now & 0xFFFFFFFF));
			sprintf(filename_rec_remote, "rec-remote-%d.opus", (int)(now & 0xFFFFFFFF));
			fp_rec_local  = fopen(filename_rec_local,  "w");
			fp_rec_remote = fopen(filename_rec_remote, "w");
			if (fp_rec_local == NULL || fp_rec_remote == NULL)
				die("Error: cannot open destination file.\n", 1);

			/* ogg init */
			ret = ogg_stream_init(&ostream_rec_local, rand());
			if (ret != 0)
				die("Error on ogg_stream_init().\n", 1);
			ret = ogg_stream_init(&ostream_rec_remote, rand());
			if (ret != 0)
				die("Error on ogg_stream_init().\n", 1);

			/* opus header packets */
			opacket_opus_header[0] = make_opus_header0_oggpacket(channels, rate);
			opacket_opus_header[1] = make_opus_header1_oggpacket("parole "VERSION);
			if (!opacket_opus_header[0] || !opacket_opus_header[1])
				die("Error while creating ogg/opus header packets.\n", 1);

			for (i = 0; i < 2; i++)  {
				write_ogg_packet(&ostream_rec_local,  opacket_opus_header[i], fp_rec_local,  1);
				write_ogg_packet(&ostream_rec_remote, opacket_opus_header[i], fp_rec_remote, 1);
			}
		}

		/* install signal handler */
		act.sa_handler = signal_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_RESTART;
		ret = sigaction(SIGINT, &act, NULL);
		if (ret == -1)
			die("Error: sigaction() failed.\n", 1);

		/* start audio capture */
		ret = audio_start(&ad_capture);
		if (ret == -1)
			exit(1);
		keep_going_capture = 1;

		/* reset counters */
		t_enca  = 0;
		t_deca  = 0;
		ic      = 0; /* capture frame counter */
		ip      = 0; /* playback frame counter */
		timeout = 0; /* timeout frame counter */

		last_sequence_number = 0;
		total_bytes_out      = 0;
		total_bytes_in       = 0;
		peak_percent         = 0.0f;
		overruns             = 0;
		underruns            = 0;
		packets_lost         = 0;

		/* main loop */
		while (keep_going || keep_going_capture) {
			/* poll: is audio_from_capture or udp_socket ready? */
			pollret = poll(pfds, nfds, -1);

			/* poll can fail and return -1 when a signal arrives. errno will be EINTR */
			if (pollret == -1) {
				if (errno == EINTR) {
					continue;
				} else {
					snprintf(msgbuf, MBS, "poll(): %s\n", strerror(errno));
					die(msgbuf, 1);
				}
			}
#if 0
			/* timeout should never happen really... */
			if (pollret == 0) {
				print_msg_nl("Poll timeout\n");
				continue;
			}
#endif

			/** UDP SOCKET **/
			if (pfds[0].revents & POLLERR)
				die("Socket POLLERR\n", 1);

			/* process data from socket if ready */
			if (pfds[0].revents & POLLIN) {
				gettimeofday(&tda0, NULL);
				/* read udp_packet */
				receive_udp_packet(sockfd, &udp_packet);
				total_bytes_in += udp_packet.len;

				/* make sure this packet comes from the remote host */
				if (is_same_addr(udp_packet.addr, remote_addr)) {
					/* read packet header and reset timeout counter */
					sequence_number = read_be32(udp_packet.data);
					timeout = 0;

					/* end-of-stream marker */
					if (sequence_number == EOS_MARKER)
						keep_going = 0;

					/* ok, sequence numbers of packets must always increase */
					if (sequence_number == 0 || sequence_number > last_sequence_number) {
						/* check if we lost packets */
						if (sequence_number != last_sequence_number + 1 &&
							sequence_number != 0 && sequence_number != EOS_MARKER) {
							error = sequence_number - (last_sequence_number + 1);
#if DEBUG
							snprintf(msgbuf, MBS, "Oops, %d packets lost.\n", error);
							print_msg_nl(msgbuf);
#endif
							packets_lost += error;
						}

						last_sequence_number = sequence_number;

						if (!encrypt) {
							opus_dec_packet    = udp_packet.data + UDP_PACKET_CLR_HEADER_LEN;
							opus_dec_packetlen = udp_packet.len  - UDP_PACKET_CLR_HEADER_LEN;
						} else {
							/* read initialization vector */
							iv[0] = read_be32(udp_packet.data+4);
							iv[1] = read_be32(udp_packet.data+8);

							/* decrypt payload in packet*/
//							xtea_decrypt_buffer_ecb(udp_packet.data + UDP_PACKET_ENC_HEADER_LEN,
//							                        udp_packet.len  - UDP_PACKET_ENC_HEADER_LEN, key);
							xtea_decrypt_buffer_cbc(udp_packet.data + UDP_PACKET_ENC_HEADER_LEN,
									                udp_packet.len  - UDP_PACKET_ENC_HEADER_LEN, key, iv);

							padding_size = udp_packet.data[udp_packet.len - 1];

							opus_dec_packet    = udp_packet.data + UDP_PACKET_ENC_HEADER_LEN;
							opus_dec_packetlen = udp_packet.len  - UDP_PACKET_ENC_HEADER_LEN - padding_size;
						}

						/* decode */
						pcmlen = opus_decode(dec, opus_dec_packet, opus_dec_packetlen, pcm, 5760, 0);
						if (pcmlen < 0) {
							snprintf(msgbuf, MBS, "opus_decode() failed: %s\n", opus_strerror(pcmlen));
							die(msgbuf, 1);
						}

						/* playback decoded data */
						ret = audio_write(&ad_playback, pcm, pcmlen, 1);
						if (ret < 0) {
							/* underrun, let's handle it and try again */
							underruns++;
							ret = audio_recover(&ad_playback, ret, 1);
							if (ret == 0) {
								ret = audio_write(&ad_playback, pcm, pcmlen, 0);
								if (ret < 0)
									die(NULL, 1);
							} else {
								die(NULL, 1);
							}
						}

						if (record) {
							/* forge ogg packet */
							opacket.packet     = opus_dec_packet;
							opacket.bytes      = opus_dec_packetlen;
							opacket.b_o_s      = 0;
							opacket.e_o_s      = 0;
							opacket.granulepos = (ip+1)*960;
							opacket.packetno   = ip+2;

							write_ogg_packet(&ostream_rec_remote, &opacket, fp_rec_remote, 0);
						}

						/* increment frame counter */
						ip++;
					} else {
#if DEBUG
						print_msg_nl("Oops, packet duplicated or out of order.\n");
#endif
					}
				} else {
					snprintf(msgbuf, MBS, "Unexpected packet from: %s:%hu\n",
					        inet_ntoa(udp_packet.addr.sin_addr), ntohs(udp_packet.addr.sin_port));
					print_msg_nl(msgbuf);
				}

				gettimeofday(&tda1, NULL);
				t_deca = (tda1.tv_sec - tda0.tv_sec) * 1000000L + tda1.tv_usec - tda0.tv_usec;
			}


			/** AUDIO CAPTURE **/
			audio_poll_descriptors_revents(&ad_capture);

			if (ad_capture.revents & POLLERR) {
				if (!(ad_capture.revents & POLLIN))
					die("Audio Capture POLLERR\n", 1);
			}

			/* process captured audio if ready */
			if (ad_capture.revents & POLLIN && keep_going_capture) {
				gettimeofday(&tea0, NULL);

				/* read ad_capture.period_size frames of audio */
				ret = audio_read(&ad_capture, 1);
				if (ret < 0) {
					/* overrun, let's handle it and try again */
					overruns++;
					ret = audio_recover(&ad_capture, ret, 1);
					if (ret == 0) {
						ret = audio_start(&ad_capture);
						if (ret == -1)
							die(NULL, 1);
						ret = audio_read(&ad_capture, 0);
						if (ret < 0)
							die(NULL, 1);
					} else {
						die(NULL, 1);
					}
				}

				/* make sure we have read an entire frame */
				if (ad_capture.frames != ad_capture.alsabuffersize) {
					snprintf(msgbuf, MBS, "Error, we only read %ld frames instead of %lu.\n",
						    ad_capture.frames, ad_capture.alsabuffersize);
					die(msgbuf, 1);
				}

				/* increment timeout counter, exit after 5s without receiving packets */
				timeout++;
				if (timeout == 250) {
					keep_going = 0;
					print_msg_nl("Timeout\n");
				}

				/* find peak in captured audio */
				audio_find_peak(&ad_capture);
				if (peak_percent < ad_capture.peak_percent)
					peak_percent = ad_capture.peak_percent;

				/* encoding */
				opus_enc_packetlen = opus_encode(enc, ad_capture.alsabuffer, ad_capture.frames, opus_enc_packet, 4000);
				if (opus_enc_packetlen < 0) {
					snprintf(msgbuf, MBS, "opus_encode() failed: %s\n", opus_strerror(opus_enc_packetlen));
					die(msgbuf, 1);
				}

				/* forge udp packet */
				if (keep_going == 0) {
					keep_going_capture = 0;
					sequence_number    = EOS_MARKER;
				} else {
					sequence_number    = ic;
				}

				if (!encrypt) {
					/* udp header */
					write_be32(udp_packet.data, sequence_number); /* packet sequence number */
					/* the payload is simply the opus packet */
					memcpy(udp_packet.data + UDP_PACKET_CLR_HEADER_LEN, opus_enc_packet, opus_enc_packetlen);
					udp_packet.len = UDP_PACKET_CLR_HEADER_LEN + opus_enc_packetlen;
				} else {
					/* udp header */
					write_be32(udp_packet.data, sequence_number); /* packet sequence number */
					iv[0] = rand();
					iv[1] = rand();
					write_be32(udp_packet.data+4, iv[0]);         /* initialization vector */
					write_be32(udp_packet.data+8, iv[1]);
					/* payload */
					memcpy(udp_packet.data + UDP_PACKET_ENC_HEADER_LEN, opus_enc_packet, opus_enc_packetlen);
					/* insert some padding to make payload size multiple of 8 */
					padding_size = 8 - (opus_enc_packetlen % 8);
					memset(udp_packet.data + UDP_PACKET_ENC_HEADER_LEN + opus_enc_packetlen, padding_size, padding_size);
					udp_packet.len = UDP_PACKET_ENC_HEADER_LEN + opus_enc_packetlen + padding_size;

					/* encrypt payload in packet */
//					xtea_encrypt_buffer_ecb(udp_packet.data + UDP_PACKET_ENC_HEADER_LEN,
//					                        udp_packet.len  - UDP_PACKET_ENC_HEADER_LEN, key);
					xtea_encrypt_buffer_cbc(udp_packet.data + UDP_PACKET_ENC_HEADER_LEN,
						                    udp_packet.len  - UDP_PACKET_ENC_HEADER_LEN, key, iv);
				}
				total_bytes_out += udp_packet.len;

				/* send udp packet */
				udp_packet.addr = remote_addr;
				send_udp_packet(sockfd, &udp_packet);

				if (record) {
					/* forge ogg packet */
					opacket.packet     = opus_enc_packet;
					opacket.bytes      = opus_enc_packetlen;
					opacket.b_o_s      = 0;
					opacket.e_o_s      = 0;
					opacket.granulepos = (ic+1)*960;
					opacket.packetno   = ic+2;

					write_ogg_packet(&ostream_rec_local, &opacket, fp_rec_local, 0);
				}

				gettimeofday(&tea1, NULL);
				t_enca = (tea1.tv_sec - tea0.tv_sec) * 1000000L + tea1.tv_usec - tea0.tv_usec;

				/* increment frame counter */
				ic++;

				/* status bar */
				if (ic == 1) {
					/* get bitrate */
					error = opus_encoder_ctl(enc, OPUS_GET_BITRATE(&bitrate));
					check_opus_error_or_die(error);
					/* get bandwidth */
					error = opus_encoder_ctl(enc, OPUS_GET_BANDWIDTH(&bandwidth));
					check_opus_error_or_die(error);
					/* get complexity */
					error = opus_encoder_ctl(enc, OPUS_GET_COMPLEXITY(&complexity));
					check_opus_error_or_die(error);

					//fprintf(stderr, "bitrate = %d bit/s, bandwidth = %d Hz, complexity = %d\n",
				//	        bitrate, convert_opus_bw(bandwidth), complexity);
					//fprintf(stderr,
				//	        "\r%02d:%02d [rb:%3.0f%% pk:%5.1f%%] [o:%4.1f i:%4.1f kB/s] [rb:%3.0f%% pl:%d/%4.2f%% ur:%d]",
				//	              0,   0,      0.0,       0.0,        0.0,    0.0,            0.0,      0,  0.0,      0);
					newline = 0;
				}
				if ((ic%50) == 0) {
					/* read available frames */
					ret = audio_avail(&ad_capture, 1);
					if (ret < 0)
						ad_capture.avail = ad_capture.buffer_size;
					ret = audio_avail(&ad_playback, 1);
					if (ret < 0)
						ad_playback.avail = 0;

					//fprintf(stderr,
				//	        "\r%02d:%02d [rb:%3.0f%% pk:%5.1f%%] [o:%4.1f i:%4.1f kB/s] [rb:%3.0f%% pl:%d/%4.2f%% ur:%d]",
				//			ic/3000, (ic/50)%60,
				//			100.0f * ad_capture.avail/(float)ad_capture.buffer_size, peak_percent,
				//			total_bytes_out/1024.0, total_bytes_in/1024.0,
				//			100.0f * ad_playback.avail/(float)ad_playback.buffer_size,
				//			packets_lost, 100.0f*packets_lost/(float)ic, underruns);
					newline = 0;

					peak_percent    = 0.0f;
					total_bytes_out = 0;
					total_bytes_in  = 0;
				}
			}
		}
		fprintf(stderr, "\n\n");

		/* check if we still have stuff on the socket */
		while ((pollret = poll(pfds, 1, 250)) > 0) {
			if (pfds[0].revents & POLLIN)
				receive_udp_packet(sockfd, &udp_packet);
		}

		/* reset signal handler */
		act.sa_handler = SIG_DFL;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_RESTART;
		ret = sigaction(SIGINT, &act, NULL);
		if (ret == -1)
			die("Error: sigaction() failed.\n", 1);

		/* clean up for the next iteration */
		ret = close(sockfd);
		ret = audio_stop(&ad_capture);
		ret = audio_close(&ad_capture);
		ret = audio_stop(&ad_playback);
		ret = audio_close(&ad_playback);

		if (record) {
			while (ogg_stream_flush(&ostream_rec_local, &opage)) {
				fwrite(opage.header, 1, opage.header_len, fp_rec_local);
				fwrite(opage.body  , 1, opage.body_len,   fp_rec_local);
			}
			while (ogg_stream_flush(&ostream_rec_remote, &opage)) {
				fwrite(opage.header, 1, opage.header_len, fp_rec_remote);
				fwrite(opage.body  , 1, opage.body_len,   fp_rec_remote);
			}
			ogg_stream_clear(&ostream_rec_local);
			ogg_stream_clear(&ostream_rec_remote);
			fclose(fp_rec_local);
			fclose(fp_rec_remote);
			free(opacket_opus_header[0]);
			free(opacket_opus_header[1]);
		}

		/* destroy encoder state */
		opus_encoder_destroy(enc);

		/* destroy decoder state */
		opus_decoder_destroy(dec);

		/* repeat only if we are listening for calls */
		if (call)
			break;
	}

	return 0;
}

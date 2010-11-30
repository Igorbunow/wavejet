#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>     /* memset				*/
#include <unistd.h>     /* close, read, write	*/
#include <pthread.h>
#include <assert.h>
#include "numbers.h"
#include "scp.h"

#define HDR_DATAFLAG	0x80
#define HDR_SRERCHFLAG	0x89	/* Service Request Enable Register Change */
#define HDR_EOIFLAG		0x01
#define HDR_RESERVED	100
#define HDR_CLEAR		0x10

#define BUFSIZE			32
#define BUNCHSIZE		65536	/* Size of a bunch coming from the scope */
#define DUSTHEAP		128		/* Max unwanted bytes coming from the scope */

#if DEV
/* In case of costiveness */
int scp_hangup(scp_t *_s)
{
	header h;

	/* Prepare scope purgative */
	h.dataflag = HDR_CLEAR;
	h.reserved[0] = 1;
	h.reserved[1] = 0;
	h.reserved[2] = 0;
	h.len = 0;

	/* Insert purgative */
	write(_s->sockfd, &h, sizeof(header));

	/* Leave scope to rest a bit */
	close(_s->sockfd);
}

int scp_recall(scp_t *_s)
{
	struct sockaddr_in sockaddr;
	int status;

	/* Get new socket */
	_s->sockfd = socket(PF_INET, SOCK_STREAM, 0);

	/* Reconnect */
	do {
		sleep(SCP_TMT);
		status = connect(_s->sockfd,
						 (struct sockaddr *) &_s->sockaddr,
						 sizeof(struct sockaddr_in));
	} while (status < 0);
	#if DEBUG
	printf("reconnected!\n");
	#endif

	/* Mux stuff to play with timeout */
	FD_ZERO(&_s->fds);
	FD_SET(_s->sockfd, &_s->fds);
}
#endif

void scp_cmd_push(scp_t *_scope,
				   const char *_query,
				   void (*_hdlr)(int _last, char *_data, int _len, void *_dst),
				   void *_dst)
{
	scp_cmd *cmd;

	/* Check trmd? already queued (handled only in this block; enable/disable
	at will) */
	if (strcmp("trmd?", _query) == 0) {
		cmd = _scope->cmd_first;
		while (cmd != NULL) {
			if (strcmp(cmd->query, _query) == 0) {
				/* Found the query already exists. Drop this one. */
				return;
			}
			cmd = cmd->next;
		}
	}

	/* Lock mutex */
	pthread_mutex_lock(&_scope->mutex);

	/* Make a new command */
	cmd = malloc(sizeof(scp_cmd));
	/* Feed command */
	cmd->hdlr = _hdlr;
	cmd->next = NULL;
	cmd->dst = _dst;
	strncpy(cmd->query, _query, SCP_CMDLEN - 1);
 	/* Make sure command is null-terminated, cause strncpy doesn't garantee
	that (man strncpy to see what I mean) */
	cmd->query[SCP_CMDLEN - 1] = 0;
	/* Add to queue as last cmd (if no last yet, queue has never been used) */
	if (_scope->cmd_last) {
		_scope->cmd_last->next = cmd;
	} else {
		_scope->cmd_first = cmd;
	}
	_scope->cmd_last = cmd;
	/* Increment command count and signal there's a new command */
	pthread_mutex_lock(&_scope->mtxwrite);
	_scope->cmdc++;
	pthread_cond_signal(&_scope->cndwrite);
	/* XXX */
	#if DEBUG
	printf("%.3d commands in the queue\r", _scope->cmdc);
	#endif
	pthread_mutex_unlock(&_scope->mtxwrite);

	/* Unlock mutex */
	pthread_mutex_unlock(&_scope->mutex);
}

scp_cmd scp_cmd_pop(scp_t *_scope)
{
	scp_cmd cmd;
	scp_cmd *first;

	/* Lock mutex */
	pthread_mutex_lock(&_scope->mutex);

	/* Decrement command counter */
	_scope->cmdc--;

	/* Keep first's address in mind for later deletion */
	first = _scope->cmd_first;

	/* Get first command */
	cmd = *_scope->cmd_first;

	/* Replace first by second if any */
	if (_scope->cmdc > 0) {
		_scope->cmd_first = _scope->cmd_first->next;
	} else {
		_scope->cmd_first = NULL;
		_scope->cmd_last = NULL;
	}

	/* Ditch old first */
	free(first);

	/* Unlock mutex */
	pthread_mutex_unlock(&_scope->mutex);

	return cmd;
}

scp_cmd *scp_cmd_get(scp_t *_scope)
{
	scp_cmd *cmd;

	pthread_mutex_lock(&_scope->mutex);
	cmd = _scope->cmd_first;
	pthread_mutex_unlock(&_scope->mutex);

	return cmd;
}

/* To be called when you're done with the scope */
int scp_destroy(scp_t *_scope)
{
	int rc;

	if (_scope != NULL) {
		_scope->readkill = 1;
		_scope->writekill = 1;
		rc = pthread_mutex_lock(&_scope->mtxread);
		pthread_cond_signal(&_scope->cndread);
		pthread_mutex_unlock(&_scope->mtxread);

		rc = pthread_join(_scope->thdread, NULL);
		if (rc) {
		/* Already joined */
			return 0;
		}

/* 		rc = pthread_cond_destroy(&_scope->cndread);
		if (rc != 0) {
			fprintf(stderr, "Couldn't destroy condition on read thread\n");
			return 1;
		} */

/* 		rc = pthread_mutex_destroy(&_scope->mtxread);
		if (rc != 0) {
			fprintf(stderr, "Couldn't destroy mutex on read thread: %s\n",
					 strerror(rc));
			return 1;
		} */

		/* FIXME What are these quit mutices and conditions? Shall we destroy
		these too? */

		/* FIXME There's one 'mutex'. To be destroyed? */

		close(_scope->sockfd);
		free(_scope);
	}

	return 0;
}

#if DEV
/* Output should be allocated such that it can hold DTPOINTS integers */
/* Returns point count */
void scp_dtwave(scp_t *_scope, int _ch, int _dtpoints, int *_output)
{
	header header;
	int islastbunch = 0;
	char query[BUFSIZE];
	/* XXX */
	int dtpoints;
	
	/* Max 5-digits value (+1 with sign), so that's room enough */
	char strdata[16];	
	/* Data from scope converted into a proper integer */
	int intdata;		
	/* General purpose counters */
	int i, j = 0;		
	/* Length */
	int len;

	/* Choose wavesrc */
	snprintf(query, BUFSIZE, "wavesrc ch%d", _ch);
	scp_query(_scope, query);

	/* Run query */
	scp_query(_scope, "dtwave?");

	/* While all bunch have been not been read (i.e. N values and N-1
	commas) */
	/* FIXME Hangs here, sometimes, notably when upon (un-)settra */
	for (i = 0, j = 0; i < dtpoints * 2 - 1; i++) {
		#if DEBUG
		printf("%d/%d\n", i, dtpoints);
		#endif
		/* Read header */
		header = scp_readheader(_scope);

		/* Get new data from the scope */
		scp_nextbunch(_scope, strdata, header.len);

		if (i % 2 == 0) {	/* If it's data */
			_output[j++] = atoi(strdata);
		}
	}

	/* Two more newlines to read */
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, strdata, header.len);
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, strdata, header.len);
}
#endif

#if DEV
int scp_getdtpoints(scp_t *_scope)
{
	header header;
	int len;
	char result[16];

	scp_query(_scope, "dtpoints?");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, result, header.len);

	return atoi(result);
}
void scp_setdtpoints(scp_t *_scope, int _dtpoints)
{
	char query[BUFSIZE];

	snprintf(query, BUFSIZE, "dtpoints %d\n", _dtpoints);

	scp_query(_scope, query);
}
#endif

/* Write header, command, header again and OPC request */
void scp_query(scp_t *_scope, const char *_command)
{
	header h;

	/* Header */
	h.dataflag = HDR_DATAFLAG | HDR_EOIFLAG;
	h.reserved[0] = 1;
	h.reserved[1] = 0;
	h.reserved[2] = 0;
	h.len = htonl(strlen(_command)); /* Write bytes in appropriate order */

	/* Write header */
	write(_scope->sockfd, &h, sizeof(header));

	/* Write command */
	write(_scope->sockfd, _command, strlen(_command));

	/* Ask for OPC */
	/* FIXME BAD! The scope doesn't seem to answer in the order he's been supplied
	commands! I've sent a dtwave, then the OPC request that follows in the code
	just here and I received *first* the OPC, then the data! */
	/* We won't do what follows anymore: readout stream until there's nothing
	left to read using file operating mode O_NONBLOCK */
	#if 0
	header.len = htonl(strlen("*opc?")); /* Write bytes in appropriate order */
	write(_scope->sockfd, &header, sizeof(header));
	write(_scope->sockfd, "*opc?", strlen("*opc?"));
	#endif

	/* PS: Well I don't what I've smoked when I wrote that but it was
	hard stuff, by the looks of it. 'Course the scope answers in the
	order of supplied commands, it's not Restolys, it isn't! */
}

#if DEV
/* Convenient function that returns a header,
 * hoping the next thing to read is one.
 */
header scp_readheader(scp_t *_scope)
{
	header h;
	int readc = 0;	/* Read character count */

	/* Initialize header (in case you return prematurely) */
	bzero(&h, sizeof(header));

	/* Error checking */
	/* assert(_scope != NULL); */
	if (_scope == NULL) {
		return;
	}

	/* Read header safely: if read hasn't read the right count, insist a bit */
 	while (readc < 8) {
		readc += read(_scope->sockfd,
		              (char *) &h + readc,
					  sizeof(header) - readc);
	}

	/* Turn size in the right order */
	h.len = ntohl(h.len);

	return h;
}
#endif

#if DEV
/* Get next data bunch. Parameter output should be allocated */
void scp_nextbunch(scp_t *_scope, char *_output, int _len)
{
	int readc = 0; /* Read character count */

	/* Error checking */
	/* assert(_scope != NULL); */
	if (_scope == NULL) {
		return;
	}

	/* Read answer */
	while (readc < _len) {
		readc += read(_scope->sockfd, _output + readc, _len - readc);
	}

	/* Finish it nicely */
	_output[_len] = 0;
}
#endif

#if DEV
/* Copy string sent by scope to _output. It's the UI's job to match it against
the right menu entry. Parameter _output has to be allocated, of course. */
void scp_gettdiv(scp_t *_scope, char *_output)
{
	header header;

	/* Get value from scope */
	scp_query(_scope, "tdiv?");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);
}
void scp_settdiv(scp_t *_scope, const char *_value)
{
	double value;
	char query[BUFSIZE];

	/* Get value and convert it with a bit of magic powder */
	value = nb_engtod(_value);

	/* Lay out a query */
	snprintf(query, BUFSIZE, "tdiv %g\n", value);

	/* Get value from scope */
	scp_query(_scope, query);
}
#endif

#if DEV
/* Copy string sent by scope to _output. It's the UI's job to match it against
the right menu entry. Parameter _output has to be allocated, of course.
Parameter _ch is stuck in [1,4], mind! */
void scp_getvdiv(scp_t *_scope, int _ch, char *_output)
{
	header header;
	char query[BUFSIZE];

	/* Get value from scope */
	snprintf(query, BUFSIZE, "c%d:vdiv?", _ch);
	scp_query(_scope, query);

	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);
}
/* Parameter _ch is stuck in [1,4], mind! */
void scp_setvdiv(scp_t *_scope, int _ch, const char *_value)
{
	double value;
	char query[BUFSIZE];

	/* Get value and convert it with a bit of magic powder */
	value = nb_engtod(_value);

	/* Lay out a query */
	snprintf(query, BUFSIZE, "c%d:vdiv %g\n", _ch, value);

	/* Set value from scope */
	scp_query(_scope, query);
}
#endif

#if DEV
/* Get trigger source from scope */
void scp_gettsrc(scp_t *_scope, char *_output)
{
	header header;

	/* Get value from scope */
	scp_query(_scope, "tsrc?");

	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[header.len - 1] = 0;
}
void scp_settsrc(scp_t *_scope, const char *_value)
{
	char query[BUFSIZE];

	/* Lay out a query */
	snprintf(query, BUFSIZE, "tsrc %s\n",  _value);

	/* Set value to scope */
	scp_query(_scope, query);
}
#endif

#if DEV
/* Get trigger coupling from scope */
void scp_gettcpl(scp_t *_scope, char *_output)
{
	header header;

	/* Get value from scope */
	scp_query(_scope, "tcpl?");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[header.len - 1] = 0;
}
/* Set trigger coupling from scope */
void scp_settcpl(scp_t *_scope, const char *_value)
{
	char query[BUFSIZE];

	/* Get value from scope */
	snprintf(query, BUFSIZE, "tcpl %s\n", _value);
	scp_query(_scope, query);
}
#endif

#if DEV
/* Get trigger level from scope */
void scp_gettlvl(scp_t *_scope, char *_output)
{
	header header;

	/* Get value from scope */
	scp_query(_scope, "tlvl?");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[header.len - 1] = 0;
}
/* Set trigger level to scope */
void scp_settlvl(scp_t *_scope, const char *_value)
{
	char query[BUFSIZE];
	double value;

	/* Convert value to double */
	value = nb_engtod(_value);

	/* Set value to scope */
	snprintf(query, BUFSIZE, "tlvl %g\n", value);
	#if DEBUG
	printf("%s\n", query);
	#endif
	scp_query(_scope, query);
}
#endif

#if DEV
/* Write PNG file to fd
FIXME To be tackled again, in particular there are missing pieces in the
retrieved image */
void scp_gettscrn(scp_t *_scope, FILE *_fd)
{
	header header;
	char strbuf[16];
	char *pb = strbuf;			/* Pointer to buffer */
	unsigned imgsize;
	char *img;
	char *pc;					/* Character pointer */
	int readc = 0, readtc = 0, writec;
	int i;

	scp_query(_scope, "tscrn? PNG");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, pb, header.len);

	/* So far we've got the header the scope prefixes the image with */

	/* Get image size */
	pb += 2;
	imgsize = atoi(pb);
	#if DEBUG
	printf("size is %d\n", imgsize);
	#endif

	/* Get header again before datablock */
	header = scp_readheader(_scope);
	#if DEBUG
	printf("announcing %d bytes\n", header.len);
	#endif

	/* Make room for image */
	img = (char *) malloc(imgsize);
	pc = img;

	/* Get image */
	while (readtc < imgsize) {
		readc = read(_scope->sockfd, pc += readtc, imgsize - readtc);
		if (readc > 0) {
			readtc += readc;
			#if DEBUG
			printf("%d\n", readtc);
			#endif
		}
	}
	#if DEBUG
	printf("read %d bytes from socket\n", readtc);
	#endif

	/* Dump image to file */
	writec = fwrite(img, 1, imgsize, _fd);
	#if DEBUG
	printf("wrote %d bytes to file\n", writec);
	#endif

	/* Clean up */
	free(img);
}
#endif

#if DEV
/* Get trigger mode (AUTO, NORMAL, SINGLE, STOP) */
void scp_gettrmd(scp_t *_scope, char *_output)
{
	header h;

	/* Submit query */
	scp_query(_scope, "trmd?\n");
	h = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, h.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[h.len - 1] = 0;
}
/* Set trigger mode (AUTO, NORMAL, SINGLE, STOP) */
void scp_settrmd(scp_t *_scope, const char *_md)
{
	char query[BUFSIZE];

	/* Lay out a query */
	snprintf(query, BUFSIZE, "trmd %s\n", _md);

	/* Submit query */
	scp_query(_scope, query);
}
#endif

#if DEV
/* Get trace status (ON/OFF) */
void scp_gettra(scp_t *_scope, int _ch, char *_output)
{
	header header;
	char query[BUFSIZE];

	/* Get value from scope */
	snprintf(query, BUFSIZE, "c%d:tra?", _ch);
	scp_query(_scope, query);
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[header.len - 1] = 0;
}
/* Set trace status (ON/OFF) */
void scp_settra(scp_t *_scope, int _ch, const char *_onoff)
{
	char query[BUFSIZE];

	/* Set value to scope */
	snprintf(query, BUFSIZE, "c%d:tra %s", _ch, _onoff);

	scp_query(_scope, query);
}
#endif


#if DEV
/* Read trigger event status register */
/* FIXME REMOVEME, that is */
void scp_gettesr(scp_t *_scope, char *_output)
{
	header header;
	char query[16];

	/* Get value from scope */
	scp_query(_scope, "tesr?");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[header.len - 1] = 0;
}
#endif

#if DEV
/* Get trigger delay */
void scp_gettrdl(scp_t *_scope, char *_output)
{
	header header;

	/* Get value from scope */
	scp_query(_scope, "trdl?");
	header = scp_readheader(_scope);
	scp_nextbunch(_scope, _output, header.len);

	/* For some reason, the scope sends a newline-terminated string */
	_output[header.len - 1] = 0;
}
/* Set trigger delay */
void scp_settrdl(scp_t *_scope, const char *_dl)
{
	char query[BUFSIZE];

	/* Set value to scope */
	snprintf(query, BUFSIZE, "trdl %s", _dl);
	scp_query(_scope, query);

	#if DEBUG
	printf("trdl -- queried the following: \"%s\"\n", query);
	#endif
}
#endif

/* Writing scope routine */
void *scp_write(scp_t *_s)
{
	scp_cmd *cmd;

	if (!_s) {
		return NULL;
	}

	while (!_s->writekill) {
		/* Wait til scope has finished digested before feeding it
		with another bitball */
		pthread_mutex_lock(&_s->mtxwrite);
		while (!_s->digested || _s->cmdc < 1) {
			pthread_cond_wait(&_s->cndwrite, &_s->mtxwrite);
		}
		pthread_mutex_unlock(&_s->mtxwrite);

		/* Get another bitball from the plate */
		cmd = scp_cmd_get(_s);

		/* Feed scope with bitball */
		scp_query(_s, cmd->query);

		/* Dump command at once if it doesn't expect any reply, flag scope
		as digesting otherwise */
		if (!cmd->hdlr) {
			scp_cmd_pop(_s);
		} else {
			/* Bon appetit */
			_s->digested = 0;
		}
	}

	return NULL;
}

/* Reading thread routine (takes the scope as argument) */
void *scp_read(scp_t *_s)
{
	/* Header variables */
	header h;						/* Working header */
	int rdc = 0;					/* Read character count */
	/* Data variables */
	char strdata[BUNCHSIZE] = "";	/* Byte-wise data buffer */
	char dustbin[DUSTHEAP];			/* Unwanted bytes coming from the scope */
	int leftover;
	/* Other variables */
	scp_cmd *cmd;

	while (!_s->readkill) {
		/* Block until you read a header */
		for (rdc = 0; rdc < 8;) {
			rdc += read(_s->sockfd, (char *) &h + rdc, sizeof(header) - rdc);
		}

		/* Length */
		h.len = ntohl(h.len);

		/* Collect data */
		for (rdc = 0; rdc < h.len && rdc < BUNCHSIZE;) {
			rdc += read(_s->sockfd,
						strdata + rdc,
						h.len - rdc < BUNCHSIZE - rdc ?
							h.len - rdc : BUNCHSIZE - rdc);
		}

		/* Too much data for the buffer? */
		leftover = h.len - rdc;
		if (leftover > 0) {
			fprintf(stderr, "Warning: dropped data because buffer too small");
		}

		/* Ditch whatever remains couldn't fit in the buffer */
		for (rdc = 0; rdc < leftover;) {
			/* It would seem it's not possible to read characters 
			from a socket and not do anything with them. */
			rdc += read(_s->sockfd,
						dustbin,
						leftover - rdc < DUSTHEAP ? leftover - rdc : DUSTHEAP);
		}
		
		/* Service request or regular command? */
		if (h.dataflag == HDR_SRERCHFLAG) {
			/* srech = 1; */	/* Do we have to pop cmd, shortly? */
			_s->trg(strdata, h.len, _s->trgdst);
		} else {
			cmd = scp_cmd_get(_s);
			if (h.dataflag == (HDR_EOIFLAG | HDR_DATAFLAG)) {
				cmd->hdlr(1, strdata, h.len, cmd->dst);
				scp_cmd_pop(_s);

				/* There's at most one command left here
				and we're going to take care of it right
				away. Therefore, scope should be done
				digesting and it's time for another bitball. */
				pthread_mutex_lock(&_s->mtxwrite);
				_s->digested = 1;
				pthread_cond_signal(&_s->cndwrite);
				pthread_mutex_unlock(&_s->mtxwrite);
			} else {
				cmd->hdlr(0, strdata, h.len, cmd->dst);
			}
		}
	}
	return NULL;
}

/* Connect to scope and returns pointer to it upon success, NULL otherwise */
scp_t *scp_new(const char *_addr,
			   const char *_port,
			   void (*_trg)(char *_data, int _len, void *_dst),
			   void *_trgdst)
{
	int rc;
	struct addrinfo hints;
	struct addrinfo *info;
	scp_t *s;		/* To be returned */
	
	/* Make room for the scope to return */
	s = (scp_t *) malloc(sizeof(scp_t));

	/* Set trigger handler */
	s->trg = _trg;
	s->trgdst = _trgdst;

	/* Get address info */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rc = getaddrinfo(_addr, _port, &hints, &info)) != 0) {
		#if DEBUG
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
		#endif
		return NULL;
	}

	/* Get socket */
	s->sockfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
	if (s->sockfd == -1) {
		#if DEBUG
		fprintf(stderr, "socket: %s\n", strerror(errno));
		#endif
		return NULL;
	}

	/* Connect */
	rc = connect(s->sockfd, info->ai_addr, info->ai_addrlen);
	if (rc == -1) {
		#if DEBUG
		fprintf(stderr, "connect: %s\n", strerror(errno));
		#endif
	}

	/* Clean up */
	freeaddrinfo(info);

	/* Mux stuff to play with timeout */
	FD_ZERO(&s->fds);
	FD_SET(s->sockfd, &s->fds);
	s->tmt.tv_sec = SCP_TMT;
	s->tmt.tv_usec = 0;

	/* Check status and return */
	if (rc == 0) {
		/* Initialize mutex */
		pthread_mutex_init(&s->mutex, NULL);

		/* Initialize quit condition */
		pthread_mutex_init(&s->mtxquit, NULL);
		pthread_cond_init(&s->cndquit, NULL);

		/* Initialize command readout condition */
		pthread_mutex_init(&s->mtxread, NULL);
		pthread_cond_init(&s->cndread, NULL);

		/* Initialize command write condition */
		pthread_mutex_init(&s->mtxwrite, NULL);
		pthread_cond_init(&s->cndwrite, NULL);
		
		/* Say there's no command queued */
		s->cmdc = 0;
		s->cmd_first = NULL;
		s->cmd_last = NULL;

		/* Get ready for reading thread */
		s->readkill = 0;
		s->writekill = 0;

		/* Start on an empty stomach */
		s->digested = 1;

		/* Start reading and writing threads */
		pthread_create(&s->thdread, NULL, (void *(*)(void *)) scp_read, s);
		pthread_create(&s->thdwrite, NULL, (void *(*)(void *)) scp_write, s);

		return s;
	}

	free(s);
	return NULL;
}

double scp_fto125(double _f)
{
	char buf[BUFSIZE];
	int first;
	int i;

	/* Make string from float */
	snprintf(buf, BUFSIZE, "0%.10f", _f);

	/* Get biggest significant digit */
	for (i = 0; i < 16 && (buf[i] == '0' || buf[i] == '.'); i++);
	first = buf[i] - '0';

	switch (first) {
		case 3:
			if (i + 1 < 16 && buf[i + 1] - '0' < 5) {
				buf[i] = '2';
			} else {
				buf[i] = '5';
			}
			break;
		case 4:
		case 6:
			buf[i] = '5';
			break;
		case 7:
			if (i + 1 < 16 && buf[i + 1] - '0' < 5) {
				buf[i] = '2';
			} else {
				buf[i] = '5';
			}
			break;
		case 8:
		case 9:
			buf[i - 1] = '1';
			buf[i] = '0';
			break;
	}

	/* Dump other digits */
	buf[i + 1] = 0;

	return atof(buf);
}

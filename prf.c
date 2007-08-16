#include "prf.h"

#include <string.h>
#define BUFSIZE 1024

int prf_read(prfs_t *_prfs)
{
	/* State Machine Variables */
	char body[PRF_LEN];
	/* IKV stands for inter-key/value - space between a key and a value;
	PV stands for post-value - space after value which may be
	meaningless/-ful */
	enum { LINE, SETTING, COMMENT, KEY, VALUE, IKV, PV };
	int i = 0;
	int state = LINE;
	char key[16], val[16], cmt[80];	/* Working variables */
	int key_i;
	int val_i;
	int cmt_i;

	/* File Variables */
	char *home;
	char path[PRF_PATHLEN];
	FILE *fp;
	int readc;

	/* Init values */
	_prfs->prfc = 0;
	_prfs->cmtc = 0;
	_prfs->linec = 0;

	/* Compute path */
	home = getenv("HOME");
	if (!home) {
		return -1;
	}
	snprintf(path, PRF_PATHLEN, "%s/%s", home, PRF_FILE);

	/* Open, read and close file */
	fp = fopen(path, "r");
	if (!fp) {
		return -1;
	}
	readc = fread(body, 1, PRF_LEN, fp);
	fclose(fp);

	while (i < readc && _prfs->prfc + _prfs->cmtc <= PRF_MAXLEN) {
		switch (state) {
			case LINE:
				if (body[i] == '#') {
					state = COMMENT;
					cmt_i = 0;
					i++;
				} else if (body[i] == '\n') {
					i++;
					_prfs->linec++;
				} else {
					state = SETTING;
				}
				break;
			case COMMENT:
				if (body[i] == '\n') {
					state = LINE;
					cmt[cmt_i] = 0;
					i++;
					cmt_i = 0;

					/* To struct */
					strncpy(_prfs->cmts[_prfs->cmtc].data, cmt, PRF_MAXLEN);
					_prfs->cmts[_prfs->cmtc].lineno = _prfs->linec;
					_prfs->cmtc++;

					_prfs->linec++;
				} else {
					cmt[cmt_i] = body[i];
					i++;
					cmt_i++;
				}
				break;
			case SETTING:
				if (body[i] == '\n') {
					state = LINE;
					i++;
					_prfs->linec++;
				} else if (body[i] == ' ' || body[i] == '\t') {
					i++;
				} else if (body[i] == '#') {
					state = COMMENT;
					i++;
				} else {
					state = KEY;

					/* Reset key before we get a new one */
					key_i = 0;
					key[0] = 0;
				}
				break;
			case KEY:
				if (body[i] == ' ' || body[i] == '\t') {
					state = IKV;

					/* Reset value before we get a new one */
					val_i = 0;
					val[0] = 0;

					/* Tie up string */
					key[key_i] = 0;

					/* To struct */
					strncpy(_prfs->prfs[_prfs->prfc].key, key, PRF_MAXLEN);

					i++;
				} else if (body[i] == '\n') {
					state = LINE;
					key[key_i] = 0;
					#if DEBUG
					printf("Syntax error in config file, line %d: ", _prfs->linec);
					printf("Missing value for key %s\n", key);
					#endif

					i++;
					_prfs->linec++;
				} else if (body[i] == '#') {
					state = COMMENT;
					key[key_i] = 0;

					#if DEBUG
					printf("Syntax error in config file, line %d: ", _prfs->linec);
					printf("Missing value for key %s\n", key);
					#endif
				} else {
					key[key_i] = body[i];
					i++;
					key_i++;
				}
				break;
			case IKV:
				if (body[i] == ' ' || body[i] == '\t') {
					i++;
				} else if (body[i] == '\n') {
					state = LINE;

					#if DEBUG
					printf("Syntax error in config file, line %d: ", _prfs->linec);
					printf("Missing value for key %s\n", key);
					#endif

					i++;
					_prfs->linec++;
				} else if (body[i] == '#') {
					state = COMMENT;
					#if DEBUG
					printf("Syntax error in config file, line %d: ", _prfs->linec);
					printf("Missing value for key %s\n", key);
					#endif
					i++;
				} else {
					state = VALUE;
				}
				break;
			case VALUE:
				if (body[i] == '\n') {
					state = LINE;

					/* Tie up string */
					val[val_i] = 0;

					/* To struct */
					strncpy(_prfs->prfs[_prfs->prfc].val, val, PRF_MAXLEN);
					_prfs->prfs[_prfs->prfc].lineno = _prfs->linec;
					_prfs->prfc++;

					i++;
					_prfs->linec++;
				} else if (body[i] == '#') {
					state = COMMENT;
					val[val_i] = 0;
					i++;

					/* To struct */
					strncpy(_prfs->prfs[_prfs->prfc].val, val, PRF_MAXLEN);
					_prfs->prfs[_prfs->prfc].lineno = _prfs->linec;
					_prfs->prfc++;
				} else if (body[i] == ' ' || body[i] == '\t') {
					state = PV;
					i++;
				} else {
					val[val_i] = body[i];
					i++;
					val_i++;
				}
				break;
			case PV:
				if (body[i] == '\n') {
					state = VALUE;
				} else if (body[i] == ' ' || body[i] == '\t') {
					state = VALUE;
				} else if (body[i] == '#') {
					state = VALUE;
				} else {
					val[val_i] = ' ';
					i++;
					val_i++;
					state = VALUE;
				}
				break;
		}
	}
	#if DEBUG
	printf("there are %d lines\n", _prfs->linec);
	#endif
	return 0;
}

/* Cheerfully returns 0 and on success and copies appropriate value to
beforehand-allocated _val, angrily returns -1 on failure */
int prf_get(const prfs_t *_prfs, const char *_key, char *_val)
{
	int i;

	/* Look for key and set value */ 
	for (i = 0; i < _prfs->prfc; i++) {
		if (strcmp(_key, _prfs->prfs[i].key) == 0) {
			strcpy(_val, _prfs->prfs[i].val);
			return 0;
		}
	}

	return -1;
}

/* Sets value given passed key. Adds the pair if the key's not found.
Returns -1 on failure (i.e. not space left), 0 otherwise. */
int prf_set(prfs_t *_prfs, const char *_key, const char *_val)
{
	int i;

	/* Look for an existing key */
	for (i = 0; i < _prfs->prfc; i++) {
		if (strcmp(_key, _prfs->prfs[i].key) == 0) {
			strcpy(_prfs->prfs[i].val, _val);
			return;
		}
	}

	/* Create new kay/value pair if key's not found */
	if (_prfs->prfc + _prfs->cmtc + 1 < PRF_MAXLEN) {
		strncpy(_prfs->prfs[_prfs->prfc].key, _key, PRF_MAXLEN);
		strncpy(_prfs->prfs[_prfs->prfc].val, _val, PRF_MAXLEN);
		_prfs->prfs[_prfs->prfc].lineno = _prfs->linec;
		_prfs->prfc++;
		_prfs->linec++;
	} else {
		return -1;
	}
}

void prf_write(prfs_t *_prfs)
{
	int lineno = 0, prf_i = 0, cmt_i = 0;
	FILE *fp;
	char buf[BUFSIZE];

	snprintf(buf, BUFSIZE, "%s/.wavejetrc", getenv("HOME"));
	fp = fopen(buf, "w");

	while (prf_i < _prfs->prfc || cmt_i < _prfs->cmtc) {
		if (prf_i < _prfs->prfc && _prfs->prfs[prf_i].lineno == lineno) {
			fprintf(fp, "%s %s ",
					_prfs->prfs[prf_i].key,
					_prfs->prfs[prf_i].val);
			prf_i++;
		}
		if (cmt_i < _prfs->cmtc && _prfs->cmts[cmt_i].lineno == lineno) {
			fprintf(fp, "#%s", _prfs->cmts[cmt_i].data);
			cmt_i++;
		}
		fprintf(fp, "\n");
		lineno++;
	}
	fclose(fp);
}

#if 0
int main(int _argc, char *_argv[])
{
	FILE *fp;
	char buf[4096];
	int len;
	prfs_t prfs;
	int i;

	if (_argc < 2) {
		printf("missing arguments\n");
		exit(1);
	}

	/* Read file */
	fp = fopen(_argv[1], "r");
	if (fp == NULL) {
		perror("fopen");
	}
	len = fread(buf, 1, 4096, fp);

	prf_parse(buf, len, &prfs);

	/* Print out all comments */
	printf("\n*** print out comments\n");
	for (i = 0; i < prfs.cmtc; i++) {
		printf("line %d: '%s'\n", prfs.cmts[i].lineno, prfs.cmts[i].data);
	}

	/* Print out all prefs */
	printf("\n*** print out prefs\n");
	for (i = 0; i < prfs.prfc; i++) {
		printf("line %d: ('%s','%s')\n",
			   prfs.prfs[i].lineno, 
			   prfs.prfs[i].key,
			   prfs.prfs[i].val);
	}

	/* Write to file */
	printf("\n*** write to file\n");
	prf_write(&prfs);

	return 0;
}
#endif

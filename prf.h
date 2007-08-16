#ifndef WJ_PRF
#define WJ_PRF

#include <stdio.h>
#include <stdlib.h>

/* Number of bytes to read from the RC file */
#define PRF_LEN 16384

/* Strings and number of lines can't be bigger than this value */
#define PRF_MAXLEN 128

/* RC File */
#define PRF_FILE ".wavejetrc"

/* RC Path Length */
#define PRF_PATHLEN 1024

typedef struct {
	unsigned lineno;
	char key[PRF_MAXLEN];
	char val[PRF_MAXLEN];
} prf_t;

typedef struct {
	unsigned lineno;
	char data[PRF_MAXLEN];
} cmt_t;

typedef struct {
	prf_t prfs[PRF_MAXLEN];
	cmt_t cmts[PRF_MAXLEN];
	int prfc;
	int cmtc;
	int linec;
} prfs_t;

/* Reads and parses the contents of an RC file and sets the key/value pairs
found in _prfs).

Things that are collected are:
- key/value pairs
- block and inline comments
- empty lines (nothing is stored, obviously, only the line count incremented

Things that are lost are:
- leading spaces (and indentation goes with it)
- duplicate spaces between key, values and inline comments

Returns 0 on success, -1 on failure.
*/
int prf_read(prfs_t *_prfs);

/* Sets user-allocated _val to _key's value when found in _prfs. The string
held in _val will always be reset before hand, so if _key is not found it
will be left out as an empty string. */
int prf_get(const prfs_t *_prfs, const char *_key, char *_val);

int prf_set(prfs_t *_prfs, const char *_key, const char *_val);
void prf_write(prfs_t *_prfs);

#endif 

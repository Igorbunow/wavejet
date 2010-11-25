#ifndef WJ_SCP
#define WJ_SCP

#include <netdb.h>

#define SCP_TMT 10	/* Timeout in seconds */

#define SCP_DTPOINTS_MAX 500
#define SCP_DTPOINTS "dtpoints 1000"
/* #define SCP_DTPOINTS "dtpoints 10" */
#define SCP_MAXVAL 32768	/* Max value a scope can give */
#define SCP_CHC 4
#define SCP_VDIVC 8			/* Number of vertical divisions */

typedef struct scp_cmd {
	char query[32];
	void (*hdlr)(int _last, char *_data, int _len, void *_dst);
	void *dst;				/* Where the data goes to (e.g. a widget) */
	struct scp_cmd *next;
} scp_cmd;

typedef struct {
	int sockfd;
	struct sockaddr_in sockaddr;
	fd_set fds;	/* Socket set for use with select */
	struct timeval tmt;
	int readkill, writekill;
	int digested;

	pthread_t thdread, thdwrite;
	pthread_mutex_t mutex;
	pthread_mutex_t mtxquit;
	pthread_cond_t cndquit;

	pthread_mutex_t mtxread, mtxwrite;
	pthread_cond_t cndread, cndwrite;

	/* Command queue */
	scp_cmd *cmd_first;
	scp_cmd *cmd_last;
	int cmdc;

	/* Trigger function */
	void (*trg)(char *_data, int _len, void *_dst);
	void *trgdst;				/* Where the data goes to (e.g. a widget) */
} scp_t;

typedef struct {
	unsigned char dataflag;
	unsigned char reserved[3];
	size_t len;
} header;

/* Make a scope connection.
 * The scope panel is not locked at this time but it will be as
 * soon as you submit a command.
 */
scp_t *scp_new(const char *_addr,
			   const char *_port,
			   void (*_trg)(char *_data, int _len, void *_dst),
			   void *_trgdst);

/* Command queue functions */
void scp_cmd_push(scp_t *_scope,
				  const char *_query,
				  void (*_hdlr)(int _last, char *_data, int _len, void *_dst),
				  void *_dst);
scp_cmd scp_cmd_pop(scp_t *_scope);

/* Remove a scope connection */
int scp_destroy(scp_t *_scope);

/* Send a query and ask for confirmation */
void scp_query(scp_t *_scope, const char *_command);

/* Toolchest */
double scp_fto125(double _f);

/* Attic */
#if DEV
/* Higher Level Commands */
void scp_dtwave(scp_t *_scope, int _chl, int _dtpoints, int *_output);

/* DTPOINTS */
int scp_getdtpoints(scp_t *_scope);
void scp_setdtpoints(scp_t *_scope, int _dtpoints);

/* Time division */
void scp_gettdiv(scp_t *_scope, char *_output);
void scp_settdiv(scp_t *_scope, const char *_value);

/* Vertical sensitivity */
void scp_getvdiv(scp_t *_scope, int _ch, char *_output);
void scp_setvdiv(scp_t *_scope, int _ch, const char *_value);

/* Trigger source */
void scp_gettsrc(scp_t *_scope, char *_output);
void scp_settsrc(scp_t *_scope, const char *_value);

/* Trigger coupling */
void scp_gettcpl(scp_t *_scope, char *_output);
void scp_settcpl(scp_t *_scope, const char *_value);

/* Trigger level */
void scp_gettlvl(scp_t *_scope, char *_output);
void scp_settlvl(scp_t *_scope, const char *_value);

/* Trigger mode */
void scp_gettrmd(scp_t *_scope, char *_output);
void scp_settrmd(scp_t *_scope, const char *_md);

/* Trace */
void scp_gettra(scp_t *_scope, int _ch, char *_output);
void scp_settra(scp_t *_scope, int _ch, const char *_onoff);

/* Trigger delay */
void scp_gettrdl(scp_t *_scope, char *_output);
void scp_settrdl(scp_t *_scope, const char *_dl);

/* Trigger Event Status register */
/* FIXME Don't exactly know what it's for */
void scp_gettesr(scp_t *_scope, char *_output);

/* Screen shot */
/* FIXME Doesn't work well yet */
void scp_gettscrn(scp_t *_scope, FILE *_fd);

/* Lower Level Tools */
header scp_readheader(scp_t *_scope);
void scp_nextbunch(scp_t *scope, char *output, int len);
#endif


#endif

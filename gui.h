#ifndef WJ_GUI
#define WJ_GUI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* memset */
#include <gtk/gtk.h>
#include <pthread.h>

#include "scp.h"
#include "cbk.h"
#include "png.h"
#include "prf.h"

#define GUI_DESC_ONLINE "Online"
#define GUI_DESC_OFFLINE "Offline"
#define GUI_DESC_CONNECTING "Connecting..."
#define GUI_DESC_FAILURE "Couldn't connect to scope"

#define GUI_TSCRNW 521			/* Screenshot width in pixels */
#define GUI_TSCRNH 421			/* Screenshot height in pixels */
#define GUI_TSCRNS 1228800		/* Size computed from TSCRNWxTSCRNH*32bits */
#define GUI_TSCRN_HMIDDLE 260
#define GUI_TSCRN_VMIDDLE 207
#define GUI_TSCRN_TOP 18
#define GUI_TSCRN_BOTTOM 398
#define GUI_TSCRN_LEFT 9
#define GUI_TSCRN_RIGHT 509
#define GUI_TSCRN_ALPHA .5
#define GUI_TSCRN_MOREALPHA .75

#define GUI_TDIVC 32
#define GUI_VDIVC 12
#define GUI_TSRCC 6
#define GUI_TCPLC 4
#define GUI_UNIT_STRLEN 16

#define GUI_TITLE "WaveJet 324 Control"
#define GUI_WIDTH 640
#define GUI_HEIGHT 480

#define GUI_PLOT_PAD 10
#define GUI_PLOT_SUBDIVLEN 10
#define GUI_PLOT_TDIVC 10
#define GUI_PLOT_VDIVC 8

#define GUI_PLOT_PNTCMAX 2000

extern char g_strvdivs[GUI_VDIVC][GUI_UNIT_STRLEN];
extern char g_strtdivs[GUI_TDIVC][GUI_UNIT_STRLEN];
extern char g_strtsrcs[GUI_TSRCC][GUI_UNIT_STRLEN];
extern char g_strtcpls[GUI_TCPLC][GUI_UNIT_STRLEN];

typedef struct {
	short x, y;
} gui_pnt_t;

typedef struct {
	/* FIXME Not very nice to have the scope here */
	scp_t *scp;
	int id;
	GtkWidget *tglenbl;
	GtkWidget *optvdiv;
	GtkWidget *lblofst;
	GtkWidget *btnupofst, *btndnofst;
} gui_chl_t;

typedef struct {
	int cbklocked;
	int loopkill;

	/* Threads */
	pthread_t thdloopplot;
	pthread_t thdconnect;
	pthread_t thdread;

	/* Thread loopplot block */
	pthread_mutex_t mtxloopplot;
	pthread_cond_t cndloopplot;

	/* Windows */
	GtkWidget *winmain;
	GtkWidget *winprfs;
	GtkWidget *winplot;

	/* Windows -- For future development */
	#if DEV
	GtkWidget *wininfo;
	GtkWidget *winscrn;
	#endif
	
	/* Menubar -- Future Dev */
	GtkWidget *itmplot;
	GtkWidget *itmscrn;
	GtkWidget *itminfo;

	/* Menubar */
	GtkWidget *itmconnect;

	/* Status bar */
	GtkWidget *statusbar;
	unsigned ctxid;

	/* Controls */
	GtkWidget *controls;
	GtkWidget *cbotdiv;
	GtkWidget *opttdiv;
	gui_chl_t chls[SCP_CHC];
	GtkWidget *rdoauto, *rdonormal, *rdosingle, *rdostop;
	GdkPixbuf *pbfledon, *pbfledoff;
	GtkWidget *imgled;
	GtkWidget *opttsrc;
	GtkWidget *etytsrc;
	GtkWidget *opttcpl;
	GtkWidget *etytcpl;
	GtkWidget *lbltlvl;
	GtkWidget *scltlvl;
	GtkWidget *lbltrdl;

	/* Plot */
	#if DEV
	GtkWidget *gnuplot;
	FILE *pp;	/* Pipe pointer */
	char plotdata[SCP_DTPOINTS_MAX];
	char plotlen;
	#endif

	/* All Images */
	GdkVisual *visual;

	/* PNG Plot */
	#if DEV
	png_structp pngptr;
	png_infop pnginfo;
	GdkImage *gdkimgscrn;
	GtkWidget *evtboxscrn;
	char pixels[GUI_TSCRNS];
	GtkWidget *btnrfstscrn;
	GtkWidget *stsbartscrn;
	unsigned ctxidtscrn;
	#endif

	/* New Plot */
	GtkWidget *evtboxplot;
	GdkImage *imgplot;
	GdkPixmap *pmpdtwave;
	GdkGC *gc;
	GtkWidget *stsbardtwave;
	unsigned ctxiddtwave;
	int data[GUI_PLOT_PNTCMAX];
	GtkWidget *alignment;
	gui_pnt_t tras[SCP_CHC][GUI_TSCRNW * GUI_TSCRNH];
	long clrs[SCP_CHC];	/* Yellow, Pink, Cyan, Green */

	/* Plot Interactivity */
	int ox, oy;	/* Origin */
	int tra;	/* Caught trace ID */
	int x, y;	/* Mouse position */
	unsigned char r, g, b;	/* Selection colour */

	/* Prefs */
	prfs_t prfs;

	/* Prefs from command-line args */
	char addr[16];
	unsigned port;

	/* Prefs Window */
	GtkWidget *etyaddr, *etyport;
	GtkWidget *imgaddr, *imgport;	/* Error images */

	/* Scope */
	scp_t *scp;
} gui_t;

gui_t *gui_new(void);

/* Missing math functions in math.h */
double round(double x);

/* Windows */
GtkWidget *gui_winmain_new(gui_t *_gui);
GtkWidget *gui_winprfs_new(gui_t *_gui);
GtkWidget *gui_winplot_new(gui_t *_gui);

/* Plot */
GtkWidget *gui_plot_new(gui_t *_gui);

/* GUI Build */
gui_t *gui_new(void);
void *gui_connect(void *_gui);
GtkWidget *gui_getmenubar(GtkWidget *window);

/* Threaded */
void *gui_loopplot(void *_p);

/* Reading thread handlers */
void gui_tdiv(int _islast, char *_data, int _len, void *_gui);
void gui_vdiv(int _islast, char *_data, int _len, void *_chl);
void gui_tra(int _islast, char *_data, int _len, void *_wgt);
void gui_tlvl(int _islast, char *_data, int _len, void *_gui);
void gui_trdl(int _islast, char *_data, int _len, void *_gui);
void gui_tscrn(int _islast, char *_data, int _len, void *_widget);
void gui_dtwave(int _last, char *_data, int _len, void *_gui);
void gui_ofst(int _last, char *_data, int _len, void *_chl);
void gui_tsrc(int _islast, char *_data, int _len, void *_gui);
void gui_tcpl(int _islast, char *_data, int _len, void *_gui);

/* Drawing routines */
void gui_drawgrid(gui_t *_gui);

/* RC */
int gui_winprfs_set(gui_t *_gui, const prfs_t *_prfs);

/* Functions for future (or past) development */
#ifdef DEV
/* Windows */
GtkWidget *gui_winscrn_new(gui_t *_gui);
GtkWidget *gui_wininfo_new(gui_t *_gui);

/* 
 * Simple minded, but much faster xwinid().
 * Written: Jeroen Belleman - September 2006
 */
unsigned ui_winid(char *s);

void gui_drawplot(gui_t *_gui);
void gui_plot_steal(GtkWidget *_gnuplot);

void gui_disconnect(gui_t *_gui);

/* PNG reading handler */
void gui_readpng(png_structp _pngstruct, png_bytep _data, png_size_t _len);
#endif

#endif

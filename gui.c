#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>     /* memset */
#include <gdk/gdkkeysyms.h>	/* Accelerator keys */
#include "gui.h"
 
/* Trigger/Plot Times */
#define GUI_TRCK 250000		/* Trigger Check Time in useconds (dft 250000) */
#define GUI_PLTM 1000000	/* Time between two plots in auto plot */

/* Other Stuff */
#define GUI_CONTEXTDESC "Context"
#define GUI_SIGNALC 4
#define BUFSIZE 4096
#define SMLBUFSIZE 128

#define LBLCONNECT "Connect to scope"
#define LBLDISCONNECT "Disconnect from scope"

extern int g_locked;
extern unsigned char g_data_ledon[];
extern unsigned char g_data_ledoff[];

char g_strvdivs[GUI_VDIVC][GUI_UNIT_STRLEN] = {
	"20.0 mV", "50.0 mV", "100 mV", "200 mV", "500 mV", "1.00 V", "2.00 V",
	"5.00 V", "10.0 V", "20.0 V", "50.0 V", "100 V"
};
char g_strtdivs[GUI_TDIVC][GUI_UNIT_STRLEN] = {
	"2.00 ns", "5.00 ns", "10.0 ns", "20.0 ns", "50.0 ns", "100 ns", "200 ns",
	"500 ns", "1.00 \316\274s", "2.00 \316\274s", "5.00 \316\274s",
	"10.0 \316\274s", "20.0 \316\274s", "50.0 \316\274s", "100 \316\274s",
	"200 \316\274s", "500 \316\274s", "1.00 ms", "2.00 ms", "5.00 ms",
	"10.0 ms", "20.0 ms", "50.0 ms", "100 ms", "200 ms", "500 ms", "1.00 s",
	"2.00 s", "5.00 s", "10.0 s", "50.0 s"
};
char g_strtsrcs[GUI_TSRCC][GUI_UNIT_STRLEN] = {
	"CH1", "CH2", "CH3", "CH4", "EXT", "EXT10"
};
char g_strtcpls[GUI_TCPLC][GUI_UNIT_STRLEN] = {
	"AC" , "DC" , "HF" , "LF"
};

gui_t *gui_new(char *_addr, char *_port)
{	
	int rc;
	gui_t *gui;

	/* Create structure */
	gui = malloc(sizeof(gui_t));

	/* Init a few values (not sure they're all needed) */
	gui->scp = NULL;
	gui->cbklocked = 0;

	/* Visual, for the images we'll draw later on */
	gui->visual = gdk_visual_get_system();

	/* Make main window  */
	gui->winmain = gui_winmain_new(gui);
	g_signal_connect(G_OBJECT(gui->winmain), "delete_event",
					 G_CALLBACK(cbk_delete), gui);
	gtk_window_set_title(GTK_WINDOW(gui->winmain), GUI_TITLE);
	gtk_widget_show(gui->winmain);

	/* Make info window */
	#if DEV
	gui->wininfo = gui_wininfo_new(gui);
	sprintf(buf, "%s - Info", GUI_TITLE);
	gtk_window_set_title(GTK_WINDOW(gui->wininfo), buf);
	g_signal_connect(G_OBJECT(gui->wininfo), "delete_event",
					 G_CALLBACK(cbk_delwininfo), gui);
	gtk_widget_hide(gui->wininfo);
	#endif

	/* Make pref window */
	gui->winprfs = gui_winprfs_new(gui);

	/* Make about window */
	gui->winabout = gui_winabout_new(gui);

	/* Create preferences (setting default parameters) */
	gui->prfs = prf_new();

	/* Read RC file */
	(int) prf_read(gui->prfs);

	/* Set parameters from runtime arguments */
	if (_addr) {
		prf_set(gui->prfs, "addr", _addr);
		if (_port) {
			prf_set(gui->prfs, "port", _port);
		}
	}

	/* Possibly open preference window */
	rc = gui_winprfs_set(gui, gui->prfs);
	if (rc != 0) {
		gtk_widget_show(gui->winprfs);
	}

	#if DEV
	/* Plot window */
	gui->winplot = gui_winplot_new(gui);
	gtk_widget_show(gui->winplot);
	sprintf(buf, "%s - Plot", GUI_TITLE);
	gtk_window_set_title(GTK_WINDOW(gui->winplot), buf);
	g_signal_connect(G_OBJECT(gui->winplot),
					 "delete_event",
					 G_CALLBACK(cbk_delwinplot),
					 gui);
	gtk_widget_show(gui->winplot);

	/* Screenshot window */
	gui->winscrn = gui_winscrn_new(gui);
	gtk_widget_show(gui->winscrn);
	sprintf(buf, "%s - Screenshot", GUI_TITLE);
	gtk_window_set_title(GTK_WINDOW(gui->winscrn), buf);
	g_signal_connect(G_OBJECT(gui->winscrn),
					 "delete_event",
					 G_CALLBACK(cbk_delwinscrn),
					 gui);
	gtk_widget_hide(gui->winscrn);
	#endif

	return gui;
}

/* Returns fully-populated menubar */
GtkWidget *gui_menubar_new(gui_t *_gui)
{
	/* Menu variables */
	GtkWidget *menubar;
	GtkWidget *mnufile;
	GtkWidget *itmfile;
	GtkWidget *itmprfs;
	GtkWidget *itmsep;
	GtkWidget *itmquit;

	GtkWidget *mnuhelp;
	GtkWidget *itmhelp;
	GtkWidget *itmabout;

	/* Accelerator variables */
	GtkAccelGroup *acc;

	/* Dev Variables */
	#if DEV
	GtkWidget *mnuscp;
	GtkWidget *itmscp;
	GtkWidget *mnuwindows;
	GtkWidget *itmwindows;
	GtkWidget *mnudev;
	GtkWidget *itmdev;
	GtkWidget *itmacn;
	#endif

	/* Set up accelerators */
	acc = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(_gui->winmain), acc);

	/* Create new menubar */
	menubar = gtk_menu_bar_new();

	/* Menus */
	itmfile = gtk_menu_item_new_with_label("File");
	mnufile = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itmfile), mnufile);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), itmfile);
	gtk_widget_show(itmfile);

	itmhelp = gtk_menu_item_new_with_label("Help");
	mnuhelp = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itmhelp), mnuhelp);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), itmhelp);
	gtk_widget_show(itmhelp);

	#if DEV
	itmscp = gtk_menu_item_new_with_label("Scope");
	mnuscp = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itmscp), mnuscp);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), itmscp);
	gtk_widget_show(itmscp);

	itmdev = gtk_menu_item_new_with_label("Dev");
	mnudev = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itmdev), mnudev);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), itmdev);
	gtk_widget_show(itmdev);

	itmwindows = gtk_menu_item_new_with_label("Windows");
	mnuwindows = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(itmwindows), mnuwindows);
	gtk_menu_bar_append(GTK_MENU_BAR(menubar), itmwindows);
	gtk_widget_show(itmwindows);
	#endif

	/* Menu items */
	_gui->itmconnect = gtk_menu_item_new_with_label(LBLCONNECT);
	g_signal_connect(G_OBJECT(_gui->itmconnect), "activate",
					 G_CALLBACK(cbk_connect), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnufile), _gui->itmconnect);
	gtk_widget_show(_gui->itmconnect);

	_gui->itmdisconnect = gtk_menu_item_new_with_label(LBLDISCONNECT);
	g_signal_connect(G_OBJECT(_gui->itmdisconnect), "activate",
					 G_CALLBACK(cbk_disconnect), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnufile), _gui->itmdisconnect);
	gtk_widget_show(_gui->itmdisconnect);

	itmprfs = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect(G_OBJECT(itmprfs), "activate",
					 G_CALLBACK(cbk_opnwinprfs), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnufile), itmprfs);
	gtk_widget_show(itmprfs);

	itmsep = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(mnufile), itmsep);
	gtk_widget_show(itmsep);

	itmquit = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	gtk_widget_add_accelerator(itmquit, "activate", acc, GDK_q,
							   GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	g_signal_connect(G_OBJECT(itmquit), "activate",
					 G_CALLBACK(cbk_quit), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnufile), itmquit);
	gtk_widget_show(itmquit);

	itmabout = gtk_image_menu_item_new_with_label("About");
	g_signal_connect(G_OBJECT(itmabout), "activate",
					 G_CALLBACK(cbk_about), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnuhelp), itmabout);
	gtk_widget_show(itmabout);

	#if DEV
	_gui->itminfo = gtk_check_menu_item_new_with_label("Info");
	g_signal_connect(G_OBJECT(_gui->itminfo), "activate",
					 G_CALLBACK(cbk_tglwininfo), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnuscp), _gui->itminfo);
	gtk_widget_show(_gui->itminfo);

	itmacn = gtk_menu_item_new_with_label("Action");
	g_signal_connect(G_OBJECT(itmacn), "activate",
					 G_CALLBACK(cbk_acnmnu), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnudev), itmacn);
	gtk_widget_show(itmacn);

	_gui->itmplot = gtk_check_menu_item_new_with_label("Plot");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_gui->itmplot), TRUE);
	g_signal_connect(G_OBJECT(_gui->itmplot), "activate",
					 G_CALLBACK(cbk_tglwinplot), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnuwindows), _gui->itmplot);
	gtk_widget_show(_gui->itmplot);

	_gui->itmscrn = gtk_check_menu_item_new_with_label("Screenshot");
	g_signal_connect(G_OBJECT(_gui->itmscrn), "activate",
					 G_CALLBACK(cbk_tglwinscrn), _gui);
	gtk_menu_shell_append(GTK_MENU_SHELL(mnuwindows), _gui->itmscrn);
	gtk_widget_show(_gui->itmscrn);
	#endif

	return menubar;
}

#if DEV
/* 
 * Simple minded, but much faster xwinid().
 * Written: Jeroen Belleman - September 2006
 */
unsigned gui_winid(char *s)
{
	FILE *pp = NULL;
	unsigned wid = 0;
	char command[100];

	sprintf(command, "xwininfo -name %s | grep id:\n", s);
	pp = popen(command, "r");
	fscanf(pp, "%*s %*s %*s %x", &wid);
	return wid;
}

/* Make new, empty gnuplot. Parameter _gnuplot has to be allocated --
gtk_socket_new() */
GtkWidget *gui_gnuplot_new(gui_t *_gui)
{
	/* Variables */
	/* int dtpoints; */
	GtkWidget *gnuplot;
	FILE *pp;

	/* Create widget */
	gnuplot = gtk_socket_new();
	_gui->gnuplot = gnuplot;

	/* Start gnuplot and send commands */
    pp = popen("gnuplot -geometry 1x1 -title gnuplot", "w");
	_gui->pp = pp;

	/* Setup gnuplot */
	fprintf(pp, "set grid\n");

	/* Read data from the following, manually entered one */
	fprintf(pp, "set format x \"\"\n");	/* Axis x values not shown */
	fprintf(pp, "set nokey\n");			/* Get rid of legend (i.e. key) */
	/* Ranges */
	/* Not yet connected, so let's assume we have MAX_DTPOINTS pts */
	fprintf(pp, "plot [0:%d] [-4:4] \"-\" with lines\n", SCP_DTPOINTS_MAX - 1);

	/* Send dummy data to gnuplot (i.e. y range) to have an empty plot */
	fprintf(pp, "10\n");

	/* gnuplot expects an 'e' when there's no more data */
	fprintf(pp, "e\n");

	/* Flush everything */
	fflush(pp);

	return gnuplot;
}

void gui_plot_steal(GtkWidget *_gnuplot)
{
	unsigned wid;

	/* Breathe before the plunge */
/*     usleep(1000000); */

	/* Steal socket (and give it to Dobby, he'll be free) */
    wid = gui_winid("gnuplot");
	if (wid != 0) {
		gtk_socket_add_id(GTK_SOCKET(_gnuplot), wid);
		gtk_socket_add_id(GTK_SOCKET(_gnuplot), wid);
	}
}
#endif

GtkWidget *gui_controls_new(gui_t *_gui)
{
	int i, j;					/* General purpose counters */
	char buf[SMLBUFSIZE];		/* General purpose buffer */
	GtkWidget *vboxcontrols;
	gui_chl_t *chl;

	/* tdiv variables */
	GtkWidget *frmtdiv;		/* Labeled group for tdiv */
	GtkWidget *cbotdiv;
	GtkWidget *mnutdiv;
	GtkWidget *itmtdiv;
	/* Channel variables */
	GtkWidget *frmchls[SCP_CHC];
	GtkWidget *hboxchls[SCP_CHC];
	GdkColor clryellow = { 0, 44461, 43690, 0 };
	GdkColor clrred = { 0, 44461, 0, 44461 };
	GdkColor clrblue = { 0, 0, 43690, 44461 };
	GdkColor clrgreen = { 0, 0, 43690, 0 };
	GdkColor *colours[SCP_CHC];
	GtkWidget *mnuvdivs[SCP_CHC];
	GtkWidget *hboxofsts[SCP_CHC];
	GtkWidget *arwupofsts[SCP_CHC], *arwdnofsts[SCP_CHC];
	GtkWidget *itmvdiv;
	/* Trigger variables */
	raw_t *rawled;			/* GP data for all leds */
	GtkWidget *imgled;
	GtkWidget *frmtrg;		/* Labeled group for trigger */
	GtkWidget *vboxtrg;
	GtkWidget *tbltrmd;
	GtkWidget *cbotsrc;
	GtkWidget *mnutsrc;
	GtkWidget *itmtsrc;
	GtkWidget *mnutcpl;
	GtkWidget *itmtcpl;
	GtkWidget *cbotcpl;
	GtkWidget *hboxtrdl;
	GtkWidget *btnleft;
	GtkWidget *imgleft;
	GtkWidget *arwleft;
	GtkWidget *btnright;
	GtkWidget *imgright;
	GtkWidget *arwright;
	GtkWidget *lbltrdl;

	/* Convenient Variables */
	GtkWidget *tglenbl;

	/* Signal Combo Box Lists */
	GList *lstdivs = NULL, *lsvdivs = NULL;
	/* Trigger Combo Box Lists */
	GList *lstsrcs = NULL, *lstcpls = NULL;

	/* Colours */
	colours[0] = &clryellow;
	colours[1] = &clrred;
	colours[2] = &clrblue;
	colours[3] = &clrgreen;

	/* Create control vbox */
	vboxcontrols = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(vboxcontrols), 10);
	gtk_widget_show(vboxcontrols);

	/* Prepare combo boxes */
	for (i = 0; i < GUI_VDIVC; i++) {
		lsvdivs = g_list_append(lsvdivs, g_strvdivs[i]);
	}
	for (i = 0; i < GUI_TDIVC; i++) {
		lstdivs = g_list_append(lstdivs, g_strtdivs[i]);
	}
	for (i = 0; i < GUI_TSRCC; i++) {
		lstsrcs = g_list_append(lstsrcs, g_strtsrcs[i]);
	}
	for (i = 0; i < GUI_TCPLC; i++) {
		lstcpls = g_list_append(lstcpls, g_strtcpls[i]);
	}

	/* tdiv */
	/* Make a frame (i.e. labeled group) */
	frmtdiv = gtk_frame_new("Time/div");
	gtk_box_pack_start(GTK_BOX(vboxcontrols), frmtdiv, FALSE, FALSE, 0);
	gtk_widget_show(frmtdiv);
	/* tdiv combo */
	mnutdiv = gtk_menu_new();
	/* Intended memory leak here */
	for (i = 0; i < GUI_TDIVC; i++) {
		itmtdiv = gtk_menu_item_new_with_label(g_strtdivs[i]);
		gtk_menu_shell_append(GTK_MENU_SHELL(mnutdiv), itmtdiv);
		gtk_widget_show(itmtdiv);
	}
	_gui->opttdiv = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(_gui->opttdiv), mnutdiv);
	gtk_widget_set_name(_gui->opttdiv, "opttdiv");
	g_signal_connect(G_OBJECT(_gui->opttdiv), "changed",
					 G_CALLBACK(cbk_optacn), _gui);
	gtk_container_add(GTK_CONTAINER(frmtdiv), _gui->opttdiv);
	gtk_widget_show(_gui->opttdiv);

	cbotdiv = gtk_combo_new();
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cbotdiv)->entry), FALSE);
	gtk_combo_set_popdown_strings(GTK_COMBO(cbotdiv), lstdivs);
	gtk_widget_set_name(GTK_COMBO(cbotdiv)->entry, "etytdiv");
	gtk_widget_show(cbotdiv);
	/* For later reference */
	_gui->cbotdiv = cbotdiv;

	/* For each four channels */
	for (i = 0; i < SCP_CHC; i++) {
		chl = &_gui->chls[i];
		chl->id = i + 1;

		/* Make a frame (i.e. labeled group) */
		snprintf(buf, SMLBUFSIZE, "Channel %d", i + 1);
		frmchls[i] = gtk_frame_new(buf);
		gtk_box_pack_start(GTK_BOX(vboxcontrols), frmchls[i], FALSE, FALSE, 0);
		gtk_widget_show(frmchls[i]);
		/* Channel HBox */
		hboxchls[i] = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(frmchls[i]), hboxchls[i]);
		gtk_widget_show(hboxchls[i]);
		/* Toggle Button */
		_gui->chls[i].tglenbl = gtk_toggle_button_new_with_label("    ");
		tglenbl = _gui->chls[i].tglenbl;
		gtk_widget_modify_bg(tglenbl, GTK_STATE_PRELIGHT, colours[i]);
		gtk_widget_modify_bg(tglenbl, GTK_STATE_ACTIVE, colours[i]);
		snprintf(buf, SMLBUFSIZE, "tglbtn%d", i + 1);
		gtk_widget_set_name(tglenbl, buf);
		g_signal_connect(G_OBJECT(GTK_TOGGLE_BUTTON(tglenbl)),
						 "toggled",
						 G_CALLBACK(cbk_tglacn), _gui);
		gtk_box_pack_start(GTK_BOX(hboxchls[i]), tglenbl, FALSE, FALSE, 0);
		gtk_widget_show(tglenbl);
		/* vdiv Combo Boxes */
		mnuvdivs[i] = gtk_menu_new();
		/* Intended memory leak here */
		for (j = 0; j < GUI_VDIVC; j++) {
			itmvdiv = gtk_menu_item_new_with_label(g_strvdivs[j]);
 			gtk_menu_shell_append(GTK_MENU_SHELL(mnuvdivs[i]), itmvdiv);
			gtk_widget_show(itmvdiv);
		}
		chl->optvdiv = gtk_option_menu_new();
		gtk_option_menu_set_menu(GTK_OPTION_MENU(chl->optvdiv), mnuvdivs[i]);
		snprintf(buf, SMLBUFSIZE, "optvdiv%d", i + 1);
		gtk_widget_set_name(chl->optvdiv, buf);
		g_signal_connect(G_OBJECT(chl->optvdiv), "changed",
						 G_CALLBACK(cbk_optacn), _gui);
		gtk_box_pack_start(GTK_BOX(hboxchls[i]),
						   chl->optvdiv,
						   TRUE, TRUE, 0);
		gtk_widget_show(chl->optvdiv);

		/* Offset Label */
		chl->lblofst = gtk_label_new(NULL);
		gtk_misc_set_alignment(GTK_MISC(chl->lblofst), 0., .5);
		gtk_widget_set_size_request(chl->lblofst, 50, -1);
		gtk_box_pack_start(GTK_BOX(hboxchls[i]), chl->lblofst, FALSE, FALSE, 5);
		gtk_widget_show(chl->lblofst);
		/* Offset VBox */
		hboxofsts[i] = gtk_vbox_new(TRUE, 0);
		gtk_box_pack_start(GTK_BOX(hboxchls[i]), hboxofsts[i], FALSE, FALSE, 0);
		gtk_widget_show(hboxofsts[i]);
		/* Offset Buttons */
		chl->btnupofst = gtk_button_new();
		snprintf(buf, SMLBUFSIZE, "btnupofst%d\n", i);
		gtk_widget_set_name(chl->btnupofst, buf);
		gtk_widget_set_size_request(chl->btnupofst, 20, 20);
		g_signal_connect(G_OBJECT(chl->btnupofst), "button-release-event",
						 G_CALLBACK(cbk_btnrlsofst), chl);
		gtk_box_pack_start(GTK_BOX(hboxofsts[i]), chl->btnupofst,
						   FALSE, FALSE, 0);
		gtk_widget_show(chl->btnupofst);
		chl->btndnofst = gtk_button_new();
		snprintf(buf, SMLBUFSIZE, "btndnofst%d\n", i);
		gtk_widget_set_name(chl->btndnofst, buf);
		gtk_widget_set_size_request(chl->btndnofst, 20, 20);
		g_signal_connect(G_OBJECT(chl->btndnofst), "button-release-event",
						 G_CALLBACK(cbk_btnrlsofst), chl);
		gtk_box_pack_start(GTK_BOX(hboxofsts[i]), chl->btndnofst,
						   FALSE, FALSE, 0);
		gtk_widget_show(chl->btndnofst);
		/* Offset Arrows */
		arwupofsts[i] = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_NONE);
		gtk_container_add(GTK_CONTAINER(chl->btnupofst), arwupofsts[i]);
		gtk_widget_show(arwupofsts[i]);
		arwdnofsts[i] = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
		gtk_container_add(GTK_CONTAINER(chl->btndnofst), arwdnofsts[i]);
		gtk_widget_show(arwdnofsts[i]);
	}

	/* Trigger Control Frame */
	frmtrg = gtk_frame_new("Trigger");
	gtk_box_pack_start(GTK_BOX(vboxcontrols), frmtrg, FALSE, FALSE, 0);
	gtk_widget_show(frmtrg);
	/* VBox */
	vboxtrg = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(frmtrg), vboxtrg);
	gtk_widget_show(vboxtrg);
	/* Mode */
	tbltrmd = gtk_table_new(2, 2, TRUE);
	gtk_box_pack_start(GTK_BOX(vboxtrg), tbltrmd, FALSE, FALSE, 0);
	gtk_widget_show(tbltrmd);

	_gui->rdoauto = gtk_radio_button_new_with_label(NULL, "AUTO");
	gtk_widget_set_name(_gui->rdoauto, "rdoauto");
	g_signal_connect(G_OBJECT(_gui->rdoauto), "toggled",
					 G_CALLBACK(cbk_tglacn), _gui);
	gtk_table_attach_defaults(GTK_TABLE(tbltrmd), _gui->rdoauto, 0, 1, 0, 1);
	gtk_widget_show(_gui->rdoauto);

	_gui->rdonormal =
	gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(_gui->rdoauto),
												"NORMAL");
	gtk_widget_set_name(_gui->rdonormal, "rdonormal");
	g_signal_connect(G_OBJECT(_gui->rdonormal), "toggled",
					 G_CALLBACK(cbk_tglacn), _gui);
	gtk_table_attach_defaults(GTK_TABLE(tbltrmd), _gui->rdonormal, 1, 2, 0, 1);
	gtk_widget_show(_gui->rdonormal);

	_gui->rdosingle =
	gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(_gui->rdoauto),
												"SINGLE");
	gtk_widget_set_name(_gui->rdosingle, "rdosingle");
	g_signal_connect(G_OBJECT(_gui->rdosingle), "toggled",
					 G_CALLBACK(cbk_tglacn), _gui);
	gtk_table_attach_defaults(GTK_TABLE(tbltrmd), _gui->rdosingle, 0, 1, 1, 2);
	gtk_widget_show(_gui->rdosingle);

	_gui->rdostop =
	gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(_gui->rdoauto),
												"STOP");
	gtk_widget_set_name(_gui->rdostop, "rdostop");
	g_signal_connect(G_OBJECT(_gui->rdostop), "toggled",
					 G_CALLBACK(cbk_tglacn), _gui);
	gtk_table_attach_defaults(GTK_TABLE(tbltrmd), _gui->rdostop, 1, 2, 1, 2);
	gtk_widget_show(_gui->rdostop);

	/* LED */
	rawled = png_raw_new(g_data_ledon);
	_gui->pbfledon = gdk_pixbuf_new_from_data(rawled->data,
											  GDK_COLORSPACE_RGB,
											  TRUE, 8, rawled->w, rawled->h,
											  4 * rawled->w, NULL, NULL);
	rawled = png_raw_new(g_data_ledoff);
	_gui->pbfledoff = gdk_pixbuf_new_from_data(rawled->data,
											   GDK_COLORSPACE_RGB,
											   TRUE, 8, rawled->w, rawled->h,
											   4 * rawled->w, NULL, NULL);
	imgled = gtk_image_new_from_pixbuf(_gui->pbfledoff);
	gtk_container_add(GTK_CONTAINER(vboxtrg), imgled);
	gtk_widget_show(imgled);
	_gui->imgled = imgled;
	/* Source */
	mnutsrc = gtk_menu_new();
	for (i = 0; i < GUI_TSRCC; i++) {
		itmtsrc = gtk_menu_item_new_with_label(g_strtsrcs[i]);
		gtk_menu_shell_append(GTK_MENU_SHELL(mnutsrc), itmtsrc);
		gtk_widget_show(itmtsrc);
	}
	_gui->opttsrc = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(_gui->opttsrc), mnutsrc);
	gtk_widget_set_name(_gui->opttsrc, "opttsrc");
	g_signal_connect(G_OBJECT(_gui->opttsrc), "changed",
					 G_CALLBACK(cbk_optacn), _gui);
	gtk_container_add(GTK_CONTAINER(vboxtrg), _gui->opttsrc);
	gtk_widget_show(_gui->opttsrc);

	cbotsrc = gtk_combo_new();
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cbotsrc)->entry), FALSE);
	gtk_combo_set_popdown_strings(GTK_COMBO(cbotsrc), lstsrcs);
	gtk_widget_set_name(GTK_COMBO(cbotsrc)->entry, "cbotsrc");
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cbotsrc)->entry), "?");
	gtk_widget_set_name(cbotsrc, "cbotsrc");
	_gui->etytsrc = GTK_COMBO(cbotsrc)->entry;
	/* Coupling */
	mnutcpl = gtk_menu_new();
	for (i = 0; i < GUI_TCPLC; i++) {
		itmtcpl = gtk_menu_item_new_with_label(g_strtcpls[i]);
		gtk_menu_shell_append(GTK_MENU_SHELL(mnutcpl), itmtcpl);
		gtk_widget_show(itmtcpl);
	}
	_gui->opttcpl = gtk_option_menu_new();
	gtk_option_menu_set_menu(GTK_OPTION_MENU(_gui->opttcpl), mnutcpl);
	gtk_widget_set_name(_gui->opttcpl, "opttcpl");
	g_signal_connect(G_OBJECT(_gui->opttcpl), "changed",
					 G_CALLBACK(cbk_optacn), _gui);
	gtk_container_add(GTK_CONTAINER(vboxtrg), _gui->opttcpl);
	gtk_widget_show(_gui->opttcpl);

	cbotcpl = gtk_combo_new();
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cbotcpl)->entry), FALSE);
	gtk_combo_set_popdown_strings(GTK_COMBO(cbotcpl), lstcpls);
	gtk_widget_set_name(GTK_COMBO(cbotcpl)->entry, "cbotcpl");
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cbotcpl)->entry), "?");
	gtk_widget_set_name(cbotsrc, "cbotcpl");
	_gui->etytcpl = GTK_COMBO(cbotcpl)->entry;
	/* Trigger Delay */
	hboxtrdl = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vboxtrg), hboxtrdl, FALSE, FALSE, 0);
	gtk_widget_show(hboxtrdl);
	/* Left Button */
	btnleft = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(hboxtrdl), btnleft, FALSE, FALSE, 0);
	gtk_widget_show(btnleft);
	imgleft = gtk_image_new_from_stock("gtk-go-back", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_name(btnleft, "btnleft");
	g_signal_connect(G_OBJECT(btnleft), "button_release_event",
					 G_CALLBACK(cbk_usracn), _gui);
	gtk_widget_show(imgleft);
	arwleft = gtk_arrow_new(GTK_ARROW_LEFT, GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(btnleft), arwleft);
	gtk_widget_show(arwleft);
	/* Trigger Delay Label */
	lbltrdl = gtk_label_new("?");
	gtk_box_pack_start(GTK_BOX(hboxtrdl), lbltrdl, TRUE, FALSE, 0);
	gtk_widget_show(lbltrdl);
	_gui->lbltrdl = lbltrdl;
	/* Right Button */
	btnright = gtk_button_new();
	gtk_box_pack_start(GTK_BOX(hboxtrdl), btnright, FALSE, FALSE, 0);
	gtk_widget_show(btnright);
	imgright = gtk_image_new_from_stock("gtk-go-forward", GTK_ICON_SIZE_BUTTON);
	gtk_widget_set_name(btnright, "btnright");
	g_signal_connect(G_OBJECT(btnright), "button_release_event",
					 G_CALLBACK(cbk_usracn), _gui);
	gtk_widget_show(imgright);
	arwright = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(btnright), arwright);
	gtk_widget_show(arwright);

	return vboxcontrols;
}

/* Returns fully-populated contents box */
GtkWidget *gui_contents_new(gui_t *_gui)
{
	GtkWidget *hbox;
	GtkWidget *vboxtlvl;
	GtkWidget *scltlvl;
	GtkWidget *lbltlvl;
	GtkWidget *controls;

	/* Three-part HBox (plot, trigger level, controls) */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);

	/* Plot */
	_gui->evtboxplot = gui_winplot_new(_gui);
	gtk_box_pack_start(GTK_BOX(hbox), _gui->evtboxplot, FALSE, FALSE, 0);
	gtk_widget_show(_gui->evtboxplot);

	/* Trigger level vbox */
	vboxtlvl = gtk_vbox_new(FALSE, 0);
	gtk_widget_set_size_request(vboxtlvl, 50, -1);
	gtk_box_pack_start(GTK_BOX(hbox), vboxtlvl, FALSE, FALSE, 0);
	gtk_widget_show(vboxtlvl);
	/* Trigger level scale */
	scltlvl = gtk_vscale_new_with_range(-1., 1., 1.);
	gtk_range_set_inverted(GTK_RANGE(scltlvl), TRUE);
	gtk_scale_set_draw_value(GTK_SCALE(scltlvl), FALSE);
	gtk_widget_set_name(scltlvl, "scltlvl");
	g_signal_connect(G_OBJECT(scltlvl),
					 "button_release_event",
					 G_CALLBACK(cbk_usracn), _gui);
	g_signal_connect(G_OBJECT(scltlvl), "value-changed",
					 G_CALLBACK(cbk_scl), _gui);
	gtk_widget_set_sensitive(scltlvl, FALSE);
	gtk_box_pack_start(GTK_BOX(vboxtlvl), scltlvl, TRUE, TRUE, 0);
	gtk_widget_show(scltlvl);
	_gui->scltlvl = scltlvl;
	/* Trigger level label */
	lbltlvl = gtk_label_new("?");
	gtk_box_pack_start(GTK_BOX(vboxtlvl), lbltlvl, FALSE, FALSE, 0);
	gtk_widget_show(lbltlvl);
	_gui->lbltlvl = lbltlvl;

	/* Controls */
	controls = gui_controls_new(_gui);
	gtk_widget_set_sensitive(controls, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), controls, FALSE, FALSE, 0);
	_gui->controls = controls;

	return hbox;
}

/* Return full-blast window but without having fed any values in widgets */
GtkWidget *gui_winmain_new(gui_t *_gui)
{
	GtkWidget *vbox;			/* Menubar, contents, statusbar */
	GtkWidget *menubar;
	GtkWidget *contents;		/* Window content */

	/* Make new window */
	_gui->winmain = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* Three-part VBox (menubar, contents, statusbar) in window */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_gui->winmain), vbox);
	gtk_widget_show(vbox);
	/* Menubar */
	menubar = gui_menubar_new(_gui);
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show(menubar);
	/* Content */
	contents = gui_contents_new(_gui);
	gtk_box_pack_start(GTK_BOX(vbox), contents, TRUE, TRUE, 0);
	gtk_widget_show(contents);
	/* Statusbar */
	_gui->statusbar = gtk_statusbar_new();
	_gui->ctxid = gtk_statusbar_get_context_id(GTK_STATUSBAR(_gui->statusbar),
											   GUI_CONTEXTDESC);
	gtk_box_pack_start(GTK_BOX(vbox), _gui->statusbar, FALSE, FALSE, 0);
	gtk_widget_show(_gui->statusbar);

	return _gui->winmain;
}

#if DEV
GtkWidget *gui_wininfo_new(gui_t *_gui)
{
	GtkWidget *table;
	GtkWidget *lbldscqlen;

	_gui->wininfo = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	table = gtk_table_new(1, 2, TRUE);
	gtk_container_add(GTK_CONTAINER(_gui->wininfo), table);
	gtk_widget_show(table);

	lbldscqlen = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(lbldscqlen),
						 "<span weight='bold'>Commands queued</span>");
	gtk_table_attach(GTK_TABLE(table), lbldscqlen, 0, 1, 0, 1,
					 GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(lbldscqlen);

	_gui->lblvalqlen = gtk_label_new("?");
	gtk_table_attach(GTK_TABLE(table), _gui->lblvalqlen, 1, 2, 0, 1,
					 GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(_gui->lblvalqlen);

	return _gui->wininfo;
}
#endif

/* Creates a new about window with widgets */
GtkWidget *gui_winabout_new(gui_t *_g)
{
	GtkWidget *box;
	GtkWidget *frame;
	GtkWidget *label;
	char buf[SMLBUFSIZE];

	_g->winabout = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(_g->winabout), "About Wavejet Control");
	g_signal_connect(G_OBJECT(_g->winabout), "delete-event",
					 G_CALLBACK(cbk_delwinabout), NULL);

	box = gtk_vbox_new(FALSE, 20);
	gtk_container_add(GTK_CONTAINER(_g->winabout), box);
	gtk_widget_show(box);

	frame = gtk_frame_new("Wavejet Control Version");
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	label = gtk_label_new("20101125");
	gtk_container_add(GTK_CONTAINER(frame), label);
	gtk_widget_show(label);

	frame = gtk_frame_new("GTK+ Version");
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	sprintf(buf, "%d.%d.%d", gtk_major_version,
							 gtk_minor_version,
							 gtk_micro_version);
	label = gtk_label_new(buf);
	gtk_container_add(GTK_CONTAINER(frame), label);
	gtk_widget_show(label);

	frame = gtk_frame_new("Release Notes");
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	label = gtk_label_new("\
Added this about box.\n\
BYTE waveforms (as opposed to ASCII) for all trigger modes.\n\
Various fixes.");
	gtk_container_add(GTK_CONTAINER(frame), label);
	gtk_widget_show(label);

	return _g->winabout;
}

/* Creates a new RC window with widgets */
GtkWidget *gui_winprfs_new(gui_t *_gui)
{
	GtkWidget *table;
	GtkWidget *lbladdr, *lblport;
	char buf[SMLBUFSIZE];	/* GP Buffer */
	GtkTooltips *tooltips;

	/* Window */
	snprintf(buf, SMLBUFSIZE, "%s - Preferences", GUI_TITLE);
	_gui->winprfs =
		gtk_dialog_new_with_buttons(buf,
									GTK_WINDOW(_gui->winmain),
									0,
									GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
									GTK_STOCK_OK, GTK_RESPONSE_OK,
									GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
									NULL);

	/* Table */
	table = gtk_table_new(2, 3, TRUE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(_gui->winprfs)->vbox), table);
	gtk_widget_show(table);

	/* Labels */
	lbladdr = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(lbladdr),
						 "<span weight='bold'>Scope Address</span>");
	gtk_misc_set_alignment(GTK_MISC(lbladdr), 1, .5);
	gtk_table_attach(GTK_TABLE(table), lbladdr, 0, 1, 0, 1,
					 GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(lbladdr);

	lblport = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(lblport),
						 "<span weight='bold'>Scope Port</span>");
	gtk_misc_set_alignment(GTK_MISC(lblport), 1, .5);
	gtk_table_attach(GTK_TABLE(table), lblport, 0, 1, 1, 2,
					 GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(lblport);

	/* Entries */
	tooltips = gtk_tooltips_new();
	_gui->etyaddr = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), _gui->etyaddr, 1, 2, 0, 1,
					 GTK_FILL, GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, _gui->etyaddr,
						 "Scope hostname or IP address",
						 "Scope hostname or IP address");
	gtk_widget_show(_gui->etyaddr);

	_gui->etyport = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), _gui->etyport, 1, 2, 1, 2,
					 GTK_FILL, GTK_FILL, 0, 0);
	gtk_tooltips_set_tip(tooltips, _gui->etyport,
						 "Scope port (usually 1861)",
						 "Scope port (usually 1861)");
	gtk_widget_show(_gui->etyport);

	/* Error Icons */
	_gui->imgaddr =
		gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
	gtk_table_attach(GTK_TABLE(table), _gui->imgaddr, 2, 3, 0, 1,
					 GTK_SHRINK, GTK_SHRINK, 0, 0);

	_gui->imgport =
		gtk_image_new_from_stock(GTK_STOCK_STOP, GTK_ICON_SIZE_MENU);
	gtk_table_attach(GTK_TABLE(table), _gui->imgport, 2, 3, 1, 2,
					 GTK_SHRINK, GTK_SHRINK, 0, 0);


	/* Signals */
 	g_signal_connect(G_OBJECT(_gui->winprfs), "delete_event",
					 G_CALLBACK(cbk_delwinprfs), _gui);
	g_signal_connect(G_OBJECT(_gui->winprfs), "response",
					 G_CALLBACK(cbk_rspwinprfs), _gui);

	return _gui->winprfs;
}

/* Sets RC values to RC window after it's created ('cause upon window creation
is too early as we haven't read the RC file yet) */
int gui_winprfs_set(gui_t *_gui, prfs_t *_prfs)
{
	int rc = 0;

	/* RC Variables */
	char addr[PRF_MAXLEN] = "";
	char port[PRF_MAXLEN] = "";

	if (_gui && _prfs) {
		rc |= prf_get(_prfs, "addr", addr);
		gtk_entry_set_text(GTK_ENTRY(_gui->etyaddr), addr);
	}

	if (_gui && _prfs) {
		rc |= prf_get(_prfs, "port", port);
		gtk_entry_set_text(GTK_ENTRY(_gui->etyport), port);
	}

	return rc;
}

GtkWidget *gui_winplot_new(gui_t *_gui)
{
	GtkWidget *imgplot;

	/* Initialize things */
	_gui->ox = -1;
	_gui->oy = -1;
	_gui->tra = -1;
	_gui->clrs[0] = 173 >> (8 - _gui->visual->red_prec) <<
					_gui->visual->red_shift |
					170 >> (8 - _gui->visual->green_prec) <<
					_gui->visual->green_shift |
					0   >> (8 - _gui->visual->blue_prec) <<
					_gui->visual->blue_shift;
	_gui->clrs[1] = 173 >> (8 - _gui->visual->red_prec) <<
					_gui->visual->red_shift |
					0   >> (8 - _gui->visual->green_prec) <<
					_gui->visual->green_shift |
					173 >> (8 - _gui->visual->blue_prec) <<
					_gui->visual->blue_shift;
	_gui->clrs[2] = 0   >> (8 - _gui->visual->red_prec) <<
					_gui->visual->red_shift |
					170 >> (8 - _gui->visual->green_prec) <<
					_gui->visual->green_shift |
					173 >> (8 - _gui->visual->blue_prec) <<
					_gui->visual->blue_shift;
	_gui->clrs[3] = 0   >> (8 - _gui->visual->red_prec) <<
					_gui->visual->red_shift |
					170 >> (8 - _gui->visual->green_prec) <<
					_gui->visual->green_shift |
					0   >> (8 - _gui->visual->blue_prec) <<
					_gui->visual->blue_shift;

	/* GtkAlignment, to control the size of the children */
	_gui->alignment = gtk_alignment_new(0., 0., 0., 0.);
	gtk_widget_show(_gui->alignment);

	/* Create GdkImage */
	_gui->imgplot = gdk_image_new(GDK_IMAGE_FASTEST,
                            	  _gui->visual,
                            	  GUI_TSCRNW,
                            	  GUI_TSCRNH);

	/* A GtkImage can't receive events. Put it in an event box */
	_gui->evtboxplot = gtk_event_box_new();
	#ifdef DEV
	g_signal_connect(G_OBJECT(_gui->evtboxplot), "motion-notify-event",
					 G_CALLBACK(cbk_mtnplot), _gui);
	g_signal_connect(G_OBJECT(_gui->evtboxplot), "button-release-event",
					 G_CALLBACK(cbk_rlsplot), _gui);
	#endif
	gtk_container_add(GTK_CONTAINER(_gui->alignment), _gui->evtboxplot);
	gtk_widget_show(_gui->evtboxplot);

	/* Create plot image */
	imgplot = gtk_image_new_from_image(_gui->imgplot, NULL);
	gtk_container_add(GTK_CONTAINER(_gui->evtboxplot), imgplot);
	gtk_widget_show(imgplot);

	/* Draw grid */
	gui_drawgrid(_gui);

	return _gui->alignment;
}

#ifdef DEV
GtkWidget *gui_winscrn_new(gui_t *_gui)
{
	/* Widget Variables */
	GtkWidget *vbox;
	GtkWidget *toolbar;
	GtkWidget *gtkimage;

	/* Startup Image Variables */
	int i, j;
	FILE *fp;
	png_bytepp rows;
	long colour;

	/* Make new window */
	_gui->winscrn = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	/* VBox */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_gui->winscrn), vbox);
	gtk_widget_show(vbox);

	/* Buttons */
	_gui->btnrfstscrn = gtk_button_new_from_stock(GTK_STOCK_REFRESH);

	/* Init PNG */
	_gui->pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	_gui->pnginfo = png_create_info_struct(_gui->pngptr);
	png_set_palette_to_rgb(_gui->pngptr);

	/* Read startup PNG image */
	fp = fopen("cern.png", "r");
	png_init_io(_gui->pngptr, fp);
	png_read_png(_gui->pngptr, _gui->pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
	rows = png_get_rows(_gui->pngptr, _gui->pnginfo);

	/* Create GdkImage */
	_gui->gdkimgscrn = gdk_image_new(GDK_IMAGE_FASTEST,
									 _gui->visual,
									 GUI_TSCRNW,
									 GUI_TSCRNH);

	/* Set selection 24 bit colour and normalize it to visual */
	_gui->r = _gui->g = _gui->b = 68;
	_gui->r = _gui->r >> (8 - _gui->visual->red_prec);
	_gui->g = _gui->g >> (8 - _gui->visual->green_prec);
	_gui->b = _gui->b >> (8 - _gui->visual->blue_prec);

	/* Draw startup image (CERN logo) */
	for (i = 0; i < GUI_TSCRNH; i++) {
		for (j = 0; j < GUI_TSCRNW; j++) {
			colour = rows[i][3 * j] >>
					 (8 - _gui->visual->red_prec) <<
					 _gui->visual->red_shift |
					 rows[i][3 * j + 1]  >>
					 (8 - _gui->visual->green_prec) <<
					 _gui->visual->green_shift |
				 	 rows[i][3 * j + 2]  >>
					 (8 - _gui->visual->blue_prec) <<
					 _gui->visual->blue_shift;
			gdk_image_put_pixel(_gui->gdkimgscrn, j, i, colour);
		}
	}

	/* Make image backup */
	memcpy(_gui->pixels,
		   _gui->gdkimgscrn->mem,
		   _gui->gdkimgscrn->width *
		   _gui->gdkimgscrn->height *
		   _gui->gdkimgscrn->bpp);

	/* A GtkImage can't receive events. Put it in an event box */
	_gui->evtboxscrn = gtk_event_box_new();
	gtk_box_pack_start(GTK_BOX(vbox), _gui->evtboxscrn, FALSE, FALSE, 0);
	gtk_widget_show(_gui->evtboxscrn);

	/* Create plot image */
	gtkimage = gtk_image_new_from_image(_gui->gdkimgscrn, NULL);
	gtk_container_add(GTK_CONTAINER(_gui->evtboxscrn), gtkimage);
	gtk_widget_show(gtkimage);

	/* Cleanup PNG */
	png_destroy_read_struct(&_gui->pngptr, &_gui->pnginfo, NULL);
	fclose(fp);

	/* Housekeeping */
	_gui->ox = -1;	/* Origin set to stupid values (i.e. no origin yet) */
	_gui->oy = -1;	/* Origin set to stupid values (i.e. no origin yet) */

	/* Statusbar */
	_gui->stsbartscrn = gtk_statusbar_new();
	_gui->ctxidtscrn =
		gtk_statusbar_get_context_id(GTK_STATUSBAR(_gui->stsbartscrn),
									 GUI_CONTEXTDESC);
	gtk_statusbar_push(GTK_STATUSBAR(_gui->stsbartscrn),
					   _gui->ctxidtscrn, "Ready");
	gtk_box_pack_start(GTK_BOX(vbox), _gui->stsbartscrn, FALSE, FALSE, 0);
	gtk_widget_show(_gui->stsbartscrn);

	return _gui->winscrn;
}

void gui_drawplot(gui_t *_gui)
{
	int i, j;
	long clr;
	int r, g, b;
	int x, y;
	float alpha;

	/* Catch origin */
	if (_gui->ox == -1) {
		_gui->ox = _gui->x;
		_gui->oy = _gui->y;
	}

	/* Make first click was in plot */
	if (_gui->ox < GUI_TSCRN_LEFT || _gui->ox > GUI_TSCRN_RIGHT ||
		_gui->oy < GUI_TSCRN_TOP || _gui->oy > GUI_TSCRN_BOTTOM) {
		return;
	}

	/* Check coordinates */
	if (_gui->x < GUI_TSCRN_LEFT) {
		x = GUI_TSCRN_LEFT;
	} else if (_gui->x > GUI_TSCRN_RIGHT) {
		x = GUI_TSCRN_RIGHT;
	} else {
		x = _gui->x;
	}
	y = _gui->y;

	/* Clear previous image changes */
	memcpy(_gui->gdkimgscrn->mem,
		   _gui->pixels,
		   _gui->gdkimgscrn->width *
		   _gui->gdkimgscrn->height *
		   _gui->gdkimgscrn->bpp);

	/* Draw center-mirrored selection */
	if (x > GUI_TSCRN_HMIDDLE) {	/* Click right */
		for (i = GUI_TSCRN_VMIDDLE - 10; i < GUI_TSCRN_VMIDDLE + 11; i++) {
			/* Do the math! (I.e. pen, paper, scribble) */
			for (j = 2 * GUI_TSCRN_HMIDDLE - x - 1; j < x; j++) { 
				/* Get destination pixel colour */
				clr = gdk_image_get_pixel(_gui->gdkimgscrn, j, i);
				r = (clr & _gui->visual->red_mask) >>
					_gui->visual->red_shift;
				g = (clr & _gui->visual->green_mask) >>
					_gui->visual->green_shift;
				b = (clr & _gui->visual->blue_mask) >>
					_gui->visual->blue_shift;

				/* Compute blended colour */
				if (i < GUI_TSCRN_VMIDDLE - 8 || i > GUI_TSCRN_VMIDDLE + 8 ||
					j < 2 * GUI_TSCRN_HMIDDLE - x + 1 || j > x - 3) {
					alpha = GUI_TSCRN_MOREALPHA;
				} else {
					alpha = GUI_TSCRN_ALPHA;
				}
				r = r * (1 - alpha) + _gui->r * alpha;
				g = g * (1 - alpha) + _gui->g * alpha;
				b = b * (1 - alpha) + _gui->b * alpha;

				/* Make colour */
				clr = r << _gui->visual->red_shift |
					  g << _gui->visual->green_shift |
					  b << _gui->visual->blue_shift;

				/* Put pixel */
				gdk_image_put_pixel(_gui->gdkimgscrn, j, i, clr);
			}
		}
	} else {						/* Click left */
		for (i = GUI_TSCRN_VMIDDLE - 10; i < GUI_TSCRN_VMIDDLE + 11; i++) {
			/* Do the math! (I.e. pen, paper, scribble) */
			for (j = x + 1; j < 2 * GUI_TSCRN_HMIDDLE - x - 2; j++) { 
				/* Get destination pixel colour */
				clr = gdk_image_get_pixel(_gui->gdkimgscrn, j, i);
				r = (clr & _gui->visual->red_mask) >>
					_gui->visual->red_shift;
				g = (clr & _gui->visual->green_mask) >>
					_gui->visual->green_shift;
				b = (clr & _gui->visual->blue_mask) >>
					_gui->visual->blue_shift;

				/* Compute blended colour */
				if (i < GUI_TSCRN_VMIDDLE - 8 || i > GUI_TSCRN_VMIDDLE + 8 ||
					j < x + 3 || j > 2 * GUI_TSCRN_HMIDDLE - x - 5) {
					alpha = GUI_TSCRN_MOREALPHA;
				} else {
					alpha = GUI_TSCRN_ALPHA;
				}
				r = r * (1 - alpha) + _gui->r * alpha;
				g = g * (1 - alpha) + _gui->g * alpha;
				b = b * (1 - alpha) + _gui->b * alpha;

				/* Make colour */
				clr = r << _gui->visual->red_shift |
					  g << _gui->visual->green_shift |
					  b << _gui->visual->blue_shift;

				/* Put pixel */
				gdk_image_put_pixel(_gui->gdkimgscrn, j, i, clr);
			}
		}
	}

	/* Update display */
	gtk_widget_queue_draw(_gui->evtboxscrn);
}
#endif

/* Reading thread handlers */
void gui_tdiv(int _islast, char *_data, int _len, void *_gui)
{
	char buf0[SMLBUFSIZE], buf1[SMLBUFSIZE];	/* GP Buffers */
	gui_t *gui;
	int i;

	/* Convenient */
	gui = _gui;

	/* NULL _data? (Because of timeout, error, end of the world...) */
	if (_data == NULL) {
		return;
	}

	/* Get data from scope */
	nb_eng(nb_engtod(_data), 3, 0, buf0);
	snprintf(buf1, SMLBUFSIZE, "%ss", buf0);

	/* Find corresponding menu index */
	for (i = 0; i < GUI_TDIVC; i++) {
		if (strcmp(buf1, g_strtdivs[i]) == 0){
			g_locked = 1;
			gdk_threads_enter();
			gtk_option_menu_set_history(GTK_OPTION_MENU(gui->opttdiv), i);
			gdk_flush();
			gdk_threads_leave();
			g_locked = 0;
			break;
		}
	}
}

void gui_tra(int _islast, char *_data, int _len, void *_tgl)
{
	GtkWidget *tglbtn;

	/* Convenient */
	tglbtn = _tgl;

	/* Finish string and remove newline */
	_data[_len - 1] = 0;

	g_locked = 1;
	gdk_threads_enter();
	if (strcmp(_data, "ON") == 0) {
		#if 0
		cbk_locked = 1;
		/* FIXME Not perfect: if there's a lot of time between set_text and
		 * event handling, any other callback which should've done their
		 * work will be locked too. But usually, events are handled so quickly
		 * it shouldn't happen and it's probably no worth bothering any
		 * further. */
		 #endif
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tglbtn), TRUE);
	} else if (strcmp(_data, "OFF") == 0) {
		#if 0
		cbk_locked = 1;
		/* FIXME Not perfect: if there's a lot of time between set_text and
		 * event handling, any other callback which should've done their
		 * work will be locked too. But usually, events are handled so quickly
		 * it shouldn't happen and it's probably no worth bothering any
		 * further. */
		 #endif
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tglbtn), FALSE);
	}
	gdk_flush();
	gdk_threads_leave();
	g_locked = 0;
}

void gui_tlvl(int _islast, char *_data, int _len, void *_gui)
{
	char buf0[SMLBUFSIZE], buf1[SMLBUFSIZE];	/* GP Buffers */
	double dbltlvl, dblscl;
	gui_t *gui;
	int idx;
	gui_chl_t *chl;

	/* Convenient */
	gui = _gui;

	/* Get floating point number value */
	dbltlvl = nb_engtod(_data);

	/* Get tsrc */
	idx = gtk_option_menu_get_history(GTK_OPTION_MENU(gui->opttsrc));

	/* Compute values depending on tsrc */
	if (strcmp(g_strtsrcs[idx], "CH1") == 0) {
		chl = &gui->chls[0];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		dblscl = dbltlvl / (nb_engtod(g_strvdivs[idx]) * 4);
	} else if (strcmp(g_strtsrcs[idx], "CH2") == 0) {
		chl = &gui->chls[1];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		dblscl = dbltlvl / (nb_engtod(g_strvdivs[idx]) * 4);
	} else if (strcmp(g_strtsrcs[idx], "CH3") == 0) {
		chl = &gui->chls[2];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		dblscl = dbltlvl / (nb_engtod(g_strvdivs[idx]) * 4);
	} else if (strcmp(g_strtsrcs[idx], "CH4") == 0) {
		chl = &gui->chls[3];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		dblscl = dbltlvl / (nb_engtod(g_strvdivs[idx]) * 4);
	} else if (strcmp(g_strtsrcs[idx], "EXT") == 0) {
		dblscl = dbltlvl / .5;
	} else if (strcmp(g_strtsrcs[idx], "EXT10") == 0) {
		dblscl = dbltlvl / 5.;
	}

	/* Set label text and set slider value */
	nb_eng(dbltlvl, 3, 0, buf0);
	snprintf(buf1, SMLBUFSIZE, "%sV", buf0);
	g_locked = 1;
	gdk_threads_enter();
	/* For some reason, you should first set the slider value, then the label
	 * text, otherwise some display problem may occur */
	gtk_range_set_value(GTK_RANGE(gui->scltlvl), dblscl);
	gtk_label_set_text(GTK_LABEL(gui->lbltlvl), buf1);
	gdk_flush();
	gdk_threads_leave();
	g_locked = 0;
}

void gui_trdl(int _islast, char *_data, int _len, void *_gui)
{
	char buf0[SMLBUFSIZE], buf1[SMLBUFSIZE];	/* GP Buffers */
	gui_t *gui;
	double val;

	/* Convenient */
	gui = _gui;

	val = nb_engtod(_data);
	if (val != 0.) {
		nb_eng(nb_engtod(_data), 3, 0, buf0);
		snprintf(buf1, SMLBUFSIZE, "%ss", buf0);
	} else {
		snprintf(buf1, SMLBUFSIZE, "0 s");
	}
	g_locked = 1;
	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL(gui->lbltrdl), buf1);
	gdk_flush();
	gdk_threads_leave();
	g_locked = 0;
}

void gui_tsrc(int _islast, char *_data, int _len, void *_gui)
{
	gui_t *gui;
	int i;

	/* Convenient */
	gui = _gui;

	/* Remove newline and end string */
	_data[_len - 1] = 0;

	/* Find corresponding menu index */
	for (i = 0; i < GUI_TSRCC; i++) {
		if (strcmp(_data, g_strtsrcs[i]) == 0){
			g_locked = 1;
			gdk_threads_enter();
			gtk_option_menu_set_history(GTK_OPTION_MENU(gui->opttsrc), i);
			gdk_flush();
			gdk_threads_leave();
			g_locked = 0;
			break;
		}
	}
}

void gui_tcpl(int _islast, char *_data, int _len, void *_gui)
{
	gui_t *gui;
	int i;

	/* Convenient */
	gui = _gui;

	/* Remove newline and end string */
	_data[_len - 1] = 0;

	/* Find corresponding menu index */
	for (i = 0; i < GUI_TCPLC; i++) {
		if (strcmp(_data, g_strtcpls[i]) == 0){
			g_locked = 1;
			gdk_threads_enter();
			gtk_option_menu_set_history(GTK_OPTION_MENU(gui->opttcpl), i);
			gdk_flush();
			gdk_threads_leave();
			g_locked = 0;
			break;
		}
	}
}

/* GUI connection to scope (THE connection, that is) */
void *gui_connect(void *_gui)
{
	int i;
	int rc;
	scp_t *scp;
	gui_t *gui;
	char buf[SMLBUFSIZE];
	char addr[PRF_LEN];
	char port[PRF_LEN];

	/* Gtk Variables */
	GtkWidget *dialog;

	/* Convenient */
	gui = _gui;

	/* Say we're trying to connect */
	gdk_threads_enter();
	gtk_statusbar_push(GTK_STATUSBAR(gui->statusbar),
					   gui->ctxid,
					   GUI_DESC_CONNECTING);
	gdk_flush();
	gdk_threads_leave();

	/* Connect to scope, giving runtime-precedence to command-line prefs */
	rc = prf_get(gui->prfs, "addr", addr);
	if (rc != 0) {
		/* No point in going any further */
		gdk_threads_enter();
		gtk_statusbar_push(GTK_STATUSBAR(gui->statusbar),
						   gui->ctxid,
						   GUI_DESC_FAILURE);
		gdk_flush();
		gdk_threads_leave();
		return NULL;
	}
	prf_get(gui->prfs, "port", port);

	scp = scp_new(addr, port);

	/* Take actions if successful */
	if (scp) {
		/* Say it's successful and alter menus */
		gdk_threads_enter();
		gtk_statusbar_push(GTK_STATUSBAR(gui->statusbar), gui->ctxid,
						   GUI_DESC_ONLINE);
		gtk_widget_set_sensitive(gui->itmconnect, FALSE);
		gtk_widget_set_sensitive(gui->itmdisconnect, TRUE);
		gdk_flush();
		gdk_threads_leave();
		/* Tell GUI we have a scope */
		gui->scp = scp;
		for (i = 0; i < SCP_CHC; i++) {
			gui->chls[i].scp = scp;
		}
		/* Start plotting thread */
		pthread_mutex_init(&gui->mtxloopplot, NULL);
		pthread_cond_init(&gui->cndloopplot, NULL);
		gui->plotkill = 0;
		pthread_create(&gui->thdloopplot, NULL, gui_loopplot, gui);
		/* Feed GUI with various scope values */
		scp_cmd_push(scp, "tdiv?", gui_tdiv, gui);
		for (i = 0; i < SCP_CHC; i++) {
			snprintf(buf, SMLBUFSIZE, "c%d:tra?", i + 1);
			scp_cmd_push(scp, buf, gui_tra, gui->chls[i].tglenbl);
			snprintf(buf, SMLBUFSIZE, "c%d:vdiv?", i + 1);
			scp_cmd_push(scp, buf, gui_vdiv, &gui->chls[i]);
			snprintf(buf, SMLBUFSIZE, "c%d:ofst?", i + 1);
			scp_cmd_push(scp, buf, gui_ofst, &gui->chls[i]);
		}
		scp_cmd_push(scp, "tsrc?", gui_tsrc, gui);
		scp_cmd_push(scp, "tcpl?", gui_tcpl, gui);
		scp_cmd_push(scp, "tlvl?", gui_tlvl, gui);
		scp_cmd_push(scp, "trdl?", gui_trdl, gui);

		#if DEV
		/* Enable touch screen */
		gtk_signal_connect(GTK_OBJECT(gui->evtboxscrn),
						   "motion_notify_event",
						   (GtkSignalFunc) cbk_select,
						   gui);
		gtk_signal_connect(GTK_OBJECT(gui->evtboxscrn),
						   "button_release_event",
						   (GtkSignalFunc) cbk_dropselection,
						   gui);
		#endif

		/* Enable GUI */
		gdk_threads_enter();
		gtk_widget_set_sensitive(gui->controls, TRUE);
		gtk_widget_set_sensitive(gui->scltlvl, TRUE);
		/* FIXME What does this do? */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->rdonormal), TRUE);
		gdk_flush();
		gdk_threads_leave();
	} else {
		/* Tell it's not successful */
		scp_destroy(scp);
		gdk_threads_enter();
		dialog = gtk_message_dialog_new(GTK_WINDOW(gui->winmain),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			"Couldn't connect to scope");
		gtk_widget_show(dialog);
		g_signal_connect(G_OBJECT(dialog), "response",
						 G_CALLBACK(cbk_rpsdlg), _gui);

		gtk_statusbar_push(GTK_STATUSBAR(gui->statusbar), gui->ctxid,
						   GUI_DESC_FAILURE);
		gdk_flush();
		gdk_threads_leave();
	}

	/* FIXME It's never picked up anyway, is it? */
	return scp;
}

int gui_disconnect(gui_t *_gui)
{
	int rc;
/* 	GtkWidget *lblconnect; */

	/* Quit plotting thread */
	_gui->plotkill = 1;
	rc = pthread_mutex_lock(&_gui->mtxloopplot);
	pthread_cond_signal(&_gui->cndloopplot);
	pthread_mutex_unlock(&_gui->mtxloopplot);

	rc = pthread_join(_gui->thdloopplot, NULL);
	if (rc != 0) {
		/* Already joined */
		return 0;
	}

/* 	rc = pthread_cond_destroy(&_gui->cndloopplot);
	if (rc != 0) {
		fprintf(stderr, "Couldn't destroy condition on plot loop\n");
		return 1;
	} */

/* 	rc = pthread_mutex_destroy(&_gui->mtxloopplot);
	if (rc != 0) {
		fprintf(stderr, "Couldn't destroy mutex on plot loop: %s\n",
				strerror(rc));
		return 1;
	} */

	/* Quit scope reading thread */
	gdk_threads_leave();
	rc = scp_destroy(_gui->scp);
	gdk_threads_enter();
	if (rc) {
		fprintf(stderr, "Couldn't destroy scope\n");
		return 1;
	}

	/* Update GUI */
	gtk_widget_set_sensitive(_gui->scltlvl, FALSE);
	gtk_widget_set_sensitive(_gui->controls, FALSE);
	gtk_widget_set_sensitive(_gui->itmconnect, TRUE);
	gtk_widget_set_sensitive(_gui->itmdisconnect, FALSE);
	gtk_statusbar_push(GTK_STATUSBAR(_gui->statusbar),
					   _gui->ctxid,
					   GUI_DESC_OFFLINE);

/* 	gtk_widget_set_sensitive(_gui->evtboxplot, FALSE); */

	#if 0
	lblconnect = gtk_bin_get_child(GTK_BIN(_gui->itmconnect));
	gtk_label_set_text(GTK_LABEL(lblconnect), LBLCONNECT);
	#endif

	return 0;
}

#if DEV
void gui_readpng(png_structp _pngptr, png_bytep _data, png_size_t _len)
{
	void **datainfo;

	/* Get input */
	datainfo = png_get_io_ptr(_pngptr);

	/* Copy data */
	memcpy(_data, (char *) datainfo[1] + *((int *) datainfo[0]), _len);
	*((int *) datainfo[0]) += _len;
}

/* Newer PNG file writer (called repeatedly) */
void gui_tscrn(int _last, char *_data, int _len, void *_gui)
{
	int i, j;
	static int foreword = 1;
	gui_t *gui;
	long colour;

	/* Handler pngread Variables */
	void *datainfo[2];	/* Data and info passed to the readpng handler */
	int offset = 0;		/* Bytes already read */

	/* PNG Variables */
	png_structp pngptr;
	png_infop pnginfo;
	png_bytepp rows;

	/* Convenient */
	gui = (gui_t *) _gui;

	/* Scope sends 10 bytes foreword which is not part of the PNG file itself.
	   I don't know (nor care) what it is, I'll just track it and skip it. */
	if (foreword) {
		/* In case of error or timeout */
		if (_data == NULL) {
			/* Say we're done */
			gdk_threads_enter();
			gtk_statusbar_push(GTK_STATUSBAR(gui->stsbartscrn),
							   gui->ctxidtscrn, "Ready");
			gtk_widget_set_sensitive(gui->btnrfstscrn, TRUE);
			gdk_flush();
			gdk_threads_leave();
		}
		foreword = 0;
	} else if (_last) {
		/* Ignore final lousy byte and prepare next PNG
		which will have a foreword */
		foreword = 1;
		/* Say we're done */
		gdk_threads_enter();
		gtk_statusbar_push(GTK_STATUSBAR(gui->stsbartscrn),
						   gui->ctxidtscrn, "Ready");
		gtk_widget_set_sensitive(gui->btnrfstscrn, TRUE);
		gdk_flush();
		gdk_threads_leave();
	} else {
		/* Init PNG */
		pngptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		pnginfo = png_create_info_struct(pngptr);
		png_set_palette_to_rgb(pngptr);

		/* Setup pngread handler info and data */
		datainfo[0] = &offset;
		datainfo[1] = _data;

		/* Read PNG data */
		png_set_read_fn(pngptr, datainfo, gui_readpng);
		png_read_png(pngptr, pnginfo, PNG_TRANSFORM_IDENTITY, NULL);
		rows = png_get_rows(pngptr, pnginfo);

		gdk_threads_enter();
		/* Draw pixels */
		for (i = 0; i < GUI_TSCRNH; i++) {
			for (j = 0; j < GUI_TSCRNW; j++) {
				colour = rows[i][3 * j] >>
						 (8 - gui->visual->red_prec) <<
						 gui->visual->red_shift |
						 rows[i][3 * j + 1]  >>
						 (8 - gui->visual->green_prec) <<
						 gui->visual->green_shift |
						 rows[i][3 * j + 2]  >>
						 (8 - gui->visual->blue_prec) <<
						 gui->visual->blue_shift;
				gdk_image_put_pixel(gui->gdkimgscrn, j, i, colour);
			}
		}
		/* Make image backup */
		memcpy(gui->pixels,
			   gui->gdkimgscrn->mem,
			   gui->gdkimgscrn->width *
			   gui->gdkimgscrn->height *
			   gui->gdkimgscrn->bpp);
		/* Update display */
		if (gui->ox == -1) {
			gtk_widget_queue_draw(gui->evtboxscrn);
		} else {
			gui_drawplot(gui);
		}
		gdk_flush();
		gdk_threads_leave();

		/* Cleanup PNG */
		png_destroy_read_struct(&pngptr, &pnginfo, NULL);
	}

	/* PNG data comes in three bunches from the scope. The first is made of 10
	bytes I don't bother reading. The second is the PNG data itself. The third
	is one lousy trailing byte I never bother keeping because I'd bet it's just
	there to annoy me. */
}
#endif

void gui_drawline(GdkImage *_img, gui_pnt_t *_tra,
				  long _clr, int _x1, int _y1, int _x2, int _y2)
{
	int i;
	double dydx, dxdy;
/* 	static int j = 0; */
/* 	gui_pnt_t pnt; */

	if (_x2 - _x1 == 0) {
		/* Vertical Bar */
		if (_y2 - _y1 > 0) {
			for (i = 0; i < _y2 - _y1; i++) {
				gdk_image_put_pixel(_img, _x1, _y1 + i, _clr);
/* 				_tra[_x1] = _y1 + i; */
/* 				pnt.x = _x1;
				pnt.y = _y1 + i;
				_tra[j++] = pnt; */
			}
		} else if (_y1 - _y2 > 0) {
			for (i = 0; i < _y1 - _y2; i++) {
				gdk_image_put_pixel(_img, _x1, _y2 + i, _clr);
/* 				_tra[_x1] = _y2 + i; */
/* 				pnt.x = _x1;
				pnt.y = _y2 + i;
				_tra[j++] = pnt; */
			}
		} else {
			gdk_image_put_pixel(_img, _x1, _y1, _clr);
/* 			_tra[_x1] = _y1; */
/* 			pnt.x = _x1;
			pnt.y = _y1;
			_tra[j++] = pnt; */
		}
	} else if (_y2 - _y1 == 0) {
		/* Horizontal Bar */
		if (_x2 - _x1 > 0) {
			for (i = 0; i < _x2 - _x1; i++) {
				gdk_image_put_pixel(_img, _x1 + i, _y1, _clr);
/* 				_tra[_x1 + i] = _y1; */
/* 				pnt.x = _x1 + i;
				pnt.y = _y1;
				_tra[j++] = pnt; */
			}
		} else {
			/* We shouldn't reach this */
			#if DEBUG
			printf("reached the unreachable\n");
			#endif
		}
	} else {
		/* Oblique line */
		dydx = (float) (_y2 - _y1) / (_x2 - _x1);
		if (dydx > 1.) {
			dxdy = (float) (_x2 - _x1) / (_y2 - _y1);
			for (i = 0; i <= _y2 - _y1; i++) {
				gdk_image_put_pixel(_img, _x1 + round(dxdy * i), i + _y1, _clr);
/* 				_tra[_x1 + (int) round(dxdy * i)] = i + _y1; */
/* 				pnt.x = _x1 + round(dxdy * i);
				pnt.y = i + _y1;
				_tra[j++] = pnt; */
			}
		} else if (dydx < -1.) {
			dxdy = (float) (_x2 - _x1) / (_y2 - _y1);
			for (i = 0; i <= _y1 - _y2; i++) {
				gdk_image_put_pixel(_img, _x2 + round(dxdy * i), i + _y2, _clr);
/* 				_tra[_x2 + (int) round(dxdy * i)] = i + _y2; */
/* 				pnt.x = _x2 + round(dxdy * i);
				pnt.y = i + _y2;
				_tra[j++] = pnt; */
			}
		} else {
			for (i = 0; i <= _x2 - _x1; i++) {
				gdk_image_put_pixel(_img, i + _x1, _y1 + round(dydx * i), _clr);
				/* Don't know why there's a _tra[...] = ... missing */
/* 				pnt.x = i + _x1;
				pnt.y = _y1 + round(dydx * i);
				_tra[j++] = pnt; */
			}
		}
	}
}

void gui_drawgrid(gui_t *_gui)
{
	int i, j;
	long grey;

	grey = 115 >> (8 - _gui->visual->red_prec) << _gui->visual->red_shift |
		   117 >> (8 - _gui->visual->green_prec) << _gui->visual->green_shift |
		   115 >> (8 - _gui->visual->blue_prec) << _gui->visual->blue_shift;

	/* Frame - Top and Bottom */
	for (i = GUI_PLOT_PAD;
		 i < GUI_PLOT_PAD + GUI_PLOT_TDIVC * GUI_PLOT_SUBDIVLEN * 5; i++) {
		gdk_image_put_pixel(_gui->imgplot, i, GUI_PLOT_PAD, grey);
		gdk_image_put_pixel(_gui->imgplot, i + 1,
							GUI_PLOT_PAD +
							GUI_PLOT_VDIVC *
							GUI_PLOT_SUBDIVLEN * 5, grey);
	}
	/* Frame - Left and Right */
	for (i = GUI_PLOT_PAD;
		 i < GUI_PLOT_PAD + GUI_PLOT_VDIVC * GUI_PLOT_SUBDIVLEN * 5; i++) {
		gdk_image_put_pixel(_gui->imgplot, GUI_PLOT_PAD, i + 1, grey);
		gdk_image_put_pixel(_gui->imgplot,
							GUI_PLOT_PAD +
							GUI_PLOT_TDIVC *
							GUI_PLOT_SUBDIVLEN * 5, i, grey);
	}

	/* Vertical Bars */
	for (i = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5;
		 i < GUI_PLOT_PAD + GUI_PLOT_TDIVC * GUI_PLOT_SUBDIVLEN * 5;
		 i += GUI_PLOT_SUBDIVLEN * 5) {
		for (j = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN;
			 j < GUI_PLOT_PAD + GUI_PLOT_VDIVC * GUI_PLOT_SUBDIVLEN * 5;
			 j += GUI_PLOT_SUBDIVLEN) {
			gdk_image_put_pixel(_gui->imgplot, i, j, grey);
		}
	}
	/* Horizontal Bars */
	for (i = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5;
		 i < GUI_PLOT_PAD + GUI_PLOT_VDIVC * GUI_PLOT_SUBDIVLEN * 5;
		 i += GUI_PLOT_SUBDIVLEN * 5) {
		for (j = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN;
			 j < GUI_PLOT_PAD + GUI_PLOT_TDIVC * GUI_PLOT_SUBDIVLEN * 5;
			 j += GUI_PLOT_SUBDIVLEN) {
			gdk_image_put_pixel(_gui->imgplot, j, i, grey);
		}
	}
	/* Dashes - On the vertical axis */
	for (i = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5;
		 i < GUI_PLOT_PAD + GUI_PLOT_VDIVC * GUI_PLOT_SUBDIVLEN * 5;
		 i += GUI_PLOT_SUBDIVLEN * 5) {
		gdk_image_put_pixel(_gui->imgplot,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_TDIVC / 2) - 2,
							i, grey);
		gdk_image_put_pixel(_gui->imgplot,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_TDIVC / 2) - 1,
							i, grey);
		gdk_image_put_pixel(_gui->imgplot,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_TDIVC / 2) + 1,
							i, grey);
		gdk_image_put_pixel(_gui->imgplot,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_TDIVC / 2) + 2,
							i, grey);
	}
	/* Dashes - On the horizontal axis */
	for (i = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5;
		 i < GUI_PLOT_PAD + GUI_PLOT_TDIVC * GUI_PLOT_SUBDIVLEN * 5;
		 i += GUI_PLOT_SUBDIVLEN * 5) {
		gdk_image_put_pixel(_gui->imgplot, i,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_VDIVC / 2) - 2,
							grey);
		gdk_image_put_pixel(_gui->imgplot, i,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_VDIVC / 2) - 1,
							grey);
		gdk_image_put_pixel(_gui->imgplot, i,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_VDIVC / 2) + 1,
							grey);
		gdk_image_put_pixel(_gui->imgplot, i,
							GUI_PLOT_PAD +
							GUI_PLOT_SUBDIVLEN * 5 * (GUI_PLOT_VDIVC / 2) + 2,
							grey);
	}
}

#if DEV
void gui_clearscreen(GdkImage *_img, long _traces[SCP_CHC][GUI_TSCRNW * GUI_TSCRNH])
{

	/* FIXME I should keep a snapshot of an empty screen with its grid
	instead; it would speed things up a good deal */
/* 	bzero(_img->mem, _img->width * _img->height * _img->bpp); */
/* 	for (i = 0; i < _img->height; i++) {
		for (j = 0; j < _img->width; j++) {
			gdk_image_put_pixel(_img, j, i, black);
		}
	} */

	/* Clear separate traces */
/* 	bzero(_traces, SCP_CHC * GUI_TSCRNW * GUI_TSCRNH); */
}
#endif

/* One word about BYTE waveforms: there's a leading VICP packet storing the
number of points. For instance if you fire DTPOINT 2; DTWAVE?, you'll get:
ffffff80 01 00 00 00 00 00 0a 23 38 30 30 30 30 30 30 30 32
ffffff80 01 00 00 00 00 00 02 ffffffeb ffffffeb
ffffff81 01 00 00 00 00 00 01 0a

The first VICP packet is the length in ASCII (yes, even if the waveform is
in BYTE mode). The actual values we're interested in take the second VICP
packet. The third packet is the traditional leave-taking newline.

Note that ASCII waveforms are not lead by this header packet but talk shop
at once:
ffffff80 01 00 00 00 00 00 05 2d 35 33 37 36	<- ASCII Value
ffffff80 01 00 00 00 00 00 01 2c				<- Comma
ffffff80 01 00 00 00 00 00 05 2d 35 33 37 36	<- ASCII Value
ffffff80 01 00 00 00 00 00 01 0a				<- Newline (not comma)
ffffff81 01 00 00 00 00 00 01 0a				<- Leave-taking
*/

/* Called once for each and every channel */
void gui_dtwave(int _last, char *_data, int _len, void *_gui)
{
	int i;
	static int c = 0;
	gui_t *gui;
	long white;
	int x1, y1, x2, y2;
	static int ch = 0;
	static int header = 1; /* Track first waveform header VICP packet */
	static int len; /* Waveform length, as announced by waveform header */
	static int panic = 1; /* Display unexpected waveform message only once */

	/* Convenient */
	gui = _gui;

	/* Colours */
	white = 255 >> (8 - gui->visual->red_prec) << gui->visual->red_shift |
			255 >> (8 - gui->visual->green_prec) << gui->visual->green_shift |
			255 >> (8 - gui->visual->blue_prec) << gui->visual->blue_shift;
	
	/* Let's be cautious and assume a BYTE waveform doesn't necessarily
	fit into a single VICP packet. In this case, it should be sound to
	assume a VICP packet whose header announces less points than the
	waveform header originally promises should be followed by another
	one, without a waveform header. (Since the waveform header is for
	all the points, snappie?) Of course, that's just good sense, no
	proof that's really how it goes. */

	if (header) {
		/* Read ASCII size from first VICP packet */
		_data[_len] = 0;
		_data += 2; /* Skip leading #8 */
		len = atoi(_data);
		header = 0;
	} else if (!_last) {
		if (_len < len) {
			/* Waveform data couldn't fit in single packet */
			memcpy(gui->data + c, _data,
				   _len < GUI_PLOT_PNTCMAX ? _len : GUI_PLOT_PNTCMAX);
			c += _len < GUI_PLOT_PNTCMAX ? _len : GUI_PLOT_PNTCMAX;
		} else if (_len == len) {
			/* Waveform data fits nicely in single packet. We're done. Time
			to plot something on screen. */
			memcpy(gui->data + c, _data,
				   _len < GUI_PLOT_PNTCMAX ? _len : GUI_PLOT_PNTCMAX);
			c += _len < GUI_PLOT_PNTCMAX ? _len : GUI_PLOT_PNTCMAX;
		} else {
			/* Panic */
			if (panic) {
				fprintf(stderr, "\n\
Unexpected waveform format -- Let me explain: it was assumed a waveform\n\
header VICP packet would announce the *total* number of points for this\n\
trace; but now I'm getting a single VICP packet with *more* points than\n\
the aforexpected total.\n\
\n\
Tell you what: I'll just try to plot the waveform like it was perfectly\n\
normal, but you should have someone look into this because it's definitely\n\
fishy. I won't repeat this message again. Enough bad news for today, \
says I.\n");
				panic = 0;
			}
			memcpy(gui->data + c, _data,
				   _len < GUI_PLOT_PNTCMAX ? _len : GUI_PLOT_PNTCMAX);
			c += _len;
		}
	} else {
		gdk_threads_enter();

		/* Clear screen */
		if (ch == 0) {
			memset(gui->imgplot->mem, 0,
				   gui->imgplot->width *
				   gui->imgplot->height *
				   gui->imgplot->bpp);
		}
		/* Plot points */
		for (i = 0; i < c - 1; i++) {
			x1 = round((float) i *
					   (GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_TDIVC) /
					   (c - 1));
			x2 = round((float) (i + 1) *
					   (GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_TDIVC) /
					   (c - 1));
/* 			y1 = round((float) (gui->data[i] + 32768) *
					   GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_VDIVC /
					   65535);
			y2 = round((float) (gui->data[i + 1] + 32768) *
					   GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_VDIVC /
					   65535); */
			y1 = round((float) (gui->data[i] + 128) *
					   GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_VDIVC /
					   255);
			y2 = round((float) (gui->data[i + 1] + 128) *
					   GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_VDIVC /
					   255);
/* 			printf("%d, %d\n", gui->data[i], gui->data[i + 1]); */
			gui_drawline(gui->imgplot,
						 gui->tras[ch],
						 gui->clrs[ch],
						 x1 + GUI_PLOT_PAD,
						 GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5 *
						 GUI_PLOT_VDIVC - y1, 
						 x2 + GUI_PLOT_PAD,
						 GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5 *
						 GUI_PLOT_VDIVC - y2);

			/* XXX Draw point */
/* 			gdk_image_put_pixel(gui->imgplot,
								x1 + GUI_PLOT_PAD,
								GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5 *
								GUI_PLOT_VDIVC - y1,
								gui->clrs[2]);  */

			/* Register trace drawings (for touchscreen, etc.) */
/* 			x = x1 + GUI_PLOT_PAD;
			y = GUI_PLOT_PAD + GUI_PLOT_SUBDIVLEN * 5 * GUI_PLOT_VDIVC - y1;
			gui->traces[ch][GUI_TSCRNW * y + x] = colours[ch]; */
		}
		if (ch == 3) {
			gui_drawgrid(gui);
			/* Eventually update screen */
			gtk_widget_queue_draw(gui->evtboxplot);
		}
		gdk_flush();
		gdk_threads_leave();
		ch = (ch + 1) % 4;
		c = 0;
		header = 1;
	}
}

void gui_trmdsingle(int _last, char *_data, int _len, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = (gui_t *) _gui;

	/* Get rid of trailing newline */
	_data[_len - 1] = 0;

	/* Take actions according to trigger mode */
	if (strcmp(_data, "STOP") == 0) {
		/* Switch on LED */
		gdk_threads_enter();
		gtk_image_set_from_pixbuf(GTK_IMAGE(gui->imgled), gui->pbfledon);
		gdk_flush();
		gdk_threads_leave();

		/* Get new waves */
		scp_cmd_push(gui->scp, "dtform byte", NULL, NULL);
		scp_cmd_push(gui->scp, "wavesrc CH1", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
		scp_cmd_push(gui->scp, "wavesrc CH2", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
		scp_cmd_push(gui->scp, "wavesrc CH3", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
		scp_cmd_push(gui->scp, "wavesrc CH4", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);

		/* Make the blinking visible */
		usleep(100000);

		/* Switch off LED and stop looking for a trigger */
		gdk_threads_enter();
		gtk_image_set_from_pixbuf(GTK_IMAGE(gui->imgled), gui->pbfledoff);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->rdostop), TRUE);
		gdk_flush();
		gdk_threads_leave();
	}
}

void gui_trmdnormal(int _last, char *_data, int _len, void *_gui)
{
	gui_t *gui;
	/* char query[] = "wavesrc CH1; dtwave?; wavesrc CH2; dtwave?;\ 
wavesrc CH3; dtwave?; wavesrc CH4; dtwave?"; */

	/* Convenient */
	gui = (gui_t *) _gui;

	/* Get rid of trailing newline */
	_data[_len - 1] = 0;

	/* Take actions according to trigger mode */
	if (strcmp(_data, "STOP") == 0) {
/* 		printf("Trig'd at %ld\n", time(NULL)); */
		/* Switch on LED */
		gdk_threads_enter();
		gtk_image_set_from_pixbuf(GTK_IMAGE(gui->imgled), gui->pbfledon);
		gdk_flush();
		gdk_threads_leave();

		/* Get new screenshot */
/* 		scp_cmd_push(gui->scp, buf, gui_newdtwave, gui); */
		scp_cmd_push(gui->scp, "dtform byte", NULL, NULL);
		scp_cmd_push(gui->scp, "wavesrc CH1", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
		scp_cmd_push(gui->scp, "wavesrc CH2", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
		scp_cmd_push(gui->scp, "wavesrc CH3", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
		scp_cmd_push(gui->scp, "wavesrc CH4", NULL, NULL);
		scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
		scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);

		/* Make the blinking visible */
		usleep(100000);

		/* Switch off LED and stop looking for a trigger */
		gdk_threads_enter();
		gtk_image_set_from_pixbuf(GTK_IMAGE(gui->imgled), gui->pbfledoff);
		gdk_flush();
		gdk_threads_leave();

		/* Back to single mode */
		scp_cmd_push(gui->scp, "trmd SINGLE", NULL, NULL);
	}
}

void gui_vdiv(int _islast, char *_data, int _len, void *_chl)
{
	char buf0[SMLBUFSIZE], buf1[SMLBUFSIZE];	/* GP Buffers */
	int i;
	double d;
	gui_chl_t *chl;

	/* Convenient */
	chl = _chl;

	/* Get data from scope */
	d = nb_engtod(_data);
	nb_eng(d, 3, 0, buf0);
	snprintf(buf1, SMLBUFSIZE, "%sV", buf0);

	/* Find corresponding menu index */
	for (i = 0; i < GUI_VDIVC; i++) {
		if (strcmp(buf1, g_strvdivs[i]) == 0){
			g_locked = 1;
			gdk_threads_enter();
			gtk_option_menu_set_history(GTK_OPTION_MENU(chl->optvdiv), i);
			gdk_flush();
			gdk_threads_leave();
			g_locked = 0;
			break;
		}
	}
}

void gui_ofst(int _last, char *_data, int _len, void *_chl)
{
	char buf0[SMLBUFSIZE], buf1[SMLBUFSIZE];	/* GP Buffers */
	double dbl;
	gui_chl_t *chl;

	/* Convenient */
	chl = _chl;

	/* Get data from scope */
	dbl = nb_engtod(_data);
	if (dbl != 0.) {
		nb_eng(dbl, 3, 0, buf0);
		snprintf(buf1, SMLBUFSIZE, "%sV", buf0);
	} else {
		snprintf(buf1, SMLBUFSIZE, "0 V");
	}

	/* Update GUI */
	g_locked = 1;
	gdk_threads_enter();
	gtk_label_set_text(GTK_LABEL(chl->lblofst), buf1);
	gdk_flush();
	gdk_threads_leave();
	g_locked = 0;
}

/* Looping plot thread routine */
enum { TRMD_AUTO, TRMD_NORMAL, TRMD_SINGLE, TRMD_STOP };
void *gui_loopplot(void *_gui)
{
	gui_t *gui;
	GtkRadioButton *rdoauto, *rdonormal, *rdosingle, *rdostop;
	int trmd;

	/* Convenient */
	gui = (gui_t *) _gui;
	rdoauto = GTK_RADIO_BUTTON(gui->rdoauto);
	rdonormal = GTK_RADIO_BUTTON(gui->rdonormal);
	rdosingle = GTK_RADIO_BUTTON(gui->rdosingle);
	rdostop = GTK_RADIO_BUTTON(gui->rdostop);

	while (!gui->plotkill) {
		/* Check trmd */
		gdk_threads_enter();
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdoauto))) {
			trmd = TRMD_AUTO;
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdonormal))) {
			trmd = TRMD_NORMAL;
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdosingle))) {
			trmd = TRMD_SINGLE;
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rdostop))) {
			trmd = TRMD_STOP;
		}
		gdk_threads_leave();

		/* Take actions */
		if (trmd == TRMD_AUTO) {
			scp_cmd_push(gui->scp, "dtform byte", NULL, NULL);
/* 			scp_cmd_push(gui->scp, "dtform ascii", NULL, NULL); */
			scp_cmd_push(gui->scp, "wavesrc CH1", NULL, NULL);
			scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
			scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
			scp_cmd_push(gui->scp, "wavesrc CH2", NULL, NULL);
			scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
			scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
			scp_cmd_push(gui->scp, "wavesrc CH3", NULL, NULL);
			scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
			scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
			scp_cmd_push(gui->scp, "wavesrc CH4", NULL, NULL);
			scp_cmd_push(gui->scp, SCP_DTPOINTS, NULL, NULL);
			scp_cmd_push(gui->scp, "dtwave?", gui_dtwave, gui);
			usleep(GUI_PLTM);
		} else if (trmd == TRMD_NORMAL) {
			scp_cmd_push(gui->scp, "trmd?", gui_trmdnormal, gui);
			usleep(GUI_TRCK);
		} else if (trmd == TRMD_SINGLE) {
			scp_cmd_push(gui->scp, "trmd?", gui_trmdsingle, gui);
			usleep(GUI_TRCK);
		} else if (trmd == TRMD_STOP) {
			pthread_mutex_lock(&gui->mtxloopplot);
			pthread_cond_wait(&gui->cndloopplot, &gui->mtxloopplot);
			pthread_mutex_unlock(&gui->mtxloopplot);
		}
	}
	return NULL;
}

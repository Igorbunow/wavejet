#include "cbk.h"

int g_locked = 0;
#define BUFSIZE 64

/* Window deletion with the quit menu itme */
void cbk_quit(GtkWidget *_wgt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	#ifdef NICE
	/* Make sure the queue is empty */
/* 	if (gui->scp) {
		pthread_mutex_lock(&gui->scp->mtxquit);
		printf("passed the mutex lock\n");
		pthread_cond_wait(&gui->scp->cndquit, &gui->scp->mtxquit);
		printf("passed the condwait\n");
		pthread_mutex_unlock(&gui->scp->mtxquit);
	} */

	/* Terminate threads */
	gui->loopkill = 1;
	pthread_mutex_lock(&gui->mtxloopplot);
	pthread_cond_signal(&gui->cndloopplot);
	pthread_mutex_unlock(&gui->mtxloopplot);

	/* Clean up */
	if (gui->scp) {
		/* FIXME Could block again here */
		pthread_join(gui->thdloopplot, NULL);
		close(gui->scp->sockfd);
	}
	#endif

	/* Quit main event loop */
	gtk_main_quit();
}

/* Window deletion with the close button */
void cbk_delete(GtkWidget *_wgt, GdkEvent *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	#ifdef NICE
	/* Make sure the queue is empty */
/* 	if (gui->scp) {
		pthread_mutex_lock(&gui->scp->mtxquit);
		printf("passed the mutex lock\n");
		pthread_cond_wait(&gui->scp->cndquit, &gui->scp->mtxquit);
		printf("passed the condwait\n");
		pthread_mutex_unlock(&gui->scp->mtxquit);
	} */

	/* Terminate threads */
	gui->loopkill = 1;
	pthread_mutex_lock(&gui->mtxloopplot);
	pthread_cond_signal(&gui->cndloopplot);
	pthread_mutex_unlock(&gui->mtxloopplot);

	/* Clean up */
	if (gui->scp) {
		/* FIXME Could block again here */
		pthread_join(gui->thdloopplot, NULL);
		close(gui->scp->sockfd);
	}
	#endif

	/* Quit main event loop */
	gtk_main_quit();
}

#ifdef DEV
void cbk_tscrn(GtkWidget *_wgt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	/* Set status bar */
	gtk_statusbar_push(GTK_STATUSBAR(gui->stsbartscrn),
					   gui->ctxidtscrn,
					   "Retrieving screenshot...");
	/* Disable toolbar button to avoid throttling */
	gtk_widget_set_sensitive(gui->btnrfstscrn, FALSE);

	/* Update display */
	scp_cmd_queue(gui->scp, "trmd NORMAL", NULL, NULL); 
/* 	scp_cmd_queue(gui->scp, SCP_DTPOINTS, NULL, NULL); */
	scp_cmd_queue(gui->scp, "tscrn? PNG", gui_tscrn, _gui);
/* 	scp_cmd_queue(gui->scp, "*cls", gui_tscrn, _gui); */
}
#endif

#ifdef DEV
void cbk_select(GtkWidget *_widget, GdkEventMotion *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	gui->x = _evt->x;
	gui->y = _evt->y;
	gui_drawplot(_gui);
}
#endif

#if DEV
void cbk_dropselection(GtkWidget *_widget, GdkEventButton *_evt, void *_gui)
{
	gui_t *gui;
	double dbl;
	char buf[BUFSIZE];
	double dbltdiv;
	const char *strtdiv;

	((gui_t *) _gui)->ox = -1;
	((gui_t *) _gui)->oy = -1;

	/* Convenient */
	gui = _gui;

	/* Clear previous image changes */
	memcpy(gui->gdkimgscrn->mem,
		   gui->pixels,
		   gui->gdkimgscrn->width *
		   gui->gdkimgscrn->height *
		   gui->gdkimgscrn->bpp);
	/* Update display */
	gtk_widget_queue_draw(gui->evtboxscrn);

	/* Compute tdiv */
	strtdiv = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(gui->cbotdiv)->entry));
	dbltdiv = nb_engtod(strtdiv);

	/* How many divisions selected? */
	dbl = (double) abs(_evt->x - GUI_TSCRN_HMIDDLE) * 2 / 50 * dbltdiv / 10;
	dbl = scp_fto125(dbl);
	nb_eng(dbl, 3, 0, buf);

	/* Apply changes to scope */
	sprintf(buf, "tdiv %.10f", dbl);
	scp_cmd_queue(gui->scp, buf, NULL, NULL);
	scp_cmd_queue(gui->scp, "tdiv?", gui_get_tdiv, gui); 
}
#endif

/* GP Handler for Options (i.e. Popup Menus) */
void cbk_optacn(GtkOptionMenu *_opt, void *_gui)
{
	char buf[BUFSIZE];
	const char *wgtname;
	gui_t *gui;
	int idx;

	/* Allow action only if user asked for it (i,e. not programmatically) */
	if (g_locked) {
		return;
	}

	/* Convenient */
	gui = _gui;

	/* Get widget name to know what to do with it */
	wgtname = gtk_widget_get_name(GTK_WIDGET(_opt));

	if (strcmp(wgtname, "opttdiv") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "tdiv %.5e", nb_engtod(g_strtdivs[idx]));
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "tdiv?", gui_tdiv, gui); 
	} else if (strcmp(wgtname, "optvdiv1") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "c1:vdiv %.5e", nb_engtod(g_strvdivs[idx]));
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "c1:vdiv?", gui_vdiv, &gui->chls[0]); 
	} else if (strcmp(wgtname, "optvdiv2") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "c2:vdiv %.5e", nb_engtod(g_strvdivs[idx]));
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "c2:vdiv?", gui_vdiv, &gui->chls[1]); 
	} else if (strcmp(wgtname, "optvdiv3") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "c3:vdiv %.5e", nb_engtod(g_strvdivs[idx]));
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "c3:vdiv?", gui_vdiv, &gui->chls[2]); 
	} else if (strcmp(wgtname, "optvdiv4") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "c4:vdiv %.5e", nb_engtod(g_strvdivs[idx]));
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "c4:vdiv?", gui_vdiv, &gui->chls[3]); 
	} else if (strcmp(wgtname, "opttsrc") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "tsrc %s", g_strtsrcs[idx]);
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "tsrc?", gui_tsrc, gui); 
	} else if (strcmp(wgtname, "opttcpl") == 0) {
		/* Get value from widget */
		idx = gtk_option_menu_get_history(_opt);

		/* Compose query */
		snprintf(buf, BUFSIZE, "tcpl %s", g_strtcpls[idx]);
		scp_cmd_push(gui->scp, buf, NULL, NULL);
		scp_cmd_push(gui->scp, "tcpl?", gui_tcpl, gui); 
	} else {
		#if DEBUG
		printf("Event not handled yet. "); 
		printf("Yes, that's a bug, a filthy little cockroach!\n");
		#endif
	}
}

/* Non-combo actions (i.e triggered only once per event) */
int cbk_usracn(GtkWidget *_wgt, GdkEventButton *_evt, void *_gui)
{
	const char *wgtname;
	gui_t *gui;
	const char *str;
	double dbltlvl, dbltrdl, dbltdiv;
	char buf0[BUFSIZE], buf1[BUFSIZE];
	int idx;
	char query[32];
	gui_chl_t *chl;

	/* Allow action only if user asked for it (i,e. not programmatically) */
	if (g_locked) {
		return 0;
	}

	/* Convenient */
	gui = _gui;

	/* Get widget name to know what to do with it */
	wgtname = gtk_widget_get_name(_wgt);

	if (strcmp(wgtname, "btnleft") == 0) {
		/* Get current trigger delay */
		str = gtk_label_get_text(GTK_LABEL(gui->lbltrdl));
		dbltrdl = nb_engtod(str);

		/* Get tdiv */
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(gui->opttdiv));
		dbltdiv = nb_engtod(g_strtdivs[idx]);

		dbltrdl -= dbltdiv;
		if (dbltrdl != 0.) {
			nb_eng(dbltrdl, 3, 0, buf0);
			snprintf(buf1, BUFSIZE, "%ss", buf0);
		} else {
			snprintf(buf1, BUFSIZE, "0 s");
		}
		gtk_label_set_text(GTK_LABEL(gui->lbltrdl), buf1);

		/* Change trigger delay on scope */
		snprintf(buf1, BUFSIZE, "trdl %.10f", dbltrdl);
		scp_cmd_push(gui->scp, buf1, NULL, NULL);
		scp_cmd_push(gui->scp, "trdl?", gui_trdl, gui); 
	} else if (strcmp(wgtname, "btnright") == 0) {
		/* Get current trigger delay */
		str = gtk_label_get_text(GTK_LABEL(gui->lbltrdl));
		dbltrdl = nb_engtod(str);

		/* Get tdiv */
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(gui->opttdiv));
		dbltdiv = nb_engtod(g_strtdivs[idx]);

		dbltrdl += dbltdiv;
		if (dbltrdl != 0.) {
			nb_eng(dbltrdl, 3, 0, buf0);
			snprintf(buf1, BUFSIZE, "%ss", buf0);
		} else {
			snprintf(buf1, BUFSIZE, "0s");
		}
		gtk_label_set_text(GTK_LABEL(gui->lbltrdl), buf1);

		/* Change trigger delay on scope */
		snprintf(buf1, BUFSIZE, "trdl %.10f", dbltrdl);
		scp_cmd_push(gui->scp, buf1, NULL, NULL);
		scp_cmd_push(gui->scp, "trdl?", gui_trdl, gui); 
	} else if (strcmp(wgtname, "scltlvl") == 0) {
		/* Get tlvllbl value */
		dbltlvl = gtk_range_get_value(GTK_RANGE(_wgt));

		/* Get tsrc */
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(gui->opttsrc));

		/* Compute values depending on tsrc */
		if (strcmp(g_strtsrcs[idx], "CH1") == 0) {
			chl = &gui->chls[0];
			idx =
				gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
			nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
			sprintf(query, "tlvl %e",
					dbltlvl * nb_engtod(g_strvdivs[idx]) * 4);
		} else if (strcmp(g_strtsrcs[idx], "CH2") == 0) {
			chl = &gui->chls[1];
			idx =
				gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
			nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
			sprintf(query, "tlvl %e",
					dbltlvl * nb_engtod(g_strvdivs[idx]) * 4);
		} else if (strcmp(g_strtsrcs[idx], "CH3") == 0) {
			chl = &gui->chls[2];
			idx =
				gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
			nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
			sprintf(query, "tlvl %e",
					dbltlvl * nb_engtod(g_strvdivs[idx]) * 4);
		} else if (strcmp(g_strtsrcs[idx], "CH4") == 0) {
			chl = &gui->chls[3];
			idx =
				gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
			nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
			sprintf(query, "tlvl %e",
					dbltlvl * nb_engtod(g_strvdivs[idx]) * 4);
		} else if (strcmp(g_strtsrcs[idx], "EXT") == 0) {
			nb_eng(dbltlvl * .5, 3, 0, buf0);
			sprintf(query, "tlvl %e", dbltlvl * .5);
		} else if (strcmp(g_strtsrcs[idx], "EXT10") == 0) {
			nb_eng(dbltlvl * 5., 3, 0, buf0);
			sprintf(query, "tlvl %e", dbltlvl * 5.);
		}

		/* Set tlvllbl value */
		snprintf(buf1, BUFSIZE, "%sV", buf0);
		gtk_label_set_text(GTK_LABEL(gui->lbltlvl), buf1);

		/* Set tlvl value to scope */
		scp_cmd_push(gui->scp, query, NULL, NULL);
		scp_cmd_push(gui->scp, "tlvl?", gui_tlvl, gui); 
	}

	/* For the mouse to let go of the slider */
	return 0;
}

/* Channel Toggles */
void cbk_tglacn(GtkToggleButton *_tgl, void *_gui)
{
	int b;	/* GP Boolean */
	gui_t *gui;
	const char *wgtname;

	/* Allow action only if user asked for it (i.e. not programmatically) */
	if (g_locked) {
		return;
	}

	/* Convenient */
	gui = _gui;

	/* Get widget name to know what to do with it */
	wgtname = gtk_widget_get_name(GTK_WIDGET(_tgl));

	if (strcmp(wgtname, "tglbtn1") == 0) {
		b = gtk_toggle_button_get_active(_tgl);
		if (b) {
			scp_cmd_push(gui->scp, "c1:tra ON", NULL, NULL);
		} else {
			scp_cmd_push(gui->scp, "c1:tra OFF", NULL, NULL);
		}
		scp_cmd_push(gui->scp, "c1:tra?", gui_tra, _tgl); 
	} else if (strcmp(wgtname, "tglbtn2") == 0) {
		b = gtk_toggle_button_get_active(_tgl);
		if (b) {
			scp_cmd_push(gui->scp, "c2:tra ON", NULL, NULL);
		} else {
			scp_cmd_push(gui->scp, "c2:tra OFF", NULL, NULL);
		}
		scp_cmd_push(gui->scp, "c2:tra?", gui_tra, _tgl); 
	} else if (strcmp(wgtname, "tglbtn3") == 0) {
		b = gtk_toggle_button_get_active(_tgl);
		if (b) {
			scp_cmd_push(gui->scp, "c3:tra ON", NULL, NULL);
		} else {
			scp_cmd_push(gui->scp, "c3:tra OFF", NULL, NULL);
		}
		scp_cmd_push(gui->scp, "c3:tra?", gui_tra, _tgl); 
	} else if (strcmp(wgtname, "tglbtn4") == 0) {
		b = gtk_toggle_button_get_active(_tgl);
		if (b) {
			scp_cmd_push(gui->scp, "c4:tra ON", NULL, NULL);
		} else {
			scp_cmd_push(gui->scp, "c4:tra OFF", NULL, NULL);
		}
		scp_cmd_push(gui->scp, "c4:tra?", gui_tra, _tgl); 
	} else if (strcmp(wgtname, "rdoauto") == 0) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_tgl))) {
			scp_cmd_push(gui->scp, "trmd AUTO", NULL, NULL);
		}
	} else if (strcmp(wgtname, "rdonormal") == 0) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_tgl))) {
			scp_cmd_push(gui->scp, "trmd SINGLE", NULL, NULL);
		}
	} else if (strcmp(wgtname, "rdosingle") == 0) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_tgl))) {
			scp_cmd_push(gui->scp, "trmd SINGLE", NULL, NULL);
		}
	} else if (strcmp(wgtname, "rdostop") == 0) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_tgl))) {
			scp_cmd_push(gui->scp, "trmd STOP", NULL, NULL);
		} else {
			pthread_mutex_lock(&gui->mtxloopplot);
			pthread_cond_signal(&gui->cndloopplot);
			pthread_mutex_unlock(&gui->mtxloopplot);
		}
	}
}

/* Handler for Vertical Scale */
void cbk_scl(GtkRange *_range, void *_gui)
{
	double dbltlvl;
	int idx;
	gui_t *gui;
	char buf0[BUFSIZE], buf1[BUFSIZE];
	gui_chl_t *chl;

	/* Allow action only if user asked for it (i.e. not programmatically) */
	if (g_locked) {
		return;
	}

	/* Convenient */
	gui = _gui;

	/* Get tlvllbl value */
	dbltlvl = gtk_range_get_value(_range);

	/* Get tsrc */
	idx = gtk_option_menu_get_history(GTK_OPTION_MENU(gui->opttsrc));

	/* Compute values depending on tsrc */
	if (strcmp(g_strtsrcs[idx], "CH1") == 0) {
		chl = &gui->chls[0];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
	} else if (strcmp(g_strtsrcs[idx], "CH2") == 0) {
		chl = &gui->chls[0];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
	} else if (strcmp(g_strtsrcs[idx], "CH3") == 0) {
		chl = &gui->chls[0];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
	} else if (strcmp(g_strtsrcs[idx], "CH4") == 0) {
		chl = &gui->chls[0];
		idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
		nb_eng(dbltlvl * nb_engtod(g_strvdivs[idx]) * 4, 3, 0, buf0);
	} else if (strcmp(g_strtsrcs[idx], "EXT") == 0) {
		nb_eng(dbltlvl * .5, 3, 0, buf0);
	} else if (strcmp(g_strtsrcs[idx], "EXT10") == 0) {
		nb_eng(dbltlvl * 5., 3, 0, buf0);
	}

	/* Set tlvllbl value */
	snprintf(buf1, BUFSIZE, "%sV", buf0);
	gtk_label_set_text(GTK_LABEL(gui->lbltlvl), buf1);
}

/* Change value by +/- vdiv/10 */
gboolean cbk_btnrlsofst(GtkWidget *_arw, GdkEventButton *_evt, void *_chl)
{
	int idx;
	gui_chl_t *chl;
	char buf0[BUFSIZE], buf1[BUFSIZE];
	const char *name;
	const char *txt;
	double dblofst, dblvdiv, dbl;

	/* Convenient */
	chl = _chl;

	/* Get widget name */
	name = gtk_widget_get_name(_arw);

	/* Get vdiv */
	idx = gtk_option_menu_get_history(GTK_OPTION_MENU(chl->optvdiv));
	dblvdiv = nb_engtod(g_strvdivs[idx]);

	/* Get ofst */
	txt = gtk_label_get_text(GTK_LABEL(chl->lblofst));
	dblofst = nb_engtod(txt);

	/* Up or down? Rock the ship, Capt'n! */
	if (strncmp(name, "btnup", strlen("btnup")) == 0) {
		dbl = dblofst + dblvdiv / 10;
	} else if (strncmp(name, "btndn", strlen("btndn")) == 0) {
		dbl = dblofst - dblvdiv / 10;
	}
	/* FIXME It's DIR-TY! Handle floats more nicely */
	if (dbl != 0.) {
		nb_eng(dbl, 3, 0, buf0);
	} else {
		snprintf(buf0, BUFSIZE, "0");
	}
	snprintf(buf1, BUFSIZE, "%sV", buf0);
	gtk_label_set_text(GTK_LABEL(chl->lblofst), buf1);

	/* Update to scope */
	snprintf(buf1, BUFSIZE, "c%d:ofst %f", chl->id, dbl);
	scp_cmd_push(chl->scp, buf1, NULL, NULL);
	snprintf(buf1, BUFSIZE, "c%d:ofst?", chl->id);
	scp_cmd_push(chl->scp, buf1, gui_ofst, chl);

	/* Don't block signal or the button will behave queerly */
	return FALSE;
}

/* Touch Screen */
#ifdef DEV
gboolean cbk_mtnplot(GtkWidget *_wgt, GdkEventMotion *_evt, void *_gui)
{
	#if 0
	int i, j, k;
	gui_t *gui;
	int idx;
/* 	unsigned long clr;
	int r, g, b; */

	/* Convenient */
	gui = _gui;

	/* Don't allow to go past actual drawing area */
	if (_evt->x < GUI_PLOT_PAD || _evt->x > GUI_TSCRNW - GUI_PLOT_PAD) {
		return TRUE;
	}
	if (_evt->y < GUI_PLOT_PAD || _evt->y > GUI_TSCRNH - GUI_PLOT_PAD) {
		return TRUE;
	}

	/* Register origin and catch a trace */
	if (gui->ox == -1 || gui->oy == -1 || gui->tra == -1) {
		gui->ox = _evt->x;
		gui->oy = _evt->y;

		idx = _evt->y * GUI_TSCRNW + _evt->x;
		for (k = 0; k < SCP_CHC; k++) {
			if (gui->tras[k][idx] != 0 || gui->tras[k][idx - 1] != 0 ||
				gui->tras[k][idx + 1] != 0 ||
				gui->tras[k][idx - GUI_TSCRNW] != 0 ||
				gui->tras[k][idx + GUI_TSCRNW] != 0) {
				printf("clicked %d\n", k + 1);
				gui->tra = k;
				break;
			}
		}
	}

	/* Draw plot again */
	memset(gui->imgplot->mem, 0,
		   gui->imgplot->width *
		   gui->imgplot->height *
		   gui->imgplot->bpp);
	/* Draw other traces */
/* 	for (k = 0; k < SCP_CHC; k++) {
		if (k != gui->tra) {
			for (i = 0; i < GUI_TSCRNH; i++) {
				for (j = 0; j < GUI_TSCRNW; j++) {
					if (gui->tras[k][GUI_TSCRNW * i + j] != 0) {
						gdk_image_put_pixel(gui->imgplot, j, i,
											gui->clrs[k]);
					}
				}
			}
		}
	} */
	/* Draw trace being moved */
	for (i = GUI_PLOT_PAD + 1; i < GUI_TSCRNW - GUI_PLOT_PAD - 1; i++) {
		gdk_image_put_pixel(gui->imgplot, i,
							gui->tras[gui->tra][i] + _evt->y - gui->oy,
							gui->clrs[gui->tra]);
		
	}
/* 	for (i = 0; i < GUI_TSCRNH; i++) {
		for (j = 0; j < GUI_TSCRNW; j++) {
			if (gui->tras[gui->tra][GUI_TSCRNW * i + j] != 0) {
				if (i + _evt->y - gui->oy > GUI_PLOT_PAD &&
					i + _evt->y - gui->oy < GUI_TSCRNH - GUI_PLOT_PAD - 1) {
					gdk_image_put_pixel(gui->imgplot,
										j, i + _evt->y - gui->oy,
										gui->clrs[gui->tra]);
				}
			}
		}
	} */
	gui_drawgrid(gui);
	gtk_widget_queue_draw(gui->evtboxplot);
	#endif

	return TRUE;
}

gboolean cbk_rlsplot(GtkWidget *_wgt, GdkEventKey *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	/* Reset the lot */
	gui->ox = -1;
	gui->oy = -1;
	gui->tra = -1;

	return TRUE;
}
#endif

/* Menu Actions */
#if DEV
void cbk_acnmnu(GtkMenuItem *_itm, void *_gui)
{
	int i, j;
	gui_t *gui;

	gui = _gui;

	for (i = 0; i < GUI_TSCRNH; i++) {
		for (j = 0; j < GUI_TSCRNW; j++) {
			if (gui->tras[0][i * GUI_TSCRNW + j] != 0) {
			}
		}
		printf("\n");
	}
}
#endif

void cbk_connect(GtkMenuItem *_itm, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	/* Connecting thread (the same as in main.c) */
	pthread_create(&gui->thdconnect, NULL, gui_connect, gui);
}

/* Dialogs */
/* Upon on-the-blink-scope error dialog dismissal */
void cbk_rpsdlg(GtkDialog *_dlg, int _arg1, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	/* TODO Offer to reconnect */
	gtk_widget_destroy(GTK_WIDGET(_dlg));
}

/*** Windows **************************************************************/

/* Upon window creation */

/* Re-read RC file and opens prefs window */
void cbk_opnwinprfs(GtkMenuItem *_itm, void *_gui)
{
	gui_t *gui;
	int retval;

	/* Convenient */
	gui = _gui;

	/* Read RC file */
	retval = prf_read(&gui->prfs);

	/* Set all the values we need */
	if (retval == 0) {
		retval = gui_winprfs_set(gui);
	}
	if (retval < 0) {
		gtk_widget_show(gui->winprfs);
	}

	gtk_widget_show(gui->winprfs);
}

/* Upon window deletion */
/* Granted, the response handler does that too. But what the response
 * handler doesn't do is to prevent window destruction such as we do
 * her by returning true. */
gboolean cbk_delwinprfs(GtkWidget *_wgt, GdkEvent *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	gtk_widget_hide(_wgt);

	/* Don't actually delete window (from memory, that is) */
	return TRUE;
}

void cbk_rspwinprfs(GtkDialog *_dlg, int _rsp, void *_gui)
{
	gui_t *gui;
	const char *val; /* GP Value */
	int status = 0;	/* Error status */

	/* Convenient */
	gui = _gui;

	if (_rsp == GTK_RESPONSE_ACCEPT) {
		/* Alter settings */
		val = gtk_entry_get_text(GTK_ENTRY(gui->etyaddr));
		if (val[0] == 0) {
			gtk_widget_show(gui->imgaddr);
			status = -1;
		} else {
			gtk_widget_hide(gui->imgaddr);
			prf_set(&gui->prfs, "addr", val);
		}

		val = gtk_entry_get_text(GTK_ENTRY(gui->etyport));
		if (val[0] == 0) {
			gtk_widget_show(gui->imgport);
			status = -1;
		} else {
			gtk_widget_hide(gui->imgport);
			prf_set(&gui->prfs, "port", val);
		}

		/* Print new settings to file */
		if (status == 0) {
			prf_write(&gui->prfs);
		} else {
			return;
		}
	}

	/* Hide window */
	gtk_widget_hide(GTK_WIDGET(_dlg));
}

#if DEV
gboolean cbk_delwinplot(GtkWidget *_wgt, GdkEvent *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	gtk_widget_hide(_wgt);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gui->itmplot), FALSE);

	/* Don't actually delete window (from memory, that is) */
	return TRUE;
}

gboolean cbk_delwinscrn(GtkWidget *_wgt, GdkEvent *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	gtk_widget_hide(_wgt);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gui->itmscrn), FALSE);

	/* Don't actually delete window (from memory, that is) */
	return TRUE;
}

gboolean cbk_delwininfo(GtkWidget *_wgt, GdkEvent *_evt, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	gtk_widget_hide(_wgt);
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(gui->itminfo), FALSE);

	/* Don't actually delete window (from memory, that is) */
	return TRUE;
}

void cbk_tglwinplot(GtkMenuItem *_itm, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(_itm))) {
		gtk_widget_show(gui->winplot);
	} else {
		gtk_widget_hide(gui->winplot);
	}
}

void cbk_tglwinscrn(GtkMenuItem *_itm, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(_itm))) {
		gtk_widget_show(gui->winscrn);
	} else {
		gtk_widget_hide(gui->winscrn);
	}
}

void cbk_tglwininfo(GtkMenuItem *_itm, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(_itm))) {
		gtk_widget_show(gui->wininfo);
	} else {
		gtk_widget_hide(gui->wininfo);
	}
}

void cbk_plotszealloc(GtkWidget *_alignment, GtkAllocation *_alloc, void *_gui)
{
	gui_t *gui;

	/* Convenient */
	gui = _gui;
	
	/* New Image */
/* 	g_object_unref(gui->gdkimgplot);
	gui->gdkimgplot = gdk_image_new(GDK_IMAGE_FASTEST,
									gui->visual,
									_alloc->width,
									_alloc->height); */
/* 	printf("%d %d\n", _alloc->width, _alloc->height); */
	printf("%p\n", (void *) _alignment);
/* 	g_signal_handler_block(_alignment, gui->hidplotszealloc); */
	g_signal_handler_disconnect(G_OBJECT(_alignment), gui->hidplotszealloc);
	gtk_image_set_from_image(GTK_IMAGE(gui->gtkimgplot), gui->gdkimgplot, NULL);
	usleep(500000);
	gui->hidplotszealloc = g_signal_connect(G_OBJECT(_alignment),
											"size_allocate",
											G_CALLBACK(cbk_plotszealloc),
											gui);
/* 	g_signal_handler_unblock(_alignment, gui->hidplotszealloc); */
}
#endif

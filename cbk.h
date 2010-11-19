#ifndef WJ_CBK
#define WJ_CBK

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

#include "scp.h"
#include "gui.h"
#include "numbers.h"

/* Widget Actions */
void cbk_optacn(GtkOptionMenu *_opt, void *_gui);
int cbk_usracn(GtkWidget *_wgt, GdkEventButton *_evt, void *_gui);
void cbk_tglacn(GtkToggleButton *_tgl, void *_gui);
void cbk_scl(GtkRange *_range, void *_gui);
gboolean cbk_btnrlsofst(GtkWidget *_arw, GdkEventButton *_evt, void *_data);

/* Menu Actions */
void cbk_connect(GtkMenuItem *_itm, void *_gui);
void cbk_disconnect(GtkMenuItem *_itm, void *_gui);
void cbk_opnwinprfs(GtkMenuItem *_itm, void *_gui);
void cbk_about(GtkMenuItem *_i, void *_gui);

/* Dialog Responses */
void cbk_rpsdlg(GtkDialog *_dlg, int _arg1, void *_gui);
void cbk_rspwinprfs(GtkDialog *_dlg, int _rsp, void *_gui);

/* Windows Deletion */
void cbk_quit(GtkWidget *_wgt, void *_gui);
void cbk_delete(GtkWidget *_wgt, GdkEvent *_evt, void *_gui);
gboolean cbk_delwinprfs(GtkWidget *_wgt, GdkEvent *_evt, void *_gui);
gboolean cbk_delwinabout(GtkWidget *_wgt, GdkEvent *_evt, void *_nothing);

/* Future development functions */
#ifdef DEV
void cbk_tscrn(GtkWidget *_wgt, void *_gui);
void cbk_select(GtkWidget *_widget, GdkEventMotion *_evt, void *_gui);
void cbk_dropselection(GtkWidget *_widget, GdkEventButton *_evt, void *_gui);

/* Touch Screen */
gboolean cbk_mtnplot(GtkWidget *_wgt, GdkEventMotion *_evt, void *_gui);
gboolean cbk_rlsplot(GtkWidget *_wgt, GdkEventKey *_evt, void *_gui);

/* Menu Actions */
void cbk_acnmnu(GtkMenuItem *_itm, void *_gui);

/* Windows */
gboolean cbk_delwinplot(GtkWidget *_wgt, GdkEvent *_evt, void *_gui);
gboolean cbk_delwinscrn(GtkWidget *_wgt, GdkEvent *_evt, void *_gui);
gboolean cbk_delwininfo(GtkWidget *_wgt, GdkEvent *_evt, void *_gui);
void cbk_tglwinplot(GtkMenuItem *_itm, void *_gui);
void cbk_tglwinscrn(GtkMenuItem *_itm, void *_gui);
void cbk_tglwininfo(GtkMenuItem *_itm, void *_gui);
void cbk_plotszealloc(GtkWidget *_alignment, GtkAllocation *_alloc, void *_gui);
#endif

#endif

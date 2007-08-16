#include <stdio.h>
#include <gtk/gtk.h>

#include "gui.h"

#define BUFSIZE 128

int main(int argc, char *argv[])
{
	/* GUI variables */
	gui_t *gui;

	/* Other variables */
	char buf[BUFSIZE];				/* General purpose string buffer */

	/* Initialize GDK threads */
	g_thread_init(NULL);
	gdk_threads_init();

	/* Initialize GTK */
	gtk_init(&argc, &argv);
	
	/* Say what GTK version we're running */
	snprintf(buf, BUFSIZE, "Using GTK+-%d.%d.%d", gtk_major_version,
												  gtk_minor_version,
												  gtk_micro_version);
	#if DEBUG
	printf("%s\n", buf);
	#endif

	/* Create GUI */
	gui = gui_new();

	/* Read command-line arguments */
	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			printf("usage: wavejet [addr [port]]\n");
			exit(0);
		} else {
			strncpy(gui->addr, argv[1], 16);
		}
	} else if (argc == 3) {
		if (argv[1][0] == '-' || argv[2][0] == '-') {
			printf("usage: wavejet [addr [port]]\n");
			exit(1);
		} else {
			strncpy(gui->addr, argv[1], 16);
			gui->port = atoi(argv[2]);
		}
	} else if (argc > 3) {
		printf("usage: wavejet [addr [port]]\n");
		exit(1);
	}

	/* Connecting thread */
	pthread_create(&gui->thdconnect, NULL, gui_connect, gui);

	/* Main event loop */
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}

#include <stdio.h>
#include <gtk/gtk.h>

#include "gui.h"

#define BUFSIZE 128

int main(int argc, char *argv[])
{
	/* GUI variables */
	gui_t *gui;

	/* Connection variables */
	char *addr;
	unsigned port = 1891;

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

	/* Read command-line arguments */
	addr = (char *) malloc(BUFSIZE);
	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			printf("usage: wavejet [addr [port]]\n");
			return 0;
		} else {
			strncpy(addr, argv[1], 16);
		}
	} else if (argc == 3) {
		if (argv[1][0] == '-' || argv[2][0] == '-') {
			printf("usage: wavejet [addr [port]]\n");
			return 1;
		} else {
			strncpy(addr, argv[1], 16);
			port = atoi(argv[2]);
		}
	} else if (argc > 3) {
		printf("usage: wavejet [addr [port]]\n");
		return 1;
	}

	/* Create GUI */
	gui = gui_new(addr, port);

	/* Connecting thread */
	pthread_create(&gui->thdconnect, NULL, gui_connect, gui);

	/* Main event loop */
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	return 0;
}

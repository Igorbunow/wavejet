#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>
#include <gtk/gtk.h>

#include "gui.h"

#define BUFSIZE 128

void usage(void)
{
	printf("usage: wavejet [addr [port]]\n");
}

int main(int argc, char *argv[])
{
	int rc;
	struct rlimit rlim;

	/* GUI variables */
	gui_t *gui;

	/* Connection variables */
	char *addr = NULL;
	char *port = NULL;

	/* Other variables */
	char buf[BUFSIZE];				/* General purpose string buffer */

	/* Hand in terminal to shell */
	rc = fork();
	if (rc == -1) {
		perror("fork");
		return 1;
	} else if (rc != 0) {
		/* Parent Process */
		return 0;
	}

	/* Now we're in the child process */

	/* Enable core dumps */
	rlim.rlim_cur = 10000000;
	rlim.rlim_max = 10000000;
	rc = setrlimit(RLIMIT_CORE, &rlim);
	if (rc != 0) {
		fprintf(stderr, "Couldn't enable core dumps: %s\n", strerror(rc));
	}

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
	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0) {
			usage();
			return 0;
		} else {
			addr = argv[1];
		}
	} else if (argc == 3) {
		if (argv[1][0] == '-' || argv[2][0] == '-') {
			usage();
			return 1;
		} else {
			addr = argv[1];
			port = argv[2];
		}
	} else if (argc > 3) {
		usage();
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

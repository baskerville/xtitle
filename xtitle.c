#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include "helpers.h"
#include "xtitle.h"

int main(int argc, char *argv[])
{
	bool snoop = false;
	bool escaped = false;
	char *format = NULL;
	char opt;

	while ((opt = getopt(argc, argv, "hvsef:")) != -1) {
		switch (opt) {
			case 'h':
				printf("xtitle [-h|-v|-s|-e|-f FORMAT] [WID ...]\n");
				return EXIT_SUCCESS;
				break;
			case 'v':
				printf("%s\n", VERSION);
				return EXIT_SUCCESS;
				break;
			case 's':
				snoop = true;
				break;
			case 'e':
				escaped = true;
				break;
			case 'f':
				format = optarg;
				break;
		}
	}

	int num = argc - optind;
	char **args = argv + optind;

	setup();

	char title[MAXLEN] = {0};

	if (num > 0) {
		char *end;
		for (int i = 0; i < num; i++) {
			errno = 0;
			long int wid = strtol(args[i], &end, 0);
			if (errno != 0 || *end != '\0')
				warn("Invalid window ID: '%s'.\n", args[i]);
			else
				output_title(wid, format, title, sizeof(title), escaped);
		}
	} else {
		xcb_window_t win = XCB_NONE;
		if (get_active_window(&win))
			output_title(win, format, title, sizeof(title), escaped);
		if (snoop) {
			signal(SIGINT, hold);
			signal(SIGHUP, hold);
			signal(SIGTERM, hold);
			watch(root, true);
			watch(win, true);
			xcb_window_t last_win = XCB_NONE;
			fd_set descriptors;
			int fd = xcb_get_file_descriptor(dpy);
			running = true;
			xcb_flush(dpy);
			while (running) {
				FD_ZERO(&descriptors);
				FD_SET(fd, &descriptors);
				if (select(fd + 1, &descriptors, NULL, NULL, NULL) > 0) {
					xcb_generic_event_t *evt;
					while ((evt = xcb_poll_for_event(dpy)) != NULL) {
						if (title_changed(evt, &win, &last_win))
							output_title(win, format, title, sizeof(title), escaped);
						free(evt);
					}
				}
				if (xcb_connection_has_error(dpy)) {
					warn("The server closed the connection.\n");
					running = false;
				}
			}
		}
	}

	xcb_ewmh_connection_wipe(ewmh);
	free(ewmh);
	xcb_disconnect(dpy);
	return EXIT_SUCCESS;
}

void setup(void)
{
	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy))
		err("Can't open display.\n");
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL)
		err("Can't acquire screen.\n");
	root = screen->root;
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0)
		err("Can't initialize EWMH atoms.\n");
}

char* expand_escapes(const char *src)
{
	char *dest = (char *)malloc(2 * strlen(src) + 1);
	char *start = dest;
	char c;
	while ((c = *(src++))) {
		if (c == '\'' || c == '\"') {
			*(dest++) = '\\';
			*(dest++) = '\"';
		}
		else if (c == '\\') {
			*(dest++) = '\\';
			*(dest++) = '\\';
		}
		else *(dest++) = c;
	}
	*dest = '\0';
	return start;
}

void output_title(xcb_window_t win, char *format, char *title, size_t len, bool escaped)
{
	get_window_title(win, title, len);
	if (escaped) {
		char *out = expand_escapes(title);
		printf(format == NULL ? FORMAT : format, out);
		free(out);
	}
	else printf(format == NULL ? FORMAT : format, title);
	printf("\n");
	fflush(stdout);
}

bool title_changed(xcb_generic_event_t *evt, xcb_window_t *win, xcb_window_t *last_win)
{
	if (XCB_EVENT_RESPONSE_TYPE(evt) == XCB_PROPERTY_NOTIFY) {
		xcb_property_notify_event_t *pne = (xcb_property_notify_event_t *) evt;
		if (pne->atom == ewmh->_NET_ACTIVE_WINDOW) {
			watch(*last_win, false);
			if (get_active_window(win)) {
				watch(*win, true);
				*last_win = *win;
			} else {
				*win = *last_win = XCB_NONE;
			}
			return true;
		} else if (*win != XCB_NONE && pne->window == *win && (pne->atom == ewmh->_NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME)) {
			return true;
		}
	}
	return false;
}

void watch(xcb_window_t win, bool state)
{
	if (win == XCB_NONE)
		return;
	uint32_t value = (state ? XCB_EVENT_MASK_PROPERTY_CHANGE : XCB_EVENT_MASK_NO_EVENT);
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, &value);
}

bool get_active_window(xcb_window_t *win)
{
	return (xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, default_screen), win, NULL) == 1);
}

void get_window_title(xcb_window_t win, char *title, size_t len) {
	xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
	xcb_icccm_get_text_property_reply_t icccm_txt_prop;
	ewmh_txt_prop.strings = icccm_txt_prop.name = NULL;
	title[0] = '\0';
	if (win != XCB_NONE && (xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, win), &ewmh_txt_prop, NULL) == 1 || xcb_icccm_get_wm_name_reply(dpy, xcb_icccm_get_wm_name(dpy, win), &icccm_txt_prop, NULL) == 1)) {
		char *src = NULL;
		size_t title_len = 0;
		if (ewmh_txt_prop.strings != NULL) {
			src = ewmh_txt_prop.strings;
			title_len = MIN(len, ewmh_txt_prop.strings_len);
		} else if (icccm_txt_prop.name != NULL) {
			src = icccm_txt_prop.name;
			title_len = MIN(len, icccm_txt_prop.name_len);
		}
		if (src != NULL) {
			strncpy(title, src, title_len);
			title[title_len] = '\0';
		}
	}
	if (ewmh_txt_prop.strings != NULL)
		xcb_ewmh_get_utf8_strings_reply_wipe(&ewmh_txt_prop);
	if (icccm_txt_prop.name != NULL)
		xcb_icccm_get_text_property_reply_wipe(&icccm_txt_prop);
}

void hold(int sig)
{
	if (sig == SIGHUP || sig == SIGINT || sig == SIGTERM)
		running = false;
}

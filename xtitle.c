#include <err.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <wchar.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include "xtitle.h"

int main(int argc, char *argv[])
{
	dpy = NULL;
	ewmh = NULL;
	visible = false;
	running = true;
	bool snoop = false;
	bool escaped = false;
	wchar_t *format = NULL;
	int ret = EXIT_SUCCESS;
	int truncate = 0;
	int opt;

	signal(SIGINT, hold);
	signal(SIGHUP, hold);
	signal(SIGTERM, hold);

	setlocale(LC_ALL, "");

	while ((opt = getopt(argc, argv, "hvseif:t:")) != -1) {
		switch (opt) {
			case 'h':
				printf("xtitle [-h|-v|-s|-e|-i|-f FORMAT|-t NUMBER] [WID ...]\n");
				goto end;
				break;
			case 'v':
				printf("%s\n", VERSION);
				goto end;
				break;
			case 's':
				snoop = true;
				break;
			case 'e':
				escaped = true;
				break;
			case 'i':
				visible = true;
				break;
			case 'f': {
				size_t format_len = mbsrtowcs(NULL, (const char**)&optarg, 0, NULL);
				if (format_len == (size_t)-1) {
					warnx("can't decode the given format string: '%s'.", optarg);
					ret = EXIT_FAILURE;
					goto end;
				}
				wchar_t *tmp = realloc(format, (format_len + 1) * sizeof(wchar_t));
				if (tmp != NULL) {
					format = tmp;
					mbsrtowcs(format, (const char**)&optarg, format_len, NULL);
					format[format_len] = L'\0';
				}
			} break;
			case 't':
				truncate = atoi(optarg);
				break;
		}
	}

	int num = argc - optind;
	char **args = argv + optind;

	if (!setup()) {
		ret = EXIT_FAILURE;
		goto end;
	}

	wchar_t title[MAXLEN] = {0};

	if (num > 0) {
		char *end;
		for (int i = 0; i < num; i++) {
			errno = 0;
			long int wid = strtol(args[i], &end, 0);
			if (errno != 0 || *end != '\0') {
				warnx("invalid window ID: '%s'.", args[i]);
			} else {
				output_title(wid, format, title, MAXLEN, escaped, truncate);
			}
		}
	} else {
		xcb_window_t win = XCB_NONE;
		if (get_active_window(&win)) {
			output_title(win, format, title, MAXLEN, escaped, truncate);
		}
		if (snoop) {
			watch(root, true);
			watch(win, true);
			xcb_window_t last_win = XCB_NONE;
			fd_set descriptors;
			int fd = xcb_get_file_descriptor(dpy);
			xcb_flush(dpy);
			while (running) {
				FD_ZERO(&descriptors);
				FD_SET(fd, &descriptors);
				/* We do this because xcb_wait_for_event prevents us from catching signals. */
				if (select(fd + 1, &descriptors, NULL, NULL, NULL) > 0) {
					xcb_generic_event_t *evt;
					while ((evt = xcb_poll_for_event(dpy)) != NULL) {
						if (title_changed(evt, &win, &last_win)) {
							output_title(win, format, title, MAXLEN, escaped, truncate);
						}
						free(evt);
					}
				}
				if (xcb_connection_has_error(dpy)) {
					warnx("the server closed the connection.");
					running = false;
				}
			}
		}
	}

end:
	if (ewmh != NULL) {
		xcb_ewmh_connection_wipe(ewmh);
	}
	if (dpy != NULL) {
		xcb_disconnect(dpy);
	}
	free(ewmh);
	free(format);
	return ret;
}

bool setup(void)
{
	dpy = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(dpy)) {
		warnx("can't open display.");
		return false;
	}
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	if (screen == NULL) {
		warnx("can't acquire screen.");
		return false;
	}
	root = screen->root;
	ewmh = malloc(sizeof(xcb_ewmh_connection_t));
	if (xcb_ewmh_init_atoms_replies(ewmh, xcb_ewmh_init_atoms(dpy, ewmh), NULL) == 0) {
		warnx("can't initialize EWMH atoms.");
		return false;
	}
	return true;
}

wchar_t* expand_escapes(const wchar_t *src)
{
	wchar_t *dest = malloc((2 * wcslen(src) + 1) * sizeof(wchar_t));
	wchar_t *start = dest;
	wchar_t c;
	while ((c = *(src++))) {
		if (c == L'\'' || c == L'\"' || c == L'\\') {
			*(dest++) = L'\\';
		}
		*(dest++) = c;
	}
	*dest = L'\0';
	return start;
}

void output_title(xcb_window_t win, wchar_t *format, wchar_t *title, size_t len, bool escaped, int truncate)
{
	get_window_title(win, title, len);
	if (truncate) {
		unsigned int n = abs(truncate);
		if (wcslen(title) > (size_t)n) {
			if (truncate > 0) {
				if (n > 3) {
					for (int i = 1; i <= 3; i++) {
						title[truncate-i] = L'.';
					}
				}
				title[truncate] = L'\0';
			} else {
				title = title + wcslen(title) + truncate;
				if (n > 3) {
					for (int i = 0; i <= 2; i++) {
						title[i] = L'.';
					}
				}
			}
		}
	}
	if (escaped) {
		wchar_t *out = expand_escapes(title);
		print_title(format, out);
		free(out);
	} else {
		print_title(format, title);
	}
	fflush(stdout);
}

void print_title(wchar_t *format, wchar_t *title)
{
	if (format == NULL) {
		wprintf(FORMAT, title);
	} else {
		wchar_t *spec = NULL;
		size_t len = wcslen(format);
		for (size_t i = 0; i < len; i++) {
			wchar_t cur = format[i];
			if (spec == NULL) {
				if (cur == L'%' || cur == L'\\') {
					spec = format + i;
				} else {
					wprintf(L"%lc", cur);
				}
			} else {
				if (*spec == L'%' && cur == L's') {
					wprintf(L"%ls", title);
				} else if (*spec == L'\\' && cur == L'n') {
					wprintf(L"\n");
				} else if (*spec == cur) {
					wprintf(L"%lc", cur);
				} else {
					wprintf(L"%lc%lc", *spec, cur);
				}
				spec = NULL;
			}
		}
	}
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
		} else if (*win != XCB_NONE && pne->window == *win && ((visible && pne->atom == ewmh->_NET_WM_VISIBLE_NAME) || pne->atom == ewmh->_NET_WM_NAME || pne->atom == XCB_ATOM_WM_NAME)) {
			return true;
		}
	}
	return false;
}

void watch(xcb_window_t win, bool state)
{
	if (win == XCB_NONE) {
		return;
	}
	uint32_t value = (state ? XCB_EVENT_MASK_PROPERTY_CHANGE : XCB_EVENT_MASK_NO_EVENT);
	xcb_change_window_attributes(dpy, win, XCB_CW_EVENT_MASK, &value);
}

bool get_active_window(xcb_window_t *win)
{
	return (xcb_ewmh_get_active_window_reply(ewmh, xcb_ewmh_get_active_window(ewmh, default_screen), win, NULL) == 1);
}

void get_window_title(xcb_window_t win, wchar_t *title, size_t len)
{
	xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
	xcb_icccm_get_text_property_reply_t icccm_txt_prop;
	ewmh_txt_prop.strings = icccm_txt_prop.name = NULL;
	title[0] = L'\0';
	if (win != XCB_NONE && ((visible && xcb_ewmh_get_wm_visible_name_reply(ewmh, xcb_ewmh_get_wm_visible_name(ewmh, win), &ewmh_txt_prop, NULL) == 1) || xcb_ewmh_get_wm_name_reply(ewmh, xcb_ewmh_get_wm_name(ewmh, win), &ewmh_txt_prop, NULL) == 1 || xcb_icccm_get_wm_name_reply(dpy, xcb_icccm_get_wm_name(dpy, win), &icccm_txt_prop, NULL) == 1)) {
		char *src = NULL;
		size_t title_len = 0;
		if (ewmh_txt_prop.strings != NULL) {
			src = ewmh_txt_prop.strings;
			title_len = MIN(len, ewmh_txt_prop.strings_len);
		} else if (icccm_txt_prop.name != NULL) {
			src = icccm_txt_prop.name;
			title_len = MIN(len, icccm_txt_prop.name_len);
			/* Extract UTF-8 embedded in Compound Text */
			if (title_len > strlen(CT_UTF8_BEGIN CT_UTF8_END) &&
			    memcmp(src, CT_UTF8_BEGIN, strlen(CT_UTF8_BEGIN)) == 0 &&
			    memcmp(src + title_len - strlen(CT_UTF8_END), CT_UTF8_END, strlen(CT_UTF8_END)) == 0) {
				src += strlen(CT_UTF8_BEGIN);
				title_len -= strlen(CT_UTF8_BEGIN CT_UTF8_END);
			}
		}
		if (src != NULL) {
			title_len = mbsnrtowcs(title, (const char**)&src, title_len, MAXLEN, NULL);
			if (title_len == (size_t)-1) {
				warnx("can't decode the title of 0x%08lX.", (unsigned long)win);
				title_len = 0;
			}
			title[title_len] = L'\0';
		}
	}
	if (ewmh_txt_prop.strings != NULL) {
		xcb_ewmh_get_utf8_strings_reply_wipe(&ewmh_txt_prop);
	}
	if (icccm_txt_prop.name != NULL) {
		xcb_icccm_get_text_property_reply_wipe(&icccm_txt_prop);
	}
}

void hold(int sig)
{
	if (sig == SIGHUP || sig == SIGINT || sig == SIGTERM) {
		running = false;
	}
}

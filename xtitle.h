#ifndef _XTITLE_H
#define _XTITLE_H

#define FORMAT         L"%ls\n"
/* Reference: http://www.sensi.org/~alec/locale/other/ctext.html */
#define CT_UTF8_BEGIN   "\x1b\x25\x47"
#define CT_UTF8_END     "\x1b\x25\x40"

xcb_connection_t *dpy;
xcb_ewmh_connection_t *ewmh;
int default_screen;
xcb_window_t root;
bool running, visible;

bool setup(void);
wchar_t* expand_escapes(const wchar_t *src);
void output_title(xcb_window_t win, wchar_t *format, bool escaped, int truncate);
void print_title(wchar_t *format, wchar_t *title, xcb_window_t win);
bool title_changed(xcb_generic_event_t *evt, xcb_window_t *win, xcb_window_t *last_win);
void watch(xcb_window_t win, bool state);
bool get_active_window(xcb_window_t *win);
wchar_t *get_window_title(xcb_window_t win);
void hold(int sig);

#endif

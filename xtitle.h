#ifndef _XTITLE_H
#define _XTITLE_H

#define MAXLEN            256
#define MIN(A, B)         ((A) < (B) ? (A) : (B))

#define FORMAT         L"%ls\n"
/* Reference: http://www.sensi.org/~alec/locale/other/ctext.html */
#define CT_UTF8_BEGIN   "\x1b\x25\x47"
#define CT_UTF8_END     "\x1b\x25\x40"

xcb_connection_t *dpy;
xcb_window_t root;
xcb_ewmh_connection_t *ewmh;
int default_screen;
bool running, visible;

void hold(int);
void setup(void);
void watch(xcb_window_t, bool);
wchar_t* expand_escapes(const wchar_t *);
void output_title(xcb_window_t, wchar_t *, wchar_t *, size_t, bool, int);
bool title_changed(xcb_generic_event_t *, xcb_window_t *, xcb_window_t *);
bool get_active_window(xcb_window_t *);
void get_window_title(xcb_window_t, wchar_t *, size_t);

#endif

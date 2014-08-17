#ifndef _XTITLE_H
#define _XTITLE_H

#define FORMAT   "%s"

xcb_connection_t *dpy;
xcb_window_t root;
xcb_ewmh_connection_t *ewmh;
int default_screen;
bool running;

void hold(int);
void setup(void);
void watch(xcb_window_t, bool);
char* expand_escapes(const char *);
void output_title(xcb_window_t, char *, char *, size_t, bool);
bool title_changed(xcb_generic_event_t *, xcb_window_t *, xcb_window_t *);
bool get_active_window(xcb_window_t *);
void get_window_title(xcb_window_t, char *, size_t);

#endif

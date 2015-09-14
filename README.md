# Description
If arguments are given, outputs the title of each arguments, otherwise outputs the title of the active window and continue to output it as it changes if the *snoop* mode is on.

# Synopsis
	xtitle [-h|-v|-s|-e|-i|-f FORMAT|-t NUMBER] [WID ...]

# Options
- `-h` — Print the synopsis to standard output and exit.
- `-v` — Print the version to standard output and exit.
- `-s` — Activate the *snoop* mode.
- `-e` — Escape the following characters: ', " and \\.
- `-i` — Try to retrieve the title from the `_NET_WM_VISIBLE_NAME` atom.
- `-f FORMAT` — Use the given `printf` format.
- `-t NUMBER` — Truncate the title after |*NUMBER*| characters starting at the first (or the last if *NUMBER* is negative) character.

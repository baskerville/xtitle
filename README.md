# Description
If arguments are given, outputs the title of each arguments, otherwise outputs the title of the active window and continue to output it as it changes if the *snoop* mode is on.

# Synopsis
	xtitle [-h|-v|-s|-e|-f FORMAT|-t NUMBER] [WID ...]

# Options
- `-h` — Print the synopsis to standard output and exit.
- `-v` — Print the version to standard output and exit.
- `-s` — Activate the *snoop* mode.
- `-e` — Escape the following characters: ', " and \\.
- `-f FORMAT` — Use the given `printf` format.
- `-t NUMBER` — Truncate the title after *NUMBER* characters.

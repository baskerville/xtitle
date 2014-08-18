# Description
If arguments are given, outputs the title of each arguments, otherwise outputs the title of the active window and continue to output it as it changes if the *snoop* mode is on.

# Synopsis
	xtitle [-h|-v|-s|-e|-f FORMAT] [WID ...]

# Options
- `-h` — Print the synopsis to standard output and exit.
- `-v` — Print the version to standard output and exit.
- `-s` — Activate the *snoop* mode.
- `-e` — Activate the *escape* mode. Prints \" instead of " or ', \\\\ instead of \\.
- `-f FORMAT` — Use the given `printf` format.

# Example usage with JSON
	xtitle -s -e -f '[{"full_text":"%s","color":"#FFA500","separator":false}]'
Escape and snoop mode work well within i3 and [i3cat][1].

[1]: https://github.com/vincent-petithory/i3cat

File matching program using 'glob' patterns.

Use instead of normal shell file matching for more control especially for errors
when no match and when filtering matches to single match.

Usage:

CGMatch [-c|--count|--newest|--oldest|--largest|--smallest|-m|-q|-e <command>] <pattern>
 -c|--count   : count matching files
 -newest      : newest matching file
 -oldest      : oldest matching file
 -largest     : largest matching file
 -smallest    : smallest matching file
 -m           : add directory marker (/ at end)
 -q           : suppress messages
 -nostr       : string to return if no match
 -e <command> : execute command on each matching file

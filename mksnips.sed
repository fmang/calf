# Delete comments and empty lines
/^\s*#/ d
/^\s*$/ d

# Anchors
/^@/ {
	s/^@\s*\(.*\)$/; const char *snip_\1 =/p
	d
}
$ a ;

# Filter the text
s/"/\\"/g
s/^\s*/"/
s/\s*$/"/

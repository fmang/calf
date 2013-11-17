# Delete comments and empty lines
/^\s*#/ d
/^\s*$/ d

# Anchors
/^@/ {
	s/^@\s*//
	s/ /_/g
	s/^.*$/; const char *snip_& =/p
	d
}
$ a ;

# Filter the text
s/"/\\"/g
s/^\s*/"/
s/\s*$/"/

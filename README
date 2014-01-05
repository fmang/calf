Calf
====

Calf is a simple CGI application to generate file listings cooler than your web
server's. It organizes files by year and month, as it was primarily made for
browsing archives.

Installing
----------

First, you need [uriparser][]. That should be the only external dependency.

[uriparser]: http://uriparser.sourceforge.net/

Now, to prepare the build:

	./configure --help
	./configure

Two notable features are FastCGI support (`--with-fcgi`) and systemd socket
activation (`--with-systemd`). Both are detected and enabled if no options are
specified.

If you expect FastCGI support, use `--with-fcgi` anyway.

	make
	make install

Nothing surprising.

systemd users would then grab `calf.service` and `calf.socket` in `systemd/`.
They are even configured with the proper paths.

Setting up
----------

Check the man page, calf(8).

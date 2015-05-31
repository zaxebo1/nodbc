Until recently, the haXe and Neko platforms provided support for only MySQL Server, SQLite and PostgreSQL. Now, thanks to nODBC, the haXe and Neko platforms can also target Microsoft SQL Server, Microsoft Access, Sybase, Oracle and more...

This extensions currently has minor irritations that require custom functions to be added for it to be SPOD compatible, as ODBC doesn't provide a standardized means to acquire the last inserted id, or the current number of rows; both required within the SPOD framework.

For a more advanced SPOD, check out Danny Wilson's haxORMap

ATTENTION!

---

I really could do with some makefiles for this project for Mac OSX (both architectures) and Linux.  Please, if you're good at that sort of thing, lend a hand :-)
For Linux there is a simple Makefile currently only tested on Gentoo 64 bit.
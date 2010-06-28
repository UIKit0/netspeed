uidir = $(datadir)/netspeed/
ui_DATA = menu.xml

serverdir = $(libdir)/bonobo/servers
server_DATA = GNOME_NetspeedApplet.server
server_in_files = $(server_DATA:=.in)
server_in_in_files = $(server_in_files:=.in)

$(server_in_files): $(server_in_in_files) Makefile
	sed s,@LIBEXECDIR@,$(libexecdir),g <$< >$@

EXTRA_DIST = GNOME_NetspeedApplet.server.in.in

CLEANFILES = $(server_DATA) $(server_in_files)

@INTLTOOL_SERVER_RULE@


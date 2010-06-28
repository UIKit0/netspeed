SUBDIRS = data help icons po src

distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

spec = $(PACKAGE).spec
EXTRA_DIST = $(spec) \
             gnome-doc-utils.make \
             intltool-update.in \
             intltool-merge.in \
             intltool-extract.in

all-local: $(spec)

DISTCLEANFILES = \
                 gnome-doc-utils.make \
                 intltool-extract \
                 intltool-merge \
                 intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

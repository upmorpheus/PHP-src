
LEMON="@LEMON@"

# sqlite has some generated source files; this specifies how to
# build them

$(srcdir)/libsqlite/src/sqlite.h:	$(srcdir)/libsqlite/src/sqlite.h.in $(srcdir)/libsqlite/VERSION
	$(SED) -e s/--VERS--/`cat $(srcdir)/libsqlite/VERSION`/ \
            -e s/--ENCODING--/$(SQLITE_ENCODING)/ \
                 $(srcdir)/libsqlite/src/sqlite.h.in >$(srcdir)/libsqlite/src/sqlite.h

# We avoid building these last three by bundling the generated versions
# in our release packages

$(srcdir)/libsqlite/src/opcodes.c: $(srcdir)/libsqlite/src/vdbe.c
	echo '/* Automatically generated file.  Do not edit */' >$@
	echo 'char *sqliteOpcodeNames[] = { "???", ' >>$@
	grep '^case OP_' $< | \
	  sed -e 's/^.*OP_/  "/' -e 's/:.*$$/", /' >>$@
	echo '};' >>$@
	
$(srcdir)/libsqlite/src/opcodes.h: $(srcdir)/libsqlite/src/vdbe.c
	echo '/* Automatically generated file.  Do not edit */' >$@
	grep '^case OP_' $< | \
	  sed -e 's/://' | \
	  awk '{printf "#define %-30s %3d\n", $$2, ++cnt}' >>$@

$(srcdir)/libsqlite/src/parse.c:	$(srcdir)/libsqlite/src/parse.y
	$(LEMON) $(srcdir)/libsqlite/src/parse.y



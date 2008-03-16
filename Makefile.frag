
#
# Zend
#

$(builddir)/zend_language_scanner.lo: $(srcdir)/zend_language_parser.h
$(builddir)/zend_ini_scanner.lo: $(srcdir)/zend_ini_parser.h

$(srcdir)/zend_language_scanner.c: $(srcdir)/zend_language_scanner.l
	$(RE2C) $(RE2C_FLAGS) --case-inverted -cbdFt $(srcdir)/zend_language_scanner_defs.h -o$@ $(srcdir)/zend_language_scanner.l

$(srcdir)/zend_language_parser.h: $(srcdir)/zend_language_parser.c
$(srcdir)/zend_language_parser.c: $(srcdir)/zend_language_parser.y
	@$(YACC) -p zend -v -d $(srcdir)/zend_language_parser.y -o $@

$(srcdir)/zend_ini_parser.h: $(srcdir)/zend_ini_parser.c
$(srcdir)/zend_ini_parser.c: $(srcdir)/zend_ini_parser.y
	@$(YACC) -p ini_ -v -d $(srcdir)/zend_ini_parser.y -o $@

$(srcdir)/zend_ini_scanner.c: $(srcdir)/zend_ini_scanner.l
	$(RE2C) $(RE2C_FLAGS) --case-inverted -cbdFt $(srcdir)/zend_ini_scanner_defs.h -o$@ $(srcdir)/zend_ini_scanner.l

$(builddir)/zend_indent.lo $(builddir)/zend_highlight.lo $(builddir)/zend_compile.lo: $(srcdir)/zend_language_parser.h

include $(top_builddir)/config_vars.mk
LTLIBRARY_OBJECTS = $(LTLIBRARY_SOURCES:.c=.lo) $(LTLIBRARY_OBJECTS_X)
LTLIBRARY_SHARED_OBJECTS = $(LTLIBRARY_OBJECTS:.lo=.slo)
$(LTLIBRARY_NAME): $(LTLIBRARY_OBJECTS) $(LTLIBRARY_DEPENDENCIES)
	$(LINK) $(LTLIBRARY_LDFLAGS) $(LTLIBRARY_OBJECTS) $(LTLIBRARY_LIBADD)

targets = $(LTLIBRARY_NAME)

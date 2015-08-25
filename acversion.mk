$(top_srcdir)/version.mk: force-version-check
	@$(top_srcdir)/generate-version $@

force-version-check:

-include $(top_srcdir)/version.mk
-include $(top_srcdir)/libversion.mk

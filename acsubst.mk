# A little sed macro for expanding common vairables in the template files.
# Included from makefiles that need this macro.
acsubst = sed -e 's|@prefix[@]|$(prefix)|g' \
		-e 's|@exec_prefix[@]|$(exec_prefix)|g' \
		-e 's|@libdir[@]|$(libdir)|g' \
		-e 's|@includedir[@]|$(includedir)|g' \
                -e 's|@HOGL_CXXFLAGS[@]|$(HOGL_CXXFLAGS)|g' \
                -e 's|@HOGL_LIBS[@]|$(HOGL_LIBS)|g' \
                -e 's|@VERSION[@]|$(VERSION)|g' \
                -e 's|@RELEASE[@]|$(RELEASE)|g'


# Subst rule for dot spec files
%.spec: %.spec.in Makefile version.mk
	@echo Generating $@
	@$(acsubst) $< > $@

# Subst rule for dot pc files
%.pc: %.pc.in Makefile version.mk
	@echo Generating $@
	@$(acsubst) $< > $@

# Subst rule for dot cmake files
%.cmake: %.cmake.in Makefile version.mk
	@echo Generating $@
	@$(acsubst) $< > $@

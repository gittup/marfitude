INSTALL_DIR = $(DATADIR)/wam
INSTALL_LIST = README
EXEC = text

wamdir = $(subst //,/,$(DESTDIR)/$(INSTALLDIR)/wam)
wamfiles = $(wildcard $(wamdir)/*.wam)
UNINSTALL_FILES += $(wamfiles)

install: $(wamfiles)
$(wamdir)/%.wam: gmae/wam.c
	@$(call RM,$@)

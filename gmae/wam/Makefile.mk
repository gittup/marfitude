INSTALL_DIR = $(DATADIR)/wam
INSTALL_LIST = README
EXEC = text

UNINSTALL_FILES += $(wildcard $(subst //,/,$(DESTDIR)/$(INSTALL_DIR)/*.wam))

project: $(wildcard $(d)/*.wam)
$(d)/%.wam: gmae/wam.c
	@$(call RM,$@)

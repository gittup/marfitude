INSTALL_DIR = $(DATADIR)/wam
INSTALL_LIST = README
include $(MK)/tgt/text.mk

UNINSTALL_FILES += $(wildcard $(DESTDIR)/$(INSTALL_DIR)/*.wam)

project: $(wildcard $(d)/*.wam)
$(d)/%.wam: $(src_gmae)/wam.c
	@$(call RM,$@)

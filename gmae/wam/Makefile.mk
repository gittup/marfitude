INSTALL_DIR = wam
INSTALL_LIST = README
include $(MK)/tgt/text.mk

uninstall_files: $(wildcard $(INSTALL_ROOT)/$(INSTALL_DIR)/*.wam)

project: $(wildcard $(d)/*.wam)
$(d)/%.wam: $(src_gmae)/wam.c
	@$(call RM,$@)

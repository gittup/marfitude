INSTALL_DIR = $(DATADIR)/images
INSTALL_LIST := $(wildcard $(d)/*.png)
include $(MK)/tgt/data.mk

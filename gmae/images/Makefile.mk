INSTALL_DIR = $(DATADIR)/images
INSTALL_LIST := $(wildcard $(d)/*.png)
include mk/tgt/data.mk

INSTALL_DIR := $(DATADIR)/sounds
INSTALL_LIST := $(wildcard $(d)/*.wav)
include $(MK)/tgt/data.mk

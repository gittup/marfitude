ifeq ($(ARCH),mingw)
INSTALL_DIR = $(DATADIR)
INSTALL_LIST = *.dll
EXEC = data
endif

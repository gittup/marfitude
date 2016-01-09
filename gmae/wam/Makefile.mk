INSTALL_DIR = $(DATADIR)/wam
INSTALL_LIST = README
EXEC = text

wamfiles = $(wildcard $(DATADIR)/wam/*.wam)
UNINSTALL_FILES += $(wamfiles)

ifeq ($(ARCH),darwin)
INSTALL_DIR = $(DESTDIR)
INSTALL_LIST = Info.plist PkgInfo
EXEC = data
endif

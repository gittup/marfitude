EXTRACFLAGS = $(SDLCFLAGS) -DMOD_MUSIC $(BDECFLAGS)

INCDIRS := $(d)/mikmod

include mk/lang/c.mk

include mk/tgt/staticlib.mk

subdir := mikmod
include mk/Recurse.mk

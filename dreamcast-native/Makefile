# Sega Rally native Dreamcast build (enDjinn on KOS). See docs/dreamcast-port.md.
ENDJINN_ROOT ?= $(abspath ../../enDJinn)
ifeq ($(wildcard $(ENDJINN_ROOT)/base_link.mk),)
$(error Set ENDJINN_ROOT to your enDjinn checkout)
endif

.DEFAULT_GOAL := all
include $(ENDJINN_ROOT)/base_link.mk

# enDjinn's make does not track option changes: rebuild everything on edits here.
$(OBJS): Makefile local.cfg.mk

# The renderer may approximate: fast-math there. Without it GCC calls __unordsf2 for every float compare on the
# SH-4 (most of the frame); gameplay keeps IEEE compares and no contraction (local.cfg.mk).
$(filter $(ENJ_BUILDDIR)/code/render/%.o,$(OBJS)): ENJ_CFLAGS += -ffast-math

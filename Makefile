#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

ifneq ($(shell pkg-config --exists fftw3f && echo true),true)
$(error fftw3f dependency not installed/available)
endif

# --------------------------------------------------------------

include dpf/Makefile.base.mk

all: plugins gen

# --------------------------------------------------------------

ifneq ($(CROSS_COMPILING),true)
CAN_GENERATE_TTL = true
else ifneq ($(EXE_WRAPPER),)
CAN_GENERATE_TTL = true
endif

# --------------------------------------------------------------

aubio:
	$(MAKE) -C aubio

plugins: aubio
	$(MAKE) -C plugins/AudioToCVPitch

ifeq ($(CAN_GENERATE_TTL),true)
gen: plugins dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh

dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator
else
gen:
endif

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C aubio
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(MAKE) clean -C plugins/AudioToCVPitch
	rm -rf bin build

# --------------------------------------------------------------

.PHONY: aubio plugins

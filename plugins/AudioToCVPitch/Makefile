#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = AudioToCVPitch

# --------------------------------------------------------------
# Location to aubio lib

AUBIO_DIR = ../../aubio

# --------------------------------------------------------------
# Files to build

FILES_DSP = AudioToCVPitch.cpp

EXTRA_DEPENDENCIES = $(AUBIO_DIR)/libaubio.a

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk

BUILD_CXX_FLAGS += -I$(AUBIO_DIR)/src

EXTRA_LIBS  = $(EXTRA_DEPENDENCIES)
EXTRA_LIBS += $(shell pkg-config --libs fftw3f)

# --------------------------------------------------------------
# Enable all possible plugin types

TARGETS += lv2_dsp

all: $(TARGETS)

# --------------------------------------------------------------

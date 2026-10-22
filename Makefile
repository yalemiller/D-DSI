# Pocket Sheet: DS-mode ROM, built with BlocksDS (ARM9 only, default ARM7).
#   make          builds pocketsheet.nds in this folder
# Host unit tests live in tests/ (make -C tests).

BLOCKSDS	?= /opt/blocksds/core

NAME		:= pocketsheet
GAME_TITLE	:= Pocket Sheet
GAME_SUBTITLE	:= Character reference
GAME_AUTHOR	:= Yale Miller
GAME_ICON	:= assets/icon32.png

SOURCEDIRS	:= source
INCLUDEDIRS	:= source assets

# Added after the default -Wall -O2. gnu11 rather than c11: libnds headers use
# the GNU asm keyword. The platform-independent code is also built with strict
# -std=c11 -Wpedantic -Werror by tests/Makefile.
CFLAGS		:= -std=gnu11 -Wextra -Wshadow

include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

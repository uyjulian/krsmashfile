
SOURCES += XP3Archive.cpp
SOURCES += StorageIntf.cpp
SOURCES += StorageImpl.cpp
SOURCES += storage.cpp
SOURCES += CharacterSet.cpp
LDLIBS += -luuid
# INCFLAGS += -Isome_path
# CFLAGS += -Dsome_definition
# LDLIBS += -lsome_library
PROJECT_BASENAME = krsmashfile

RC_LEGALCOPYRIGHT ?= Copyright (C) 2024-2024 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make

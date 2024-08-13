
SOURCES += external/zlib/adler32.c external/zlib/compress.c external/zlib/crc32.c external/zlib/deflate.c external/zlib/gzclose.c external/zlib/gzlib.c external/zlib/gzread.c external/zlib/gzwrite.c external/zlib/infback.c external/zlib/inffast.c external/zlib/inflate.c external/zlib/inftrees.c external/zlib/trees.c external/zlib/uncompr.c external/zlib/zutil.c
SOURCES += XP3Archive.cpp
SOURCES += StorageIntf.cpp
SOURCES += StorageImpl.cpp
SOURCES += storage.cpp
SOURCES += CharacterSet.cpp
INCFLAGS += -Iexternal/zlib
LDLIBS += -luuid
# INCFLAGS += -Isome_path
# CFLAGS += -Dsome_definition
# LDLIBS += -lsome_library
PROJECT_BASENAME = krsmashfile

RC_LEGALCOPYRIGHT ?= Copyright (C) 2024-2024 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make

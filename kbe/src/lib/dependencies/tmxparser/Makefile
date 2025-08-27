LIB =	tmxparser

SRCS =				\
	base64/base64			\
	TmxEllipse		\
	TmxImage		\
	TmxImageLayer		\
	TmxLayer		\
	TmxMap			\
	TmxObject		\
	TmxObjectGroup		\
	TmxPolygon		\
	TmxPolyline		\
	TmxPropertySet		\
	TmxTile			\
	TmxTileset		\
	TmxUtil			\
	TmxTileOffset		\
	TmxTileLayer		\
	TmxTerrainArray		\
	TmxTerrain		\
	TmxProperty		\
	TmxGroupLayer		\
	TmxColor			\
	TmxText

ifndef KBE_ROOT
export KBE_ROOT := $(subst /kbe/src/lib/dependencies/$(LIB),,$(CURDIR))
endif

include $(KBE_ROOT)/kbe/src/build/common.mak
# Eden Duo ACNH module, selected by this profile; the host adapter remains title-independent.
COMPANION_VENDOR = ../third_party/eden_duo
COMPANION_CXX = $(COMPANION_VENDOR)/acnh/01006F8002326000.cpp $(sort $(wildcard $(COMPANION_VENDOR)/acnh/acnh_*.cpp)) $(COMPANION_VENDOR)/bc_decoder/bc_decoder.cpp
COMPANION_INCLUDES = -I$(COMPANION_VENDOR) -I$(COMPANION_VENDOR)/acnh -I$(COMPANION_VENDOR)/bc_decoder -I$(COMPANION_VENDOR)/stb -I../third_party/zstd
COMPANION_C = ../third_party/zstd/common/entropy_common.c ../third_party/zstd/common/error_private.c ../third_party/zstd/common/fse_decompress.c ../third_party/zstd/common/xxhash.c ../third_party/zstd/common/zstd_common.c ../third_party/zstd/decompress/huf_decompress.c ../third_party/zstd/decompress/zstd_ddict.c ../third_party/zstd/decompress/zstd_decompress.c ../third_party/zstd/decompress/zstd_decompress_block.c

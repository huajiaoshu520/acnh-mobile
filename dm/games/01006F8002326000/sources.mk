# ACNH-only LAN service overrides. Other profiles use the upstream host files.
ACNH_LAN_DIR := $(GAME_DIR)/network
SRC := $(filter-out hle_account.c hle_friends.c hle_misc.c hle_net.c hle_offline.c,$(SRC)) $(sort $(wildcard $(ACNH_LAN_DIR)/*.c $(ACNH_LAN_DIR)/*.m))
CFLAGS += -I. -I$(ACNH_LAN_DIR) -DDM_GAME_OBS_MAX=35 -DDM_CUSTOM_NFP=1
GAME_SOURCE_DEPS := $(GAME_DIR)/sources.mk $(wildcard $(ACNH_LAN_DIR)/*.h)

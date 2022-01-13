ifeq "$(origin QNX_STAGE_ROOT)" "environment"
USE_INSTALL_ROOT=1
INSTALL_ROOT_nto=$(QNX_STAGE_ROOT)
endif

PORT_NAME := nova2
ENV_PREFIX := NOVA2
PORTBASE := portbase
PORT_SRCS := $(wildcard game/*.cpp)
CPPFLAGS += -Igame
include $(PORTBASE)/Makefile

.PHONY: libs
libs: $(TARGET)
	bash tools/collect_libs.sh $(TARGET) build/libs.armhf
	bash tools/check_glibc_floor.sh $(TARGET) build/libs.armhf

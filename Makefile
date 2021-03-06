CC                    := gcc
BIN                   := app


SOURCE_DIR_REL        := ./source
SOURCE_DIR            := $(abspath $(SOURCE_DIR_REL))

BUILD_DIR             := ./build
BUILD_DIR_ABS         := $(abspath $(BUILD_DIR))

FREERTOS_DIR_REL      := ./FreeRTOS
FREERTOS_DIR          := $(abspath $(FREERTOS_DIR_REL))

KERNEL_DIR            := ${FREERTOS_DIR}/Source

INCLUDE_DIRS          := -I.
INCLUDE_DIRS          += -I${SOURCE_DIR}
INCLUDE_DIRS          += -I${KERNEL_DIR}/include
INCLUDE_DIRS          += -I${KERNEL_DIR}/portable/ThirdParty/GCC/Posix
INCLUDE_DIRS          += -I${KERNEL_DIR}/portable/ThirdParty/GCC/Posix/utils

SOURCE_FILES          := $(wildcard *.c)
SOURCE_FILES          += $(wildcard ${SOURCE_DIR}/*.c)
SOURCE_FILES          += $(wildcard ${FREERTOS_DIR}/Source/*.c)
# Memory manager (use malloc() / free() )
SOURCE_FILES          += ${KERNEL_DIR}/portable/MemMang/heap_3.c
# posix port
SOURCE_FILES          += ${KERNEL_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
SOURCE_FILES          += ${KERNEL_DIR}/portable/ThirdParty/GCC/Posix/port.c


CFLAGS                :=    -ggdb3
LDFLAGS               :=    -ggdb3 -pthread
CPPFLAGS              :=    $(INCLUDE_DIRS) -DBUILD_DIR=\"$(BUILD_DIR_ABS)\"
CPPFLAGS              +=    -D_WINDOWS_

LIBS 				  := -lm


ifeq ($(COVERAGE_TEST),1)
  CPPFLAGS              += -DprojCOVERAGE_TEST=1
else
  CPPFLAGS              += -DprojCOVERAGE_TEST=0
endif


OBJ_FILES = $(SOURCE_FILES:%.c=$(BUILD_DIR)/%.o)

DEP_FILE = $(OBJ_FILES:%.o=%.d)

${BIN} : $(BUILD_DIR)/$(BIN)

${BUILD_DIR}/${BIN} : ${OBJ_FILES}
	-mkdir -p ${@D}
	$(CC) $^ ${LDFLAGS} -o $@ $(LIBS)

-include ${DEP_FILE}

${BUILD_DIR}/%.o : %.c Makefile
	-mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -c $< -o $@

.PHONY: clean

clean:
	-rm -rf $(BUILD_DIR)

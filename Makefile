DEVICE          = stm32f103c8t6
OPENCM3_DIR     = ./libopencm3/
# OBJS            += foo.o

# CFLAGS          += -Os -ggdb3
# CPPFLAGS	+= -MD
# LDFLAGS         += -static -nostartfiles
# LDLIBS          += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group


CFLAGS = -Wall -Wextra -g3 -O0
LDFLAGS = -static -nostartfiles
# LDLIBS = -lc -lnosys # no newlib libc
# LDLIBS = -lopencm3_stm32f1

OBJ = main.o

include $(OPENCM3_DIR)/mk/genlink-config.mk
include $(OPENCM3_DIR)/mk/gcc-config.mk

.PHONY: clean all

all: main

main: $(OBJ) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS) -Wl,-T,$(LDSCRIPT)

clean:
	$(Q)$(RM) -rf *.o *.d main

include $(OPENCM3_DIR)/mk/genlink-rules.mk
include $(OPENCM3_DIR)/mk/gcc-rules.mk

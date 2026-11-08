TARGET = DeadBabiesX.elf
OBJS = source/main.o source/xengine.o source/game.o source/selftest.o
KOS_CPPFLAGS += -std=gnu++17 -O2 -fexceptions -fno-fast-math -ffp-contract=off -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-narrowing
all: $(TARGET)
include $(KOS_BASE)/Makefile.rules
$(TARGET): $(OBJS)
	kos-c++ -o $@ $(OBJS) -lm
clean:
	rm -f $(OBJS) $(TARGET)

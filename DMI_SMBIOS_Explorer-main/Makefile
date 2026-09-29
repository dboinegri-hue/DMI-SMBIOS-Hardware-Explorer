# =============================================================================
#  Makefile -- DMI / SMBIOS Hardware Explorer (Linux / WSL2 Ubuntu)
# =============================================================================

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -g
LDFLAGS :=
TARGET  := dmi_explorer

SRC := src/main.c           \
       src/smbios.c         \
       src/display_system.c \
       src/display_memory.c \
       src/util.c

OBJ := $(SRC:.c=.o)

.PHONY: all clean run sudo-run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

# Use this if /sys/firmware/dmi/tables/DMI isn't world-readable.
sudo-run: $(TARGET)
	sudo ./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

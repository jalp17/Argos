CC = gcc
CFLAGS = -std=c11 -Wall -Wextra \
         -Wno-unused-variable -Wno-unused-parameter \
         -Wno-missing-braces -Wno-unused-function
LDFLAGS = -lm

BUILD = build
SRC = test_completo.c
TARGET = $(BUILD)/test_suite

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(BUILD):
	mkdir -p $(BUILD)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD)

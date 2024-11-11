CFLAGS = -Wall -g -std=c2x
LDFLAGS = -I. -L. -lpthread

bin = prog

SRC_DIR = ./src

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)

all: $(bin)

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD $(LDFLAGS)

$(bin): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

.PHONY: clean all
clean:
	rm -f $(bin) *.o *.d


-include $(OBJS:.o=.d)

ADD =
CFLAGS = -std=c2x -fPIC -O2 $(ADD)
LDFLAGS = -I../include -L../libs -lpthread -lwebserver -Wl,-rpath,../libs -Wl,-rpath,libs

bin = gdb-online-clone

SRC_DIR = src
OUT_DIR = out
# TEST_DIR = test

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)
OUT_OBJS = $(wildcard $(OUT_DIR)/*.o)
# TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

all: arrange

$(OUT_DIR):
	mkdir -p $@

arrange: $(bin)
	mv *.o *.d $(OUT_DIR)

$(bin): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: $(SRC_DIR)/%.c $(OUT_DIR) 
	$(CC) $(CFLAGS) -c $< -o $@ -MD $(LDFLAGS)

.PHONY: clean all test
clean:
	rm -f $(bin) *.o *.d
	rm out -r

-include $(OBJS:.o=.d)
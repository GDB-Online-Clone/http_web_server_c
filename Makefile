ADD =
CFLAGS = -std=c2x -fPIC $(ADD)
LDFLAGS = -Iinclude -Llibs -lpthread

UNITTEST_LDFLAGS = -lwebserver -lcunit -Wl,-rpath,libs

bin = prog # exists only for debugging
unittest = unittest # exists only for unittest
shared = libwebserver.so

SRC_DIR = src
OUT_DIR = out
INCLUDE_DIR = include/webserver
LIB_DIR = libs
TEST_DIR = test

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)
OUT_OBJS = $(wildcard $(OUT_DIR)/*.o)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

all: $(shared) arrange

binary: all $(bin)

run: binary
	./gdbc/$(bin)

test: all $(shared) $(unittest)
	./test/unittest

test-no-run: all $(shared) $(unittest)

$(OUT_DIR):
	mkdir -p $@
	mkdir -p $@/libs
	mkdir -p $@/include
	mkdir -p $@/include/webserver

arrange: $(shared)
	mv *.o *.d $(OUT_DIR)

%.o: $(SRC_DIR)/%.c $(OUT_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ -MD $(LDFLAGS)

$(shared): $(OBJS)
	mkdir -p $(LIB_DIR)
	mkdir -p include
	mkdir -p $(INCLUDE_DIR)
	$(CC) -shared $(CFLAGS) -o libs/$@ $(OBJS) $(LDFLAGS)
	cp $(SRC_DIR)/*.h include/webserver
	cp $(SRC_DIR)/*.h $(OUT_DIR)/include/webserver
	cp libs/$@ $(OUT_DIR)/libs


$(bin): arrange
	make -C gdbc

$(unittest): arrange
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_DIR)/$@ $(LDFLAGS) $(UNITTEST_LDFLAGS)

.PHONY: clean all test
clean:
	-rm -f $(bin) *.o *.d
	-rm out -r
	-make clean -C gdbc
	-rm test/$(unittest)

-include $(OBJS:.o=.d)
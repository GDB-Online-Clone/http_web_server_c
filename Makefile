CFLAGS = -Wall -std=c2x -g -DDEBUG
LDFLAGS = -I. -L. -lpthread

bin = prog # exists only for debugging
shared = libwebserver.so

SRC_DIR = ./src
OUT_DIR = ./out

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)
OUT_OBJS = $(wildcard $(OUT_DIR)/*.o)

all: $(shared) arrange

binary: all $(bin)

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
	$(CC) -shared $(CFLAGS) -o $(OUT_DIR)/libs/$@ $(OBJS) $(LDFLAGS)
	cp $(SRC_DIR)/*.h $(OUT_DIR)/include/webserver	

$(bin): arrange
	$(CC) $(CFLAGS) $(OUT_OBJS) -o $@ $(LDFLAGS)





.PHONY: clean all
clean:
	rm -f $(bin) *.o *.d
	rm out -r

-include $(OBJS:.o=.d)
CFLAGS = -Wall -g -std=c2x
LDFLAGS = -I. -L. -lpthread

bin = prog
shared = libwebserver.so

SRC_DIR = ./src
OUT_DIR = ./out

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)

all: $(OUT_DIR) $(shared) arrange

binary: $(OUT_DIR) $(bin) arrange

$(OUT_DIR):
	mkdir -p $@
	mkdir -p $@/libs
	mkdir -p $@/include
	mkdir -p $@/include/webserver

arrange:
	mv *.o *.d $(OUT_DIR)

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD $(LDFLAGS)

$(shared): $(OBJS)
	$(CC) -shared $(CFLAGS) -o $(OUT_DIR)/libs/$@ $(OBJS) $(LDFLAGS)
	cp $(SRC_DIR)/*.h $(OUT_DIR)/include/webserver

$(bin): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)





.PHONY: clean all
clean:
	rm -f $(bin) *.o *.d
	rm out -r

-include $(OBJS:.o=.d)
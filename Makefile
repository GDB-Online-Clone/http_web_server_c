CFLAGS = -Wall -g -std=c2x
LDFLAGS = -I. -L. -lpthread

bin = prog
shared = webserver.so

SRC_DIR = ./src
OUT_DIR = ./out

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)

all: $(OUT_DIR) $(shared) $(bin) arrange

$(OUT_DIR):
	mkdir -p $@
	mkdir -p $@/libs
	mkdir -p $@/include

arrange:
	mv *.o *.d $(OUT_DIR)	
	cp $(bin) $(OUT_DIR)
	cp $(SRC_DIR)/*.h $(OUT_DIR)/include

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -MD $(LDFLAGS)

$(shared): $(OBJS)
	$(CC) -shared $(CFLAGS) -o $(OUT_DIR)/libs/$@ $< $(LDFLAGS)

$(bin): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)



.PHONY: clean all
clean:
	rm -f $(bin) *.o *.d
	rm out -r


-include $(OBJS:.o=.d)

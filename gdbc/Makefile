CFLAGS = -Wall -std=c2x -g -DDEBUG
LDFLAGS = -Iinclude -Llibs -lpthread -lwebserver -Wl,-rpath,libs

bin = prog 

SRC_DIR = src
OUT_DIR = out
INCLUDE_DIR = include/webserver
LIB_DIR = libs
TEST_DIR = test

SRCS = $(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS = $(SRCS:.c=.o)
OUT_OBJS = $(wildcard $(OUT_DIR)/*.o)
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

all: arrange

$(OUT_DIR):
	mkdir -p $@

arrange: $(bin)
	mv *.o *.d $(OUT_DIR)

$(bin): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: $(SRC_DIR)/%.c $(OUT_DIR) 
	$(CC) $(CFLAGS) -c $< -o $@ -MD $(LDFLAGS)


-include $(OBJS:.o=.d)
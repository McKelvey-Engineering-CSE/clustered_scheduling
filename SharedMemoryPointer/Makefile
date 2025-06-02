GCC = g++
CFLAGS = -std=c++20 -g -pthread

TARGET = test
SRCS = test.cpp process_primitives.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(GCC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) $(OBJS)

CC = gcc

DEFS = -DX86

CFLAGS = -shared -fPIC $(DEFS)

LIBS = -ldl

OBJS = mem_trace.o rbtree.o mem_coord.o

TARGET = mem_trace.so

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared -fPIC -o $(TARGET) $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) $(TARGET)

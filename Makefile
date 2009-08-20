CC = gcc

# Processor: X86, SPARC, PARISC
# OS: LINUX, SUNOS, HP_UX
DEFS = -DX86 -DLINUX

CFLAGS = $(DEFS)

LIBS = -ldl

OBJS = mem_trace.o rbtree.o mem_coord.o

TARGET = mem_trace.so

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared -fPIC -o $(TARGET) $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) $(TARGET)

# Compilador
CC = gcc
CFLAGS = -Wall -g -lrt

# Arquivos-fonte
OBJS = pingpong-disco1.o disk-driver.o ppos_disk.o ppos-core-aux.o ppos-all.o  queue.o 

# Executável final
TARGET = test-disk-porra

# Regra padrão
all: $(TARGET)

# Regra para compilar o executável
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Regras para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpar arquivos objeto e executável
clean:
	rm -f *.o $(TARGET)
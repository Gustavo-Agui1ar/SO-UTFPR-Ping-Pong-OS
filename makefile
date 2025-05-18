# Compilador
CC = gcc
CFLAGS = -Wall -g

# Arquivos-fonte
OBJS = pingpong-contab-prio.o ppos-core-aux.o ppos-all.o queue.o

# Executável final
TARGET = test

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

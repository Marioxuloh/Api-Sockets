# Compilador
CC = gcc

# Opciones de compilación
CFLAGS = -g -Wall

# Nombres de ejecutables
TARGET_SERVER = servidor
TARGET_CLIENT = cliente

# Lista de archivos fuente
SRCS_SERVER = servidor.c
SRCS_CLIENT = cliente.c

# Generación de nombres de objetos (archivos .o)
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

# Reglas para construir ejecutables
all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) $^ -o $@

# Regla para compilar archivos fuente a objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar archivos generados
clean:
	rm -f $(OBJS_SERVER) $(TARGET_SERVER) $(OBJS_CLIENT) $(TARGET_CLIENT)

.PHONY: all clean

# Compilador
CC = gcc

# Opciones de compilación
CFLAGS = -g -Wall

# Nombres de ejecutables
TARGET_SERVER = servidor
TARGET_CLIENT_TCP = cliente_tcp
TARGET_CLIENT_UDP = cliente_udp

# Lista de archivos fuente
SRCS_SERVER = servidor.c
SRCS_CLIENT_TCP = clientcp.c
SRCS_CLIENT_UDP = clientudp.c

# Generación de nombres de objetos (archivos .o)
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT_TCP = $(SRCS_CLIENT_TCP:.c=.o)
OBJS_CLIENT_UDP = $(SRCS_CLIENT_UDP:.c=.o)

# Reglas para construir ejecutables
all: $(TARGET_SERVER) $(TARGET_CLIENT_TCP) $(TARGET_CLIENT_UDP)

$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_CLIENT_TCP): $(OBJS_CLIENT_TCP)
	$(CC) $(CFLAGS) $^ -o $@

$(TARGET_CLIENT_UDP): $(OBJS_CLIENT_UDP)
	$(CC) $(CFLAGS) $^ -o $@

# Regla para compilar archivos fuente a objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regla para limpiar archivos generados
clean:
	rm -f $(OBJS_SERVER) $(TARGET_SERVER) $(OBJS_CLIENT_TCP) $(TARGET_CLIENT_TCP) $(OBJS_CLIENT_UDP) $(TARGET_CLIENT_UDP)

.PHONY: all clean

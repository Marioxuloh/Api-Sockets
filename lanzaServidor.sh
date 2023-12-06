#!/bin/bash

# Fichero: servidor.c
# Autores:
# Mario Sánchez López DNI 70913738T
# Javier Cabo Correa DNI 70959954D

./servidor
./cliente localhost TCP ./ordenes/ordenes.txt &
./cliente localhost TCP ./ordenes/ordenes1.txt &
./cliente localhost TCP ./ordenes/ordenes2.txt &
./cliente localhost UDP ./ordenes/ordenes3.txt &
./cliente localhost UDP ./ordenes/ordenes4.txt &
./cliente localhost UDP ./ordenes/ordenes5.txt &

 
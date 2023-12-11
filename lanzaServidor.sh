#!/bin/bash

# Fichero: servidor.c
# Autores:
# Mario Sánchez López DNI 70913738T
# Javier Cabo Correa DNI 70959954D

./servidor
./cliente nogal TCP ./ordenes/ordenes.txt &
./cliente nogal TCP ./ordenes/ordenes1.txt &
./cliente nogal TCP ./ordenes/ordenes2.txt &
./cliente nogal UDP ./ordenes/ordenes3.txt &
./cliente nogal UDP ./ordenes/ordenes4.txt &
./cliente nogal UDP ./ordenes/ordenes5.txt &

 
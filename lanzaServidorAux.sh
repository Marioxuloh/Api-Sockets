#!/bin/bash

./servidor
./cliente_tcp localhost ./ordenes/ordenes.txt &
./cliente_tcp localhost ./ordenes/ordenes1.txt &
./cliente_tcp localhost ./ordenes/ordenes2.txt &
./cliente_udp localhost ./ordenes/ordenes.txt &
./cliente_udp localhost ./ordenes/ordenes1.txt &
./cliente_udp localhost ./ordenes/ordenes2.txt &
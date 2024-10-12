FROM debian:bullseye-slim

RUN apt-get update
RUN apt-get -y install gcc-riscv64-unknown-elf
RUN apt-get -y install build-essential

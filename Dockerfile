FROM ubuntu:20.04

MAINTAINER kruztw

RUN apt update -y
RUN apt install build-essential -y

RUN mkdir -p /etc/ssl/certs/

WORKDIR /app

COPY ./server.cpp ./pressure_test.cpp ./Makefile /app/

RUN make

CMD /app/server 1234





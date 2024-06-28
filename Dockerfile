FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=nonintercative

RUN  apt-get update -yqq && \
     apt-get install -yqq software-properties-common && \
	apt-get install -yqq sudo curl wget cmake locales git \
     openssl libssl-dev \
     libjsoncpp-dev libuv1-dev \
     uuid-dev libreadline-dev libbison-dev flex \
     zlib1g-dev && \
     add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
	apt-get update -yqq && \
	apt-get install -yqq gcc-10 g++-10

EXPOSE 8080

RUN locale-gen en_US.UTF-8

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

ENV CC=gcc-10
ENV CXX=g++-10
ENV AR=gcc-ar-10
ENV RANLIB=gcc-ranlib-10

WORKDIR /server

COPY ./ ./

RUN git submodule update --init --recursive && \
    mkdir ./build && \
    cd ./build && \
    cmake .. && \
    make

VOLUME ["./files", "/files"]

CMD ./api_server
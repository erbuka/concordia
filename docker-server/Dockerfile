FROM ubuntu:latest as build-stage
WORKDIR /app

# Setup build tools
RUN apt update
RUN apt install wget -y
RUN apt install software-properties-common -y
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt update
RUN apt install gcc-13 g++-13 -y
RUN apt install make -y
RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-linux.tar.gz 
RUN tar xvf premake-5.0.0-beta2-linux.tar.gz
# Sources

COPY ./src/ src/
COPY ./vendor/ vendor/
COPY ./premake5.lua .

# Build

RUN ./premake5 gmake2
RUN link /usr/bin/g++-13 /usr/bin/g++
RUN link /usr/bin/gcc-13 /usr/bin/gcc
RUN export CC=g++
RUN cd ./gmake2 && make config=dist ConcordiaServer
RUN cd ./gmake2 && make config=release ConcordiaServer


FROM ubuntu:latest
WORKDIR /app

RUN apt update && \
  apt install software-properties-common -y && \
  add-apt-repository ppa:ubuntu-toolchain-r/test && \
  apt update && \
  apt install --only-upgrade -y libstdc++6  && \
  apt clean


COPY --from=build-stage /app/bin/Dist/ConcordiaServer/ConcordiaServer .
COPY --from=build-stage /app/bin/Release/ConcordiaServer/ConcordiaServer ./ConcordiaServerDebug

EXPOSE 3000
CMD /app/ConcordiaServer
FROM ubuntu:latest
RUN apt-get update && apt-get -y upgrade
RUN apt-get install -y wget xz-utils curl

WORKDIR /
RUN sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d

RUN wget -O zig.tar.xz https://ziglang.org/download/0.11.0/zig-linux-x86_64-0.11.0.tar.xz && \
   tar xfv zig.tar.xz
RUN rm zig.tar.xz
RUN ln -s /zig-linux-x86_64-0.11.0/zig /bin/zig

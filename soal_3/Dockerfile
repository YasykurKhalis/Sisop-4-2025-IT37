FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    fuse \
    gcc \
    libfuse-dev \
    nano \
    vim \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /antink_mount /it24_host /antink-logs

COPY antink.c /antink.c
RUN gcc -Wall /antink.c `pkg-config fuse --cflags --libs` -o /antink

VOLUME ["/antink_mount", "/it24_host", "/antink-logs"]
CMD ["/antink", "/antink_mount"]

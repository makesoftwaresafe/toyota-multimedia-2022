FROM debian:bullseye-slim
RUN apt-get update && apt-get install -y build-essential libc6-dbg
WORKDIR /build/atftp-0.7.1/
COPY . .
RUN   ./configure --prefix=/usr --disable-dependency-tracking --disable-libpcre --disable-libreadline --enable-libwrap && \
  make 'CFLAGS=-g -DHAVE_ARGZ -fgnu89-inline' && \
  make install && \
  mkdir -p /srv/tftp
CMD "/usr/sbin/atftpd --daemon --no-fork /srv/tftp/"


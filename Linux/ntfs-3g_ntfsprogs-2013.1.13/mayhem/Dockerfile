FROM centos:7 as builder

RUN yum install -y unzip gcc make
RUN mkdir /ntfs-3g-src
COPY . /ntfs-3g-src/
WORKDIR /ntfs-3g-src/
RUN ./configure --prefix=/ntfs-3g --enable-debug && \
    make -j$(grep -c processor /proc/cpuinfo) && \
    make install && \
    make clean

# For fuzzing
FROM centos:7
COPY --from=builder /ntfs-3g /ntfs-3g
COPY --from=builder /lib/libntfs* /lib/
COPY --from=builder /ntfs-3g-src /ntfs-3g-src
RUN ldconfig

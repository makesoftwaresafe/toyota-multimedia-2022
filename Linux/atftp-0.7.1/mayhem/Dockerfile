# atftp 0.7.1 seems fairly old and doesn't compile with a newer GCC without
# flags. On CentOS 6, it just works.
FROM centos:6 as builder
# Point yum to the CentOS vault.
RUN sed -i 's/mirrorlist=/#mirrorlist/g' /etc/yum.repos.d/CentOS-Base.repo
RUN sed -i 's/mirror\.centos/vault\.centos/g' /etc/yum.repos.d/CentOS-Base.repo
RUN sed -i 's/#baseurl/baseurl/g' /etc/yum.repos.d/CentOS-Base.repo
RUN sed -i 's/enabled=0/enabled=1/g' /etc/yum/pluginconf.d/fastestmirror.conf
# Install packages needed to build atftp.
RUN yum install -y make gcc
# Copy in atftp source
ADD . /atftp-src
WORKDIR /atftp-src
RUN ./configure && make

# Start a new image to keep the one we upload small.
FROM centos:6
# Copy over the atftpd binary.
COPY --from=builder /atftp-src/atftpd /usr/local/bin/
# Create a directory that atftpd will serve files from and create a sample
# file.
RUN mkdir /tftpboot; echo hello world > /tftpboot/rfc1350.txt

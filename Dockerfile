FROM ubuntu:16.04
MAINTAINER Hallman He <heyong1949@gmail.com>

ENV DEBIAN_FRONTEND noninteractive

## Install dependencies
RUN apt-get update && apt-get install -y \
    	git \
	net-tools \
	vim \
	supervisor \
	libpcap-dev \
	libjansson-dev \
	gcc \
	&& \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

## Install p0f
RUN cd /opt && \
    git clone -b hpfeeds https://github.com/hallmanhe/p0f.git  && \
    cd p0f && \
    ./build.sh && \
    useradd -d /var/empty/p0f -M -r -s /bin/nologin p0f-user || true && \
    mkdir -p -m 755 /var/empty/p0f && \
    rm -rf /tmp/* /var/tmp/*

COPY p0f.conf /etc/supervisor/conf.d/
RUN mkdir /var/log/p0f

## Configuration
VOlUME /var/log/p0f
WORKDIR /opt/p0f
CMD ["/usr/bin/supervisord","--nodaemon","-c","/etc/supervisor/supervisord.conf"]

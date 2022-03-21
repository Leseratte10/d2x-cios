FROM debian:buster
MAINTAINER Leseratte

# This Dockerfile uses the precompiled devkitARM r32 to compile the d2xl-cIOS
# Update and install a bunch of crap: 
RUN apt-get update -y --force-yes && \
apt-get install -y --force-yes \
make gawk wget bzip2 unzip libmpc-dev libelf-dev gcc g++ git libfl2 libfl-dev

# Download the pre-built devkitARM r32 
# see https://github.com/Leseratte10/compile-devkitARM-r32 for sources and binaries
RUN wget https://github.com/Leseratte10/compile-devkitARM-r32/releases/download/2020-02-20/devkitARM-r32-linux_debian-buster.tar.gz

RUN tar -C /opt -xvf devkitARM-r32-linux_debian-buster.tar.gz 
RUN rm devkitARM-r32-linux_debian-buster.tar.gz

# FAKEMOTE by xerpi please see https://gbatemp.net/threads/release-fakemote-an-ios-module-that-fakes-wiimotes-from-the-input-of-usb-game-controllers.601771/
RUN wget https://github.com/xerpi/fakemote/releases/download/v0.5.0/FAKEMOTE.zip
RUN unzip -d /opt/content FAKEMOTE.zip
RUN rm FAKEMOTE.zip

# At this point, we extracted the working devkitARM setup

COPY dist.sh /dist.sh

ENTRYPOINT ["/dist.sh"]



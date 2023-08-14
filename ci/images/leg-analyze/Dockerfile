# Stolen from @afust
# See https://git.gams.com/devel/qt-machines/-/blob/main/leg/analyzer/Dockerfile
# Didn't take his image directly as we don't need Qt here

FROM ubuntu:20.04

RUN apt-get update &&                                   \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq  \
      apt-utils                     \
      git                           \
      curl                          \
      ninja-build                   \
      flex                          \
      fuse                          \
      tree                          \
      python                        \
      python3                       \
      python3-html5lib              \
      python3-pip                   \
      wget                          \
      build-essential               \
      libssl-dev                    \
      gperf                         \
      bison

### More recent cmake to get junit XML output as option for ctest
RUN curl -OL https://cloud.gams.com/s/E4Bkqdcq56rb8Zn/download/cmake-3.23.1-linux-x86_64.tar.gz && \
    tar -xzvf cmake-3.23.1-linux-x86_64.tar.gz && \
    rm cmake-3.23.1-linux-x86_64.tar.gz && \
    mkdir -p /opt/cmake && \
    mv cmake-3.23.1-linux-x86_64/* /opt/cmake
ENV PATH="$PATH:/opt/cmake/bin"

# Requirements for CodeChecker
RUN apt-get update && apt-get install -y clang clang-tools clang-tidy cppcheck
### Install codechecker, version 6.22.2 has a LD_PRELOAD bug... wait for the fix
RUN pip3 install codechecker==6.22.1
RUN apt-get clean

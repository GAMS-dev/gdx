# Stolen from @afust
# See https://git.gams.com/devel/qt-machines/-/blob/main/leg/analyzer/Dockerfile
# Didn't take his image directly as we don't need Qt here

FROM ubuntu:24.04

RUN apt-get update &&                                   \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq  \
      apt-utils                     \
      git                           \
      curl                          \
      ninja-build                   \
      flex                          \
      fuse3                         \
      tree                          \
      python-is-python3             \
      python3                       \
      python3-dev                   \
      python3-html5lib              \
      python3-pip                   \
      pipx                          \
      wget                          \
      build-essential               \
      libssl-dev                    \
      gperf                         \
      cmake                         \
      cython3                       \
      libxml2-dev                   \
      libxslt1-dev                  \
      bison &&                      \
    python3 -m venv /opt/codechecker_env && \
    /opt/codechecker_env/bin/pip install --upgrade pip setuptools wheel && \
    /opt/codechecker_env/bin/pip install codechecker==6.22.1 && \
    ln -s /opt/codechecker_env/bin/codechecker /usr/local/bin/codechecker  
### Install codechecker, version 6.22.2 has a LD_PRELOAD bug... wait for the fix

# Requirements for CodeChecker
RUN apt-get update && apt-get install -y clang clang-tools clang-tidy cppcheck
RUN apt-get clean

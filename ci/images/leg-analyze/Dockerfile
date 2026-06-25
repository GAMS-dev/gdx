FROM ubuntu:26.04

# Pre-requisites and setting up LLVM 22 repository
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq curl wget gnupg2 ca-certificates && \
    mkdir -p /etc/apt/keyrings && \
    wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | tee /etc/apt/keyrings/apt.llvm.org.asc && \
    echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/apt.llvm.org.asc] http://apt.llvm.org/resolute/ llvm-toolchain-resolute-22 main" | tee /etc/apt/sources.list.d/llvm-22.list

RUN apt-get update &&                                   \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq  \
      apt-utils                     \
      unzip                         \
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
    /opt/codechecker_env/bin/pip install codechecker && \
    ln -s /opt/codechecker_env/bin/CodeChecker /usr/local/bin/CodeChecker

# Requirements for CodeChecker explicitly pinpointed to Clang 22
RUN apt-get update && apt-get install -y clang-22 clang-tools-22 clang-tidy-22 clang++-22 cppcheck && \
    ln -s /usr/bin/clang-22 /usr/bin/clang && \
    ln -s /usr/bin/clang++-22 /usr/bin/clang++ && \
    ln -s /usr/bin/clang-tidy-22 /usr/bin/clang-tidy

RUN apt-get clean

RUN ln -s /opt/codechecker_env/lib/python3.14/site-packages/codechecker_analyzer/ld_logger/lib/x86_64/ldlogger.so /lib/ldlogger.so
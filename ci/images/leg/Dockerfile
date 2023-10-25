FROM centos:centos7

RUN yum -y update && \
  yum -y install bash-completion less which nano bzip2 zip unzip python3 openssh-clients && \
  yum clean all

# install git from source
ARG GIT_VERSION=2.40.0
RUN yum -y install gcc make libcurl-devel zlib-devel openssl-devel expat-devel && \
  cd /tmp && \
  curl -O https://mirrors.edge.kernel.org/pub/software/scm/git/git-${GIT_VERSION}.tar.xz && \
  tar xJf git-${GIT_VERSION}.tar.xz && \
  cd git-${GIT_VERSION} && \
  ./configure --without-tcltk && \
  make -j4 install NO_GETTEXT=YesPlease && \
  cp contrib/completion/git-completion.bash contrib/completion/git-prompt.sh /etc/bash_completion.d/ && \
  rm -rf /tmp/git-${GIT_VERSION}* && \
  yum -y remove gcc make libcurl-devel zlib-devel openssl-devel expat-devel && \
  yum -y autoremove && \
  yum clean all

RUN pip3 install --user PyYAML Jinja2 Cerberus pandas beautifulsoup4

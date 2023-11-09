# Mostly stolen from @afust
# Extends Ubuntu oldest still supported LTS
FROM ubuntu:22.04

RUN apt-get update &&       \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq \
      bzip2  \
      parallel  \
      s3cmd \
      subversion \
      unzip \
      python3 \
      python3-html5lib \
      python3-pip \
      wget && \
      pip3 install --user PyYAML

### Make github-release tool available (https://github.com/github-release/github-release)
RUN wget https://cloud.gams.com/s/NJ26xfETjadAC23/download/github-release-binaries.zip  &&  \
    mkdir -p /opt/github-release                                                        &&  \
    unzip github-release-binaries.zip                                                   &&  \
    chmod u+x leg-github-release                                                        &&  \
    mv leg-github-release /opt/github-release/github-release                            &&  \
    rm deg-github-release wei-github-release.exe

### Make release-cli tool available (https://docs.gitlab.com/ee/user/project/releases/release_cli.html) \
RUN wget -O /usr/local/bin/release-cli "https://gitlab.com/api/v4/projects/gitlab-org%2Frelease-cli/packages/generic/release-cli/latest/release-cli-linux-amd64" && \
    chmod u+x /usr/local/bin/release-cli

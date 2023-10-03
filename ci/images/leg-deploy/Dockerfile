# Mostly stolen from @afust
# Extends Ubuntu oldest still supported LTS
FROM ubuntu:22.04

RUN apt-get update &&       \
    DEBIAN_FRONTEND=noninteractive apt-get install -yq bzip2 parallel s3cmd subversion unzip wget

### Make github-release tool available (https://github.com/github-release/github-release)
RUN wget https://cloud.gams.com/s/NJ26xfETjadAC23/download/github-release-binaries.zip  &&  \
    mkdir -p /opt/github-release                                                        &&  \
    unzip github-release-binaries.zip                                                   &&  \
    chmod u+x leg-github-release                                                        &&  \
    mv leg-github-release /opt/github-release/github-release                            &&  \
    rm deg-github-release wei-github-release.exe

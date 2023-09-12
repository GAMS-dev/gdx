FROM centos:centos7
RUN yum -y update && \
  yum -y install python3 && \
  yum clean all
RUN pip3 install --user PyYAML Jinja2 Cerberus pandas

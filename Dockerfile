FROM ubuntu:22.04
RUN export DEBIAN_FRONTEND=noninteractive; apt update; apt install -y curl make git-core cmake python2 python3 sudo wget bzip2 xz-utils
RUN export VITASDK=/usr/local/vitasdk; export PATH=$VITASDK/bin:$PATH; git clone https://github.com/vitasdk/vdpm; cd vdpm; ./bootstrap-vitasdk.sh; ./install-all.sh
RUN echo 'export VITASDK=/usr/local/vitasdk' > /etc/profile.d/psvsdk.sh
RUN echo 'export PATH="$VITASDK/bin:$PATH"' >> /etc/profile.d/psvsdk.sh
ENTRYPOINT ["/bin/bash", "-l"]

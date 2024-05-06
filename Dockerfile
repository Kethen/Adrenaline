FROM ubuntu:22.04
RUN export DEBIAN_FRONTEND=noninteractive; apt update; apt install -y curl make git-core cmake python2 python3 sudo wget bzip2 xz-utils libreadline8 libusb-0.1-4 tmux libmpc3
RUN export VITASDK=/usr/local/vitasdk; export PATH=$VITASDK/bin:$PATH; git clone https://github.com/vitasdk/vdpm; cd vdpm; ./bootstrap-vitasdk.sh; ./install-all.sh
RUN echo 'export VITASDK=/usr/local/vitasdk' > /etc/profile.d/psvsdk.sh
RUN echo 'export PATH="$VITASDK/bin:$PATH"' >> /etc/profile.d/psvsdk.sh

RUN wget https://github.com/pspdev/pspdev/releases/download/latest/pspdev-ubuntu-latest.tar.gz -O - | gzip -d | tar -C /usr/local -x
RUN echo 'export PATH="/usr/local/pspdev/bin:$PATH"' > /etc/profile.d/pspsdk.sh
RUN echo 'export LD_LIBRARY_PATH="/usr/local/pspsdk/lib:$LD_LIBRARY_PATH"' >> /etc/profile.d/pspsdk.sh

ENTRYPOINT ["/bin/bash", "-l"]

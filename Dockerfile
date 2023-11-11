FROM ubuntu:20.04
# TODO: shared
RUN apt-get update && apt-get install -y git gcc make uuid-dev vim bash curl libpcre3-dev
RUN DEBIAN_FRONTEND=noninteractive apt-get -yq install bsd-mailx
RUN service postfix start

RUN sh -c "`curl -L https://raw.githubusercontent.com/rylnd/shpec/master/install.sh`"

COPY scripts/entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]

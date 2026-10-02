# CI image for the target-build job (.github/workflows/ci.yml): RTEMS 7
# SPARC toolchain + sparc/leon3 BSP, built reproducibly with the RTEMS
# Source Builder at the commits used for development, plus Renode for the
# end-to-end run. Building the toolchain takes about an hour, which is why CI
# pulls this prebuilt image instead of building it per run.
#
#   docker build -f tools/docker/rtems7-leon3.Dockerfile -t ghcr.io/<owner>/rtems7-leon3:1 .
#   docker push ghcr.io/<owner>/rtems7-leon3:1
#
# then set the repository variable RTEMS_TOOLCHAIN_IMAGE to that tag.

ARG UBUNTU=ubuntu:24.04

FROM ${UBUNTU} AS toolchain
ARG RSB_COMMIT=b6d95403cc142c4abd04df80bb27b5b28efe38d6
ARG RTEMS_COMMIT=e9d427c097d1993053488306eaa85f1e6d3ac961
ARG PREFIX=/opt/rtems7
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      build-essential git ca-certificates curl python3 python3-dev python-is-python3 \
      bison flex texinfo unzip xz-utils bzip2 zlib1g-dev libncurses-dev pax file \
    && rm -rf /var/lib/apt/lists/*
RUN git clone https://gitlab.rtems.org/rtems/tools/rtems-source-builder.git /src/rsb \
    && git -C /src/rsb checkout ${RSB_COMMIT}
RUN cd /src/rsb/rtems \
    && ../source-builder/sb-set-builder --prefix=${PREFIX} --no-clean 7/rtems-sparc \
    && rm -rf /src/rsb/rtems/build /src/rsb/rtems/sources
RUN git clone https://gitlab.rtems.org/rtems/rtos/rtems.git /src/rtems \
    && git -C /src/rtems checkout ${RTEMS_COMMIT} \
    && cd /src/rtems \
    && printf '[sparc/leon3]\n' > config.ini \
    && ./waf configure --prefix=${PREFIX} \
    && ./waf \
    && ./waf install

FROM ${UBUNTU}
ARG PREFIX=/opt/rtems7
ARG RENODE_VERSION=1.17.0
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      cmake make git ca-certificates curl python3 libicu74 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=toolchain ${PREFIX} ${PREFIX}
RUN curl -sSL https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/renode-${RENODE_VERSION}.linux-portable.tar.gz \
      | tar -xz -C /opt \
    && mv /opt/renode_${RENODE_VERSION}-portable /opt/renode
ENV PATH=${PREFIX}/bin:/opt/renode:${PATH} \
    RENODE=/opt/renode/renode

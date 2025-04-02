FROM gitlab.i2cat.net:5050/areas/mwi/v2x/toolchains/unex-toolchain:0.0.1

COPY src/ /src

WORKDIR /src

ENTRYPOINT [ "make" ]
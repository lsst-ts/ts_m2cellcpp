FROM lsstts/develop-env:c0028.004 as crio-develop

ARG XML_BRANCH=develop

USER root
# Copying files and run chown and chmod before changing user.
COPY . repos/ts_m2cellcpp
RUN chown -R saluser.saluser repos/ts_m2cellcpp
RUN chmod a+rwX -R /home/saluser/

USER saluser

ARG XML_BRANCH=main

WORKDIR /home/saluser

RUN source ~/.setup.sh \
    && mamba install -y readline yaml-cpp catch2 spdlog nlohmann_json \
    && echo > .crio_setup.sh -e \
echo "Configuring cRIO development environment" \\n\
export SHELL=bash \\n\
source /home/saluser/.setup_salobj.sh \\n\
export PATH=\$CONDA_PREFIX/bin:\$PATH \\n\
export LIBS="-L\$CONDA_PREFIX/lib" \\n\
export CPP_FLAGS="-I\$CONDA_PREFIX/include" \\n

FROM crio-develop

#&&& ARG M2_CELLCPP=develop

#&&& RUN cd ~/repos \
#&&&  && git clone --branch $M2_CELLCPP https://github.com/lsst-ts/ts_m2cellcpp.git \
#&&&  && source ~/.crio_setup.sh \
#&&&  && cd ~/repos/ts_m2cellcpp \
#&&&  && make

#&&& ARG cRIO_CPP=v1.5.1
#&&& ARG M1M3_SUPPORT=develop
#&&& ARG TARGET=simulator

#&&& RUN cd repos && git clone --branch $cRIO_CPP https://github.com/lsst-ts/ts_cRIOcpp
#&&& RUN source ~/.crio_setup.sh && cd repos/ts_cRIOcpp && make

#&&& RUN cd repos && git clone --branch $M1M3_SUPPORT https://github.com/lsst-ts/ts_m1m3support
#&&& RUN source ~/.crio_setup.sh && cd repos/ts_m1m3support && make $TARGET

# The current version in of develop in gtihub won't compile under this system,
# so add local files until they do compile.


# Build executables. Cleaning is required to remove any local .o files
# that may have been copied.
RUN source ~/.crio_setup.sh \
  && cd ~/repos \
  && cd ~/repos/ts_m2cellcpp \
  && make clean \
  && make tests -j10




SHELL ["/bin/bash", "-lc"]

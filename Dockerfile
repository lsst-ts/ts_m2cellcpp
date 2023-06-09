FROM lsstts/develop-env:c0028.004 as crio-develop

USER root
RUN chmod a+rwX -R /home/saluser/

USER saluser

WORKDIR /home/saluser

RUN source ~/.setup.sh \
    && mamba install -y readline yaml-cpp catch2 spdlog nlohmann_json boost \
    && echo > .crio_setup.sh -e \
echo "Configuring cRIO development environment" \\n\
export SHELL=bash \\n\
source /home/saluser/.setup_salobj.sh \\n\
export PATH=\$CONDA_PREFIX/bin:\$PATH \\n\
export LIBS="-L\$CONDA_PREFIX/lib" \\n\
export CPP_FLAGS="-I\$CONDA_PREFIX/include" \\n

FROM crio-develop

ARG M2_CELLCPP=develop

RUN source ~/.crio_setup.sh \
  && cd ~/repos \
  && git clone --branch $M2_CELLCPP https://github.com/lsst-ts/ts_m2cellcpp.git \
  && cd ~/repos/ts_m2cellcpp \
  && make \
  && make tests -j10

SHELL ["/bin/bash", "-lc"]

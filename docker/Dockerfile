
FROM lsstsqre/centos:w_latest

USER root

workdir /home/ts
RUN source /opt/lsst/software/stack/loadLSST.bash && \
    conda install -y catch2 boost asio gcovr doxygen && \
    conda install -y -c conda-forge nlohmann_json && \
    pip install ltd-conveyor && \
    yum update && \
    yum install -y wget && \
    wget https://www.ni.com/en-us/support/downloads/software-products/download/unpackaged-linux.gnu-c---c---compile-tools-x64.338443.html && \
    ./oecore-x86_64-core2-64-toolchain-6.0.sh -y  


#RUN source /home/saluser/.setup.sh && \
#    conda install -c lsstts ts-idl=${idl}

# RUN mkdir /home/saluser/miniconda3/lib/python3.7/schema/

# WORKDIR /home/saluser/miniconda3/lib/python3.7/schema/

# RUN wget https://raw.githubusercontent.com/lsst-ts/ts_salobj/develop/schema/Test.yaml

#WORKDIR /home/saluser/
#RUN git clone https://github.com/lsst-ts/ts_config_ocs.git

#ENV TS_CONFIG_OCS_DIR=/home/saluser/ts_config_ocs
#ENV RUN_ARG=1
#COPY setup.sh /home/saluser/.setup.sh

#COPY run_testing.py /home/saluser/run_testing.py
#USER root

#RUN chown saluser:saluser /home/saluser/run_testing.py && \
#    chmod a+x /home/saluser/run_testing.py

#USER saluser

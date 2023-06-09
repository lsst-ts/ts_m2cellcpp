#!/usr/bin/env groovy

properties(
    [
    buildDiscarder
        (logRotator (
            artifactDaysToKeepStr: '',
            artifactNumToKeepStr: '',
            daysToKeepStr: '14',
            numToKeepStr: ''
        ) ),
    disableConcurrentBuilds(),
    parameters
        ( [
            booleanParam(defaultValue: false, description: 'Adds --no-cache to Docker build command', name: 'noCache'),
            booleanParam(defaultValue: false, description: 'Calls make clean before building the code', name: 'clean')
        ] )
    ]
)

node {
    def SALUSER_HOME = "/home/saluser"
    def BRANCH = (env.CHANGE_BRANCH != null) ? env.CHANGE_BRANCH : env.BRANCH_NAME
    stage('Cloning sources')
    {
        dir("ts_m2cellcpp") {
            git branch: BRANCH, url: 'https://github.com/lsst-ts/ts_m2cellcpp'
        }
    }

    stage('Building dev container')
    {
        M2cellcpp = docker.build("m2cellcpp:" + env.BRANCH_NAME.replace("/", "_"), "--target crio-develop " + (params.noCache ? "--no-cache " : " ") + "$WORKSPACE/ts_m2cellcpp")
    }

    stage("Running tests")
    {
        withEnv(["SALUSER_HOME=" + SALUSER_HOME]) {
            M2cellcpp.inside("--entrypoint=''") {
                 if (params.clean) {
                 sh """
                    cd $WORKSPACE/ts_m2cellcpp
                    make clean
                 """
                 }
                 sh """
                    source $SALUSER_HOME/.crio_setup.sh

                    cd $WORKSPACE/ts_m2cellcpp

                    make run_tests
                    make junit || true
                 """
             }
        }

        junit 'ts_m2cellcpp/tests/*.xml'
    }

    stage('Build documentation')
    {
        M2cellcpp.inside("--entrypoint=''") {
            sh """
                source $SALUSER_HOME/.crio_setup.sh
                cd $WORKSPACE/ts_m2cellcpp
                make doc
            """
        }
    }

    if (BRANCH == "main" || BRANCH == "develop")
    {
        stage('Publish documentation')
        {
            withCredentials([usernamePassword(credentialsId: 'lsst-io', usernameVariable: 'LTD_USERNAME', passwordVariable: 'LTD_PASSWORD')]) {
                M2cellcpp.inside("--entrypoint=''") {
                    sh """
                        source $SALUSER_HOME/.crio_setup.sh
                        ltd upload --product ts-m2cellcpp --git-ref """ + BRANCH + """ --dir $WORKSPACE/ts_m2cellcpp/doc/html
                    """
                }
            }
        }
    }
}

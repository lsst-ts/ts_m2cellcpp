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
    // branches requiring changes in XML from default develop branch
    //&&& def XML_BRANCH = BRANCH in ["main", "tickets/DM-38711"] ? BRANCH : "develop"
    //&&& def CRIO_BRANCH = BRANCH in ["main", "tickets/DM-38668"] ? BRANCH : "develop"
    stage('Cloning sources')
    {
        dir("ts_m2cellcpp") {
            git branch: BRANCH, url: 'https://github.com/lsst-ts/ts_m2cellcpp'
        }
        //&&& dir("ts_cRIOcpp") {
        //&&&     git branch: CRIO_BRANCH, url: 'https://github.com/lsst-ts/ts_cRIOcpp'
        //&&& }
        //&&& dir("ts_m1m3support") {
        //&&&     git branch: BRANCH, url: 'https://github.com/lsst-ts/ts_m1m3support'
        //&&& }

    }

    stage('Building dev container')
    {
        //&&& M1M3sim = docker.build("m1m3sim:" + env.BRANCH_NAME.replace("/", "_"), "--target crio-develop --build-arg XML_BRANCH=$XML_BRANCH " + (params.noCache ? "--no-cache " : " ") + "$WORKSPACE/ts_m1m3support")
        M2cellcpp = docker.build("m2cellcpp:" + env.BRANCH_NAME.replace("/", "_"), "--target crio-develop --build-arg XML_BRANCH=$XML_BRANCH " + (params.noCache ? "--no-cache " : " ") + "$WORKSPACE/ts_m2cellcpp")
    }

    //&&& stage("Running tests")
    //&&& {
    //&&&     withEnv(["SALUSER_HOME=" + SALUSER_HOME]) {
    //&&&          //&&& M1M3sim.inside("--entrypoint=''") {
    //&&&              if (params.clean) {
    //&&&              sh """
    //&&&                 cd $WORKSPACE/ts_cRIOcpp
    //&&&                 make clean
    //&&&                 cd $WORKSPACE/ts_m1m3support
    //&&&                 make clean
    //&&&              """
    //&&&              }
    //&&&              sh """
    //&&&                 source $SALUSER_HOME/.crio_setup.sh
    //&&&
    //&&&                 cd $WORKSPACE/ts_cRIOcpp
    //&&&                 make
    //&&&
    //&&&                 cd $WORKSPACE/ts_m1m3support
    //&&&                 make simulator
    //&&&                 LSST_DDS_PARTITION_PREFIX=test make junit || true
    //&&&              """
    //&&&          }
    //&&&     }
    //&&&     junit 'ts_m1m3support/tests/*.xml'
    //&&& }

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
                    make junit

                 """
             }
        }

        // The path of xml needed by JUnit is relative to
        // the workspace.
        junit "${env.XML_REPORT}"
        // Publish the coverage report
        cobertura coberturaReportFile: 'jenkinsReportCov/*.xml'
    }

    //&&& original Jenkins for m2cellcpp
    //&&& stage ('Unit Tests') {
    //&&&     steps {
    //&&&         withEnv(["HOME=${env.WORKSPACE}"]) {
    //&&&             sh """
    //&&&                 source ${env.LSST_STACK}/loadLSST.bash
    //&&&                 make
    //&&&                 make run_tests
    //&&&                 make junit
    //&&&                 mkdir jenkinsReportCov/
    //&&&                     gcovr -r src/ . --xml-pretty > ${HOME}/${env.XML_REPORT_COVERAGE}
    //&&&             """
    //&&&         }
    //&&&         // The path of xml needed by JUnit is relative to
    //&&&         // the workspace.
    //&&&         junit "${env.XML_REPORT}"
    //&&&         // Publish the coverage report
    //&&&         cobertura coberturaReportFile: 'jenkinsReportCov/*.xml'
    //&&&     }
    //&&& }

    //&&& stage('Build documentation')
    //&&& {
    //&&&      M1M3sim.inside("--entrypoint=''") {
    //&&&          sh """
    //&&&             source $SALUSER_HOME/.crio_setup.sh
    //&&&             cd $WORKSPACE/ts_m1m3support
    //&&&             make doc
    //&&&          """
    //&&&      }
    //&&& }

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

    // &&& what is this doing?
    //stage('Running container')
    //{
    //    withEnv(["SALUSER_HOME=" + SALUSER_HOME]){
    //        M1M3sim.inside("--entrypoint=''") {
    //            sh """
    //                source $SALUSER_HOME/.crio_setup.sh
    //
    //                export LSST_DDS_PARTITION_PREFIX=test
    //
    //                cd $WORKSPACE/ts_m1m3support
    //                ./ts-M1M3supportd -c SettingFiles &
    //
    //                echo "Waiting for 30 seconds"
    //                sleep 30
    //
    //                cd $SALUSER_HOME/repos
    //                ./ts_sal/test/MTM1M3/cpp/src/sacpp_MTM1M3_start_commander Default
    //                sleep 30
    //                killall ts-M1M3supportd
    //            """
    //        }
    //    }
    //}

    stage('Running container')
    {
        withEnv(["SALUSER_HOME=" + SALUSER_HOME]){
            M2cellcpp.inside("--entrypoint=''") {
                sh """
                    source $SALUSER_HOME/.crio_setup.sh

                    cd $WORKSPACE/ts_m2c2llcpp

                    cd $SALUSER_HOME/repos
                """
            }
        }
    }

    // &&& Not sure if this applies to m2cellcpp at this time.
    // if (BRANCH == "main" || BRANCH == "develop")
    //{
    //    stage('Publish documentation')
    //    {
    //        withCredentials([usernamePassword(credentialsId: 'lsst-io', usernameVariable: 'LTD_USERNAME', passwordVariable: 'LTD_PASSWORD')]) {
    //            M1M3sim.inside("--entrypoint=''") {
    //                sh """
    //                    source $SALUSER_HOME/.crio_setup.sh
    //                    ltd upload --product ts-m1m3support --git-ref """ + BRANCH + """ --dir $WORKSPACE/ts_m1m3support/doc/html
    //                """
    //            }
    //        }
    //    }
    //}
}

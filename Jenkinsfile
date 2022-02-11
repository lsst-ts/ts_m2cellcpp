#!/usr/bin/env groovy

pipeline {

    agent {
        docker {
          image 'lsstsqre/centos:w_latest'
          args '-u root'
        }
    }

    options {
      disableConcurrentBuilds()
    }

    triggers {
        pollSCM('H H(0-7) * * 1')
    }

    environment {
        // Position of LSST stack directory
        LSST_STACK = "/opt/lsst/software/stack"
        // XML report path
        XML_REPORT = "tests/*.xml"
        // Authority to publish the document online
        user_ci = credentials('lsst-io')
        LTD_USERNAME = "${user_ci_USR}"
        LTD_PASSWORD = "${user_ci_PSW}"
    }

    stages {

        stage ('Install the Dependencies') {
            steps {
                // When using the docker container, we need to change
                // the HOME path to WORKSPACE to have the authority
                // to install the packages.
                withEnv(["HOME=${env.WORKSPACE}"]) {
                    sh """
                        source ${env.LSST_STACK}/loadLSST.bash
                        conda install -y catch2 boost asio gcovr doxygen
                    """
                }
            }
        }

        stage('Unit Tests') {
            steps {
                withEnv(["HOME=${env.WORKSPACE}"]) {
                    sh """
                        source ${env.LSST_STACK}/loadLSST.bash

                        make
                        make run_tests
                        make junit
                    """
                }
                // The path of xml needed by JUnit is relative to
                // the workspace.
                junit "${env.XML_REPORT}"
            }
        }

        stage('Code Coverage') {
            steps {
                withEnv(["HOME=${env.WORKSPACE}"]) {
                    sh """
                        source ${env.LSST_STACK}/loadLSST.bash

                        mkdir jenkinsReportCov/
                        make tests
                        cd tests
                        gcovr -r ../src/ . --xml-pretty > ${HOME}/${env.XML_REPORT_COVERAGE}
                    """
                }
            }
        }


        stage('Publish documentation') {
            steps {
                withEnv(["HOME=${env.WORKSPACE}"]) {
                    sh """
                        source ${env.LSST_STACK}/loadLSST.bash

                        make doc
                    """
                    //    ltd upload --product ${env.DOCUMENT_NAME} --git-ref ${BRANCH} --dir doc/html
                }
            }
        }
    }

    post {
        always {
            // Publish the coverage report
            cobertura coberturaReportFile: 'jenkinsReportCov/*.xml'

            // Change the ownership of workspace to Jenkins for the clean up
            // This is to work around the condition that the user ID of jenkins
            // is 1003 on TSSW Jenkins instance. In this post stage, it is the
            // jenkins to do the following clean up instead of the root in the
            // docker container.
            withEnv(["HOME=${env.WORKSPACE}"]) {
                sh """
                    chown -R 1003:1003 ${HOME}/
                """
            }
        }

        cleanup {
            // clean up the workspace
            deleteDir()
        }
    }
}

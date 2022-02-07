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
                        conda install -y yaml-cpp catch2 boost asio
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
    }

    post {
        always {
            // Change the ownership of workspace to Jenkins for the clean up
            // This is to work around the condition that the user ID of jenkins
            // is 1003 on TSSW Jenkins instance. In this post stage, it is the
            // jenkins to do the following clean up instead of the root in the
            // docker container.
            // Remove the ".conda/" and ".eups/" because the deleteDir() can not
            // remove the hidden directories.
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

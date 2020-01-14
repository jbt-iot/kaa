@Library('jbt-shared-lib@29872b4d') _

import com.jbt.jenkins.Container

def LIB = Container.builder(this)
LIB.init()

env.GITHUB_HTTP_URL = 'https://github.com/jbt-iot/kaa'
env.GITHUB_GIT_URL = 'git@github.com:jbt-iot/kaa.git'

// noinspection GroovyAssignabilityCheck
properties([

        parameters([
                string(
                        defaultValue: '0.9.1-SNAPSHOT',
                        description: 'Kaa version',
                        name: 'VERSION'
                ),
                string(
                        defaultValue: 'http://10.0.1.5:5000',
                        description: 'Aptly URL',
                        name: 'APTLY_URL'
                ),
                booleanParam(
                  name: 'HELM_CHART_ONLY',
                  defaultValue: false,
                  description: 'Builds only helm chart'
                )

        ]),

        [
                $class         : 'BuildBlockerProperty',
                blockingJobs   : '.*jbt-.*',
                useBuildBlocker: true
        ],

        [
                $class       : 'GithubProjectProperty',
                displayName  : '',
                projectUrlStr: env.GITHUB_HTTP_URL
        ],

        [
                $class  : 'BuildDiscarderProperty',
                strategy: [
                        $class               : 'LogRotator',
                        artifactDaysToKeepStr: '7',
                        artifactNumToKeepStr : '7',
                        daysToKeepStr        : '7',
                        numToKeepStr         : '7'
                ]
        ],

])

def gitCredentialsId = '1e989ebf-11b4-458c-8ef7-90256dd62c87'

def jbtInfrastructureBranch = 'master'
def jbtKaaAgentBuilderBranch = 'master'
def jbtBackendBranch = 'master'
def jbtIotBranch = 'master'
def jbtQaE2eBranch = 'master'


def userId = currentBuild.rawBuild.getCauses().find { it instanceof hudson.model.Cause.UserIdCause }?.getUserId() ?: "jenkins"
env.AWS_S3_ATHENA_DB_SUFFIX = "qa_${userId}_${env.BUILD_TAG}"

def isPR() {
    return env.BRANCH_NAME ==~ /PR-\d+/
}

def isMaster() {
    return env.BRANCH_NAME == '0.9.0-patched'
}

def kaaBranch = "none"
def kaaCommit = "00000000"
def kaaTag = "untagged"


def selectNode() {
    if (isPR()) {
        return "slave-02 || jenkins-dynamic"
    } else {
        return "master || jenkins-dynamic"
    }
}

node(selectNode()) {

    stage('init') {
        step([$class: 'WsCleanup'])

        assureJava()
        assureAws()
        assureDockerCompose()
        assureMaven()

    }

    stage('git') {
        sshagent(credentials: ["${gitCredentialsId}"]) {

            dir('kaa') {
                if (isPR()) {
                    echo "Checkout PR: ${env.BRANCH_NAME}"
                    git(
                            branch: "${env.CHANGE_TARGET}",
                            credentialsId: "${gitCredentialsId}",
                            url: 'git@github.com:jbt-iot/kaa.git'
                    )

                    sh "git fetch origin pull/${CHANGE_ID}/merge:${env.BRANCH_NAME}"
                    sh "git checkout ${env.BRANCH_NAME}"

                    kaaBranch = "${env.CHANGE_TARGET}"

                } else {
                    echo "Checkout branch: ${env.BRANCH_NAME}"
                    git(
                            branch: "${env.BRANCH_NAME}",
                            credentialsId: "${gitCredentialsId}",
                            url: 'git@github.com:jbt-iot/kaa.git'
                    )
                    kaaBranch = "${env.BRANCH_NAME}"
                }

                kaaCommit = sh(
                        script: "git rev-parse ${env.BRANCH_NAME}",
                        returnStdout: true
                ).trim().take(8)

                kaaTag = "${kaaBranch}-${kaaCommit}"

                currentBuild.description = "kaa: ${env.BRANCH_NAME}-${kaaCommit}"
            }

            dir('jbt-infrastructure') {
                git(
                        branch: "${jbtInfrastructureBranch}",
                        credentialsId: "${gitCredentialsId}",
                        url: 'git@github.com:jbt-iot/jbt-infrastructure.git'
                )
                def bldCommit = sh(
                        script: "git rev-parse ${jbtInfrastructureBranch}",
                        returnStdout: true
                ).trim().take(8)

                currentBuild.description += " inf: ${jbtInfrastructureBranch}-${bldCommit}"
            }


            dir('jbt-kaa-agent-builder') {
                git(
                        branch: "${jbtKaaAgentBuilderBranch}",
                        credentialsId: "${gitCredentialsId}",
                        url: 'git@github.com:jbt-iot/jbt-kaa-agent-builder.git'
                )
            }

            dir('jbt-backend') {
                git(
                        branch: "${jbtBackendBranch}",
                        credentialsId: "${gitCredentialsId}",
                        url: 'git@github.com:jbt-iot/jbt-backend.git'
                )
            }

            dir('JBT-IoT') {
                git(
                        branch: "${jbtIotBranch}",
                        credentialsId: "${gitCredentialsId}",
                        url: 'git@github.com:jbt-iot/JBT-IoT.git'
                )
            }

            dir('jbt-qa-e2e') {
                git(
                        branch: "${jbtQaE2eBranch}",
                        credentialsId: "${gitCredentialsId}",
                        url: 'git@github.com:jbt-iot/jbt-qa-e2e.git'
                )
            }

        }
    }


    stage('build kaa deb') {
        dir('kaa') {
            sh "KAA_VERSION=${env.VERSION} envsubst < ./server/node/src/deb/control/control.template > ./server/node/src/deb/control/control"
            if (isPR()) {
                sh "mvn -P compile-gwt,cassandra-dao,postgresql-dao,kafka clean package verify"
            } else {
                sh "mvn -DskipTests -DskipITs -P compile-gwt,cassandra-dao,postgresql-dao,kafka clean package"
            }
        }
    }

    stage('build kaa docker') {
        if (isPR()) {
            echo "skip build kaa docker for PR builds"
            return
        }
        withCredentials([string(credentialsId: 'ARTIFACTORY_PASS', variable: 'ARTIFACTORY_PASS')]) {
            dir('jbt-infrastructure') {
                sh """#!/bin/bash
                    set -e
                    set -x
                    cd nix
        
                    `aws ecr get-login --region us-east-1 --no-include-email`
        
                    cp ../../kaa/server/node/target/kaa-node.deb .
                    cp ../../kaa/server/node/target/sdk/cpp/kaa-cpp-ep-sdk-0.9.0.tar.gz .
        
                    docker build -t 138150065595.dkr.ecr.us-east-1.amazonaws.com/kaa:${kaaTag} .
                    docker push 138150065595.dkr.ecr.us-east-1.amazonaws.com/kaa:${kaaTag}
        
                    docker build -t 138150065595.dkr.ecr.us-east-1.amazonaws.com/kaa:${kaaBranch} .
                    docker push 138150065595.dkr.ecr.us-east-1.amazonaws.com/kaa:${kaaBranch}
            """
            }
        }

    }

    stage('push kaa sdk to artifactory') {
        if (isPR()) {
            echo "skip build kaa docker for PR builds"
            return
        }
        withCredentials([string(credentialsId: 'ARTIFACTORY_PASS', variable: 'ARTIFACTORY_PASS')]) {
            dir('kaa') {
                sh """#!/bin/bash
                    set -ex
                    tarMD5=`md5sum ./server/node/target/sdk/cpp/kaa-cpp-ep-sdk-0.9.0.tar.gz | awk '{print \$1}'`
                    
                    ARTIFACTORY_URL="http://artifactory.jbt-iops.com:8081/artifactory/example-repo-local"
                    
                    curl -uadmin:${ARTIFACTORY_PASS} --upload-file ./server/node/target/sdk/cpp/kaa-cpp-ep-sdk-0.9.0.tar.gz --header "X-Checksum-MD5:\${tarMD5}" "\${ARTIFACTORY_URL}/kaa-sdk/kaa-cpp-ep-sdk-${kaaTag}.tar.gz"
            
            """
            }
        }

    }


    stage('run local env') {
        if (isPR()) {
            echo "skip run local env for PR builds"
            return
        }

        dir('jbt-backend') {
            sh "./gradlew clean build -x test -x checkstyleMain -x checkstyleTest -x helmInitClient -x helmFilterMainChartSources -x helmUpdateMainChartDependencies -x helmUpdateMainChartDependencies -x helmFilterSparkChartSources -x helmUpdateSparkChartDependencies -x helmPackageSparkChart -x helmPackageMainChart  --parallel"
        }

        dir('jbt-kaa-agent-builder') {
            withCredentials([usernamePassword(credentialsId: 'JBT_QA_E2E_CREDENTIALS_PROD', usernameVariable: 'JBT_QA_E2E_USER', passwordVariable: 'JBT_QA_E2E_PASS'),
                             usernamePassword(credentialsId: 'KAA_CREDS_STAGE', usernameVariable: 'JBT_QA_E2E_KAA_USERNAME', passwordVariable: 'JBT_QA_E2E_KAA_PASSWORD'),
                             usernamePassword(credentialsId: 'KAA_CREDS_STAGE', usernameVariable: 'KAA_USERNAME', passwordVariable: 'KAA_PASSWORD'),
                             string(credentialsId: '5b51337c-78c3-4677-9153-f9eca88ee8bc', variable: 'AWS_ACCESS_KEY_ID'),
                             string(credentialsId: 'd27d9f8f-018d-4ed0-ac7b-749e21721e64', variable: 'AWS_SECRET_ACCESS_KEY'),
                             string(credentialsId: '5a2efc62-9fbc-4096-9bd0-719d30cd7f2b', variable: 'AWS_DEFAULT_REGION'),
                             string(credentialsId: '5a2efc62-9fbc-4096-9bd0-719d30cd7f2b', variable: 'AWS_REGION'),
                             string(credentialsId: 'ARTIFACTORY_PASS', variable: 'ARTIFACTORY_PASS'),
                             string(credentialsId: 'JBT_QA_E2E_KAA_PASSWORD', variable: 'KAA_PASSWORD'),

            ]) {
                try {
                    sh "export KAA_TAG=${kaaTag}; export COMPOSE_PROJECT=${kaaCommit}; ./run_local.sh"
                } catch (e) {
                    echo "FAILED: $e"
                    saveLogs("${kaaCommit}")
                    sh "export JBT_BACKEND_DIR=`cd ../jbt-backend;pwd`; docker-compose --project-name ${kaaCommit} down -t 1 || true"
                    throw e
                }
            }
        }
    }

    stage('e2e vs local env') {
        if (isPR()) {
            echo "skip e2e check for PR builds"
            return
        }
        try {
            def kaaAgentTag = parseKaaAgentTag()
            dir('jbt-qa-e2e') {

                withCredentials([usernamePassword(credentialsId: 'JBT_QA_E2E_CREDENTIALS_PROD', usernameVariable: 'JBT_QA_E2E_USER', passwordVariable: 'JBT_QA_E2E_PASS'),
                                 string(credentialsId: '5b51337c-78c3-4677-9153-f9eca88ee8bc', variable: 'AWS_ACCESS_KEY_ID'),
                                 string(credentialsId: 'd27d9f8f-018d-4ed0-ac7b-749e21721e64', variable: 'AWS_SECRET_ACCESS_KEY'),
                                 string(credentialsId: '5a2efc62-9fbc-4096-9bd0-719d30cd7f2b', variable: 'AWS_DEFAULT_REGION'),
                                 string(credentialsId: '5a2efc62-9fbc-4096-9bd0-719d30cd7f2b', variable: 'AWS_REGION'),
                                 string(credentialsId: 'ARTIFACTORY_PASS', variable: 'ARTIFACTORY_PASS'),
                                 string(credentialsId: 'JBT_QA_E2E_KAA_PASSWORD', variable: 'JBT_QA_E2E_KAA_PASSWORD'),

                ]) {
                    timeout(30) {
                        sh """#!/bin/bash
          
                        export JBT_QA_E2E_APPLICATION_URL='http://localhost:8084'
                        export JBT_QA_E2E_KAA_HOST='localhost'
                        export JBT_QA_E2E_KAA_PORT='7777'
                        export JBT_QA_E2E_CASSANDRA_HOST='localhost'
                        export JBT_QA_E2E_BOOTSTRAP_SERVERS='localhost:9092'
                        export JBT_QA_E2E_AGENT_IMAGE_TAG='${kaaAgentTag}'
                        export JBT_QA_E2E_S3_REPORT_BUCKET='jbt-qa-it-tag-images'
                        export JBT_QA_E2E_S3_REPORT_PREFIX='reports'
                        export JBT_QA_E2E_S3_UPLOADER_BUCKET='jbt-qa-it-tag-images'
                        export BT_QA_E2E_S3_UPLOADER_PREFIX='binary'
                        export JBT_QA_E2E_ELASTIC_PROTOCOL='http'
                        export JBT_QA_E2E_ELASTIC_HOST='localhost'
                        export JBT_QA_E2E_ELASTIC_PORT='9200'

                        ./gradlew clean test publish -PtestngSuiteXml='src/test/resources/testng-e2e.xml' -PartifactoryUsername='admin' -PartifactoryPassword='${ARTIFACTORY_PASS}' --info                    
                    """

                    }
                }
            }

        } catch (e) {
            echo "FAILED: $e"
            throw e
        } finally {
            dir('jbt-qa-e2e') {
                echo 'Publish unit test results'
                junit allowEmptyResults: true, testResults: 'build/test-results/test/TEST-*.xml'

                sh "./gradlew allureReport || true"

                allure([
                        commandline      : 'allure270pony',
                        includeProperties: false,
                        jdk              : 'jdk8u172',
                        reportBuildPolicy: 'ALWAYS',
                        results          : [[path: 'build/reports/allure-results']]
                ])
            }

            dir('jbt-kaa-agent-builder') {
                withCredentials([usernamePassword(credentialsId: 'JBT_QA_E2E_CREDENTIALS', usernameVariable: 'JBT_QA_E2E_USER', passwordVariable: 'JBT_QA_E2E_PASS'),
                                 usernamePassword(credentialsId: 'KAA_CREDS_STAGE', usernameVariable: 'JBT_QA_E2E_KAA_USERNAME', passwordVariable: 'JBT_QA_E2E_KAA_PASSWORD'),
                                 usernamePassword(credentialsId: 'KAA_CREDS_STAGE', usernameVariable: 'KAA_USERNAME', passwordVariable: 'KAA_PASSWORD'),

                ]) {
                    saveLogs("${kaaCommit}")
                    sh "export JBT_BACKEND_DIR=`cd ../jbt-backend;pwd`; docker-compose --project-name ${kaaCommit} down -t 1 || true"
                }
            }
        }
    }

    stage('aptly') {
        if (!isPR()) {
            dir('kaa') {
                sh "curl -F 'file=@./server/node/target/kaa-node.deb;filename=kaa-node_${env.VERSION}_amd64.deb' ${env.APTLY_URL}/api/files/jbt"
                sh "curl -X POST ${env.APTLY_URL}/api/repos/jbt/file/jbt?forceReplace=1"
                sh "curl -X PUT -H 'Content-Type: application/json' --data '{\"ForceOverwrite\": true, \"Signing\": {\"GpgKey\": \"Nborisenko <nborisenko@kaaiot.io>\"}}' ${env.APTLY_URL}/api/publish/:./xenial"
            }
        }
    }
    stage('upload helm charts') {
        dir ('kaa') {
            withCredentials([usernamePassword(credentialsId: 'chartmuseum', usernameVariable: 'CHARTMUSEUM_USER', passwordVariable: 'CHARTMUSEUM_PASS')]) {
                sh """
                    ./gradlew helmPackage
                    ./gradlew helmPublish \\
                      -PhelmChartmuseumUrl=${env.CHARTMUSEUM_URL} \\
                      -PhelmChartmuseumUser=${CHARTMUSEUM_USER} \\
                      -PhelmChartmuseumPassword=${CHARTMUSEUM_PASS}
                """
            }
        }

    }

    stage ('change parent chart requirements') {
        dir ('kaa') {
            if (env.BRANCH_NAME == '0.9.0-patched') {
                LIB.build.triggerBuild("jbt-iot/jbt-metachart/master", [
                  COMPONENT: 'kaa',
                  COMPONENT_VERSION: LIB.version.getCurrentVersion()
                ])
            }
        }
    }

    stage('deploy to stage') {
        dir ('kaa') {
            if (env.BRANCH_NAME == '0.9.0-patched') {
                LIB.build.triggerBuild("jbt-iot/jbt-environment/master", [
                  CLUSTER_NAME: 'stage',
                  ENVIRONMENT_NAME: 'stage',
                  SECRETS_FROM: 'stage',
                  ACTION: 'create',
                  REMOVE_ON_FAILURE: 'false',
                  HELM_CHART_ONLY: 'true'
                ])
            }
        }
    }
}//node

def parseKaaAgentTag() {
    return sh(
            script: "cat jbt-kaa-agent-builder/kaa-agent.tag | awk -F= '{print \$2}'",
            returnStdout: true
    ).trim()
}

def saveLogs(String project) {

    fetchDockerLog("${project}_action-server_1")
    fetchDockerLog("${project}_cassandra-kaa_1")
    fetchDockerLog("${project}_cassandra_1")
    fetchDockerLog("${project}_code-regeneration-service_1")
    fetchDockerLog("${project}_jbt-kaa-appender-cfg_1")
    fetchDockerLog("${project}_kaa_1")
    fetchDockerLog("${project}_kaa-binary-data-loader_1")
    fetchDockerLog("${project}_kafka_1")
    fetchDockerLog("${project}_postgres_1")
    fetchDockerLog("${project}_redis_1")
    fetchDockerLog("${project}_spark-master_1")
    fetchDockerLog("${project}_spark-worker_1")
    fetchDockerLog("${project}_ui_1")
    fetchDockerLog("${project}_web-app_1")
    fetchDockerLog("${project}_zoo_1")
    fetchSparkLogs(project)
    archiveArtifacts allowEmptyArchive: true, artifacts: '**/*.log.gz'
}

def fetchDockerLog(String container) {
    sh "docker logs '${container}' 2>&1 | gzip -vc > '${container}.log.gz'"
}

def fetchSparkLogs(String project, String filter = " ") {
    sh """docker exec ${project}_spark-worker_1 bash -c 'find /spark/work -name stderr | grep driver | xargs grep -e "$filter"' | gzip -vc > ${project}_spark_worker.driver.log.gz"""
    sh """docker exec ${project}_spark-worker_1 bash -c 'find /spark/work -name stderr | grep app    | xargs grep -e "$filter"' | gzip -vc > ${project}_spark_worker.app.log.gz"""
}


def assureDockerCompose() {
    try {
        sh "which docker-compose"
    } catch (e) {
        echo "$e"
        sh "echo 'dce9897a5359f29284224295c0d179e1 ./docker-compose' > ./docker-compose.md5"
        sh "md5sum -c ./docker-compose.md5 || curl -L https://github.com/docker/compose/releases/download/1.21.0/docker-compose-`uname -s`-`uname -m` -o ./docker-compose"
        sh "chmod +x ./docker-compose"
        env.PATH = "${env.WORKSPACE}:${env.PWD}:${env.PATH}"
    }
    sh "which docker-compose"
}


def assureAws() {
    try {
        sh "which aws"
    } catch (e) {
        echo "$e"
        env.PATH = "${env.HOME}/.local/bin:${env.PATH}"
    }
    sh "which aws"
}

def assureJava() {
    try {
        sh "which java"
    } catch (e) {
        echo "$e"
        env.JAVA_HOME = "${tool name: 'jdk8u172', type: 'jdk'}"
        env.PATH = "${env.JAVA_HOME}/bin:${env.PATH}"
    }
    sh "which java"
}


def assureMaven() {
    try {
        sh "which mvn"
    } catch (e) {
        echo "$e"
        sh "wget http://apache.ip-connect.vn.ua/maven/maven-3/3.6.1/binaries/apache-maven-3.6.1-bin.tar.gz"
        sh "tar -xvf apache-maven-3.6.1-bin.tar.gz"
        env.PATH = "${env.WORKSPACE}/apache-maven-3.6.1/bin:${env.PATH}"
    }
    sh "which mvn"

}

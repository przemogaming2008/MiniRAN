pipeline {
    agent { label 'bash' }

    options {
        timestamps()
        buildDiscarder(logRotator(numToKeepStr: '20', artifactNumToKeepStr: '20'))
        timeout(time: 45, unit: 'MINUTES')
        skipStagesAfterUnstable()
    }

    environment {
        BUILD_DIR = 'build/ci'
        CI_OUT = 'ci_out'
        CI_LOG_DIR = 'ci_out/logs'
        CI_REPORT_DIR = 'ci_out/reports'
        CI_ARTIFACT_DIR = 'ci_out/artifacts'
    }

    stages {

        stage('00 Clean workspace outputs') {
            steps {
                sh '''
                    rm -rf build/ci ci_out
                    mkdir -p ci_out/logs ci_out/reports ci_out/artifacts
                '''
            }
        }

        stage('01 Preflight - środowisko') {
            steps {
                sh 'bash ci/scripts/ci_env_report.sh'
            }
        }

        stage('02 Configure + Build') {
            steps {
                sh 'bash ci/scripts/ci_build.sh'
            }
        }

        stage('03 Unit tests') {
            steps {
                sh 'bash ci/scripts/ci_test_unit.sh'
            }
        }

        stage('04 Component tests') {
            steps {
                sh 'bash ci/scripts/ci_test_component.sh'
            }
        }

        stage('05 CLI scenarios') {
            steps {
                sh 'bash ci/scripts/ci_run_cli_scenarios.sh'
            }
        }

        stage('06 Mega CI Gate') {
            steps {
                sh 'bash ci/scripts/ci_mega_gate.sh'
            }
        }
    }

    post {
        always {
            sh 'bash ci/scripts/ci_collect_logs.sh'
            junit testResults: 'ci_out/reports/**/*.xml', allowEmptyResults: false
            archiveArtifacts artifacts: 'ci_out/**/*', fingerprint: true, allowEmptyArchive: true
        }
        success {
            echo 'MiniRAN CI: wszystko przeszło poprawnie.'
        }
        unstable {
            echo 'MiniRAN CI: testy zwróciły problemy, sprawdź raport JUnit i logi.'
        }
        failure {
            echo 'MiniRAN CI: pipeline zatrzymał się na błędzie. Sprawdź pierwszy czerwony stage.'
        }
    }
}

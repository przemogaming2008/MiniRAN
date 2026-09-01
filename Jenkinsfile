pipeline {
    agent any

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
        CI_EXPECT_TEST_REPORTS = 'false'
        MINIRAN_STATIC_ANALYSIS_STRICT = '0'
        MINIRAN_COVERAGE_STRICT = '0'
        MINIRAN_COVERAGE_MIN_LINE_PERCENT = '0'
    }

    stages {

        stage('00 Clean workspace outputs') {
            steps {
                sh '''
                    rm -rf build/ci build/coverage ci_out
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
                script {
                    env.CI_EXPECT_TEST_REPORTS = 'true'
                }
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

        stage('07 Sanitizers') {
            steps {
                sh 'bash ci/scripts/ci_test_sanitizers.sh'
            }
        }

        stage('08 Static analysis') {
            steps {
                sh 'bash ci/scripts/ci_static_analysis.sh'
            }
        }

        stage('09 Coverage') {
            steps {
                sh 'bash ci/scripts/ci_test_coverage.sh'
            }
        }
    }

    post {
        always {
            script {
                def collectStatus = sh(
                    script: 'bash ci/scripts/ci_collect_logs.sh',
                    returnStatus: true
                )

                if (collectStatus != 0) {
                    echo "WARNING: ci_collect_logs.sh failed with exit code ${collectStatus}; continuing with JUnit and artifact publishing."
                }
            }

            script {
                catchError(buildResult: 'UNSTABLE', stageResult: 'UNSTABLE') {
                    archiveArtifacts artifacts: 'ci_out/**/*',
                        fingerprint: true,
                        allowEmptyArchive: true
                }
            }

            script {
                def hasJUnitReports = sh(
                    script: 'test -n "$(find ci_out/reports -name "*.xml" -print -quit 2>/dev/null)"',
                    returnStatus: true
                ) == 0

                if (hasJUnitReports) {
                    catchError(buildResult: 'FAILURE', stageResult: 'FAILURE') {
                        junit testResults: 'ci_out/reports/**/*.xml', allowEmptyResults: false
                    }
                } else if (env.CI_EXPECT_TEST_REPORTS == 'true') {
                    error 'Test stages started, but no JUnit reports were produced.'
                } else {
                    echo 'No JUnit reports found. Test stages did not start, so preserving the original Preflight/Build failure.'
                }
            }
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

pipeline {
    agent any

    stages {
        stage('Clean') {
            steps {
                bat 'if exist build rmdir /s /q build'
            }
        }

        stage('Configure') {
            steps {
                bat 'cmake -S . -B build'
            }
        }

        stage('Build') {
            steps {
                bat 'cmake --build build'
            }
        }

        stage('Test') {
            steps {
                bat 'ctest --test-dir build --output-on-failure'
            }
        }

        stage('Run scenarios') {
            steps {
                bat 'build\\miniran_cli.exe scenarios\\tcp_basic.cfg'
                bat 'build\\miniran_cli.exe scenarios\\udp_lossy.cfg'
                bat 'build\\miniran_cli.exe scenarios\\tcp_multiUe.cfg'
                bat 'build\\miniran_cli.exe scenarios\\udp_multiUe.cfg'
                bat 'build\\miniran_cli.exe scenarios\\tcp_downlink_multiUe.cfg'
            }
        }
    }
}
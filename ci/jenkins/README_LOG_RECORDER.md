# Jenkins Log Recorder Notes

This file explains where to look when Jenkins itself has a problem.

MiniRAN build logs and Jenkins system logs are different things.

## 1. MiniRAN build logs

Project logs are created by CI scripts:

    ci_out/logs/
    ci_out/reports/
    ci_out/artifacts/summary.md

Use these first when a build stage starts and then fails.

## 2. Jenkins system logs

Use Jenkins system logs when the job does not reach project scripts or when Jenkins features fail.

Examples:

- job waits in queue
- agent is offline
- checkout fails before scripts run
- credentials are missing
- JUnit publishing fails
- artifact archiving fails
- plugin error appears
- Jenkins executor disappears

## 3. Log Recorder

In Jenkins UI:

1. Open `Manage Jenkins`
2. Open `System Log`
3. Add a new log recorder
4. Name it for example `MiniRAN-CI`
5. Add useful Jenkins loggers
6. Re-run the failing job
7. Check the recorder output

Useful logger areas:

    hudson.model
    hudson.FilePath
    hudson.Launcher
    hudson.tasks.junit
    hudson.plugins.git
    jenkins.scm
    org.jenkinsci.plugins.workflow
    org.jenkinsci.plugins.workflow.job
    org.jenkinsci.plugins.workflow.steps

Use INFO level first. Use finer levels only for short debugging sessions.

## 4. What to check

### Job waits in queue

Check:

- is any agent online?
- does the agent have free executors?
- is the job blocked by label restrictions?
- is Jenkins allowed to run on that node?

MiniRAN Jenkinsfile uses:

    agent any

So there should be no required `bash` label.

### Checkout fails

Check:

- repository URL
- branch name
- credentials
- network access from Jenkins agent
- Git installation on agent

### Preflight does not start

Check:

- agent allocation
- workspace creation
- shell availability
- Jenkins node status

### JUnit publish fails

Check:

- are XML files present in `ci_out/reports/`?
- is XML valid?
- did the test stage start?
- did artifact archiving still save raw XML?

### Artifact archiving fails

Check:

- workspace path
- file permissions
- disk space
- Jenkins controller/agent connection

## 5. Linux service logs

If Jenkins runs as a Linux service, system logs may help.

Common command:

    journalctl -u jenkins --since "1 hour ago"

Use this only for Jenkins service problems, not normal test failures.

## 6. Safety

Do not commit Jenkins secrets.

Do not paste credentials into logs.

Do not archive private tokens.

If a log contains secrets, rotate the secret before sharing the log.

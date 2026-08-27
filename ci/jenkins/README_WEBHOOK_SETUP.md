
# MiniRAN Jenkins Webhook Setup

MiniRAN CI is intended to run automatically after repository changes.

Manual `Build Now` is useful for testing, but the normal CI path should be:

1. developer pushes commit
2. repository provider sends webhook
3. Jenkins receives webhook
4. Jenkins checks out the branch
5. Jenkins runs the pipeline from `Jenkinsfile`

## Jenkins job requirement

The Jenkins job should be connected to this repository and branch.

The job must use the repository `Jenkinsfile`.

The Jenkinsfile uses:

    agent any

The agent does not need a special label, but the selected agent must pass:

    bash ci/scripts/ci_env_report.sh

## Recommended trigger

Use a repository webhook that notifies Jenkins on push events.

The webhook should trigger the Jenkins job automatically after a push to the configured branch.

## Verification

After configuring the webhook:

1. make a small commit
2. push it to the configured branch
3. check that Jenkins starts without pressing `Build Now`
4. wait for the pipeline to finish
5. verify that all normal stages ran:

   01 Preflight
   02 Configure + Build
   03 Unit tests
   04 Component tests
   05 CLI scenarios
   06 Mega CI Gate
   07 Sanitizers
6. verify that post actions ran:

   log collection
   artifact archiving
   JUnit publishing

## If Jenkins does not start

Check:

- repository webhook delivery history
- webhook target URL
- Jenkins job trigger settings
- Jenkins credentials
- repository branch configured in the job
- Jenkins system log
- whether the Jenkins server is reachable from the repository provider

## If Jenkins starts but checkout fails

Check:

- repository URL
- credentials
- branch name
- Jenkins checkout log

## If Jenkins starts but pipeline fails

Start with:

    ci_out/artifacts/summary.md

Then open the first failing stage log under:

    ci_out/logs/

## Completion criteria

Webhook setup is complete when a normal push starts the Jenkins pipeline automatically and the build reaches the post-build publishing steps.

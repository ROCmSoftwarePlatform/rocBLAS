#!/usr/bin/env groovy
// This shared library is available at https://github.com/ROCmSoftwarePlatform/rocJENKINS/
@Library('rocJenkins@perftest') _

// This is file for internal AMD use.
// If you are interested in running your own Jenkins, please raise a github issue for assistance.

import com.amd.project.*
import com.amd.docker.*
import java.nio.file.Path


rocBLASCI:
{

    def rocblas = new rocProject('rocBLAS', 'PerformanceTest')
    // customize for project
    rocblas.paths.build_command = './install.sh -lasm_ci -c'

    // Define test architectures, optional rocm version argument is available
    def nodes = new dockerNodes(['ubuntu && gfx803'], rocblas)

    boolean formatCheck = true

    def compileCommand =
    {
        platform, project->

        def command = """#!/usr/bin/env bash
                        set -x
                        pwd
                        cd ${project.paths.project_build_prefix}
                        workingdir=`pwd`

                        pushd scripts/performance/blas/
                        
                        shopt expand_aliases
                        shopt -s expand_aliases
                        shopt expand_aliases
                        
                        python -V
                        alias python=python3
                        python -V

                        python rocmsmitest.py -d ${env.EXECUTOR_NUMBER}
                        
                        popd
                    """
        platform.runCommand(this, command)

        // commonGroovy = load "${project.paths.project_src_prefix}/.jenkins/Common.groovy"
        // commonGroovy.runCompileCommand(platform, project)
    }

    def testCommand =
    {
        platform, project->
        echo "TEST STAGE"
        String sudo = auxiliary.sudo(platform.jenkinsLabel)    
        def command = """#!/usr/bin/env bash
                        set -x
                        pwd
                        cd ${project.paths.project_build_prefix}
                        workingdir=`pwd`

                        mkdir perfoutput
                        pushd scripts/performance/blas/
                        
                        shopt expand_aliases
                        shopt -s expand_aliases
                        shopt expand_aliases
                        
                        python -V
                        alias python=python3
                        python -V

                        python alltime.py -A \$workingdir/build/release/clients/staging -o \$workingdir/perfoutput -i perf.yaml -S 0 -g 0 -d ${env.EXECUTOR_NUMBER}
                        
                        popd
                        ls perfoutput
                        tar -cvf perfoutput.tar
                        
                        wget http://10.216.151.18:8080/job/Performance/job/${project.name}/job/develop/lastSuccessfulBuild/artifact/*zip*/archive.zip
                        wgetreturn=\$?
                        if [[ \$wgetreturn -eq 8 ]]; then
                            echo "Download error"
                        fi

                        if [[ -z "${env.CHANGE_ID}" ]]
                        then
                            echo "This is not a pull request"
                        else
                            echo "This is a pull request"
                        fi
                    """
        platform.runCommand(this, command)
    }

    def packageCommand =
    {
        platform, project->

        platform.archiveArtifacts(this, """${project.paths.project_build_prefix}/perfoutput.tar""")
    }

    buildProject(rocblas, formatCheck, nodes.dockerArray, compileCommand, null, null)

}

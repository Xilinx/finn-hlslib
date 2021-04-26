/******************************************************************************
 *  Copyright (c) 2019, Xilinx, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2.  Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  3.  Neither the name of the copyright holder nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION). HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

node {
    def app

    stage('Clone repository') {
        /* Let's make sure we have the repository cloned to our workspace */
        checkout scm
    }

    parallel firstBranch: {
        stage('Run tests SWG') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_swg.tcl')
        }
    }, secondBranch: {
        stage('Run tests POOL') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_pool.tcl')
        }
    }, thirdBranch: {
        stage('Run tests DWC') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_dwc.tcl')
        }
    }, fourthBranch: {
        stage('Run tests ADD') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_add.tcl')
        }
    }, fifthBranch: {
        stage('Run tests DUP_STREAM') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_dup_stream.tcl')
        }
    }, sixthBranch: {
        stage('Set-up virtual env') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('virtualenv venv; source venv/bin/activate;pip3.7 install -r requirements.txt')
        }
        stage('Generate weigths for conv test') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_weigths.py;')
        }
        stage('Generate weigths for depthwise separable conv test') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_weigths_dws.py;')
        }
		stage('Generate weigths for non square conv test') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_weigths_nonsquare.py;')
        }
		stage('Generate weigths for non square conv test') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_weigths_nonsquare_dws.py;')
        }
		stage('Generate variables for TMRC test') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_params_stmr.py tmrcheck;')
        }
		stage('Generate weigths for conv STMR test not injecting errors') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_params_stmr.py no_inj;')
        }
		stage('Generate weigths for conv STMR test injecting errors') {
            sh('source venv/bin/activate; cd tb; python3.7 gen_params_stmr.py inj;')
        }
        stage('Run tests CONV3') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_conv3.tcl')
        }
        stage('Run tests CONV3_STREAM') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_conv_stream.tcl')
        }
        stage('Run tests CONVMMV') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_convmmv.tcl')
        }
        stage('Run tests DWSCONV') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_conv_dws.tcl')
        }
		stage('Run tests NON_SQUARE_CONV') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_conv_nonsquare.tcl')
        }
		stage('Run tests NON_SQUARE_DWS_CONV') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_conv_nonsquare_dws.tcl')
        }
		stage('Run test TMRC') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_tmrc_stmr.tcl')
        }
		stage('Run tests CONV_STMR') {
            env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_conv3_stmr.tcl')
        }
    }, seventhBranch: {
        stage('Run tests DWCNM') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_dwcnm.tcl')
        }
    }, eigthBranch: {
        stage('Run tests SWG_KS') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_swg_kernelstride.tcl')
        }
    }, ninthBranch: {
        stage('Run tests QDMA') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_qdma_stream.tcl')
        }
    }, tenthBranch: {
        stage('Run tests Pool Kernel Stride') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_kernel_stride_pool.tcl')
        }
    }, eleventhBranch: {
        stage('Run tests LabelSelect Batch') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_label_select.tcl')
        }
    }, twelfthBranch: {
        stage('Run tests Dilated SWG') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_swg_dilated.tcl')
        }
    }, thirteenthBranch: {
        stage('Run tests MMV SWG Kernel Stride') {
              env.FINN_HLS_ROOT = "${env.WORKSPACE}"
            echo "${env.FINN_HLS_ROOT}"
            sh('source /proj/xbuilds/2020.1_released/installs/lin64/Vivado/2020.1/settings64.sh; cd tb; vivado_hls -f test_swg_kernelstride_mmv.tcl')
        }
    }
}

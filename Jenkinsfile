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

node('finn-build || built-in') {
    def app
    stage('Clone repository') {
        /* Let's make sure we have the repository cloned to our workspace */
        checkout scm
    }
    withEnv([
        'LC_ALL=C',
        'LANG=C',
        "FINN_HLS_ROOT=${env.WORKSPACE}",
        "HLS_ENV_SRC=/proj/xbuilds/2022.1_released/installs/lin64/Vitis/2022.1/settings64.sh",
        "HOME=${env.WORKSPACE}"
    ]){
        parallel firstBranch: {
            stage('SWG') {
                echo "HLS_ENV_SRC: ${env.HLS_ENV_SRC}"
                echo "FINN_HLS_ROOT: ${env.FINN_HLS_ROOT}"
                echo "LC_ALL: ${env.LC_ALL}"
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_swg.tcl")
            }
            stage('SWG Kernelstride') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_swg_kernelstride.tcl")
            }
            stage('SWG Kernelstride MMV') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_swg_kernelstride_mmv.tcl")
            }
            stage('SWG 1D') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_swg_1D.tcl")
            }
            stage('SWG 1D DWS') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_swg_1D_dws.tcl")
            }
        }, secondBranch: {
            stage('POOL') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_pool.tcl")
            }
            stage('POOL 1D') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_pool_1d.tcl")
            }
            stage('POOL Kernelstride') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_kernel_stride_pool.tcl")
            }
        }, thirdBranch: {
            stage('DWC') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_dwc.tcl")
            }
        }, fourthBranch: {
            stage('ADD') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_add.tcl")
            }
            stage('ELTWISE') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_eltwise.tcl")
            }
            stage('MAX_NORM') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_max_norm.tcl")
            }
        }, fifthBranch: {
            stage('DUP_STREAM') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_dup_stream.tcl")
            }
        }, sixthBranch: {
            stage('CONV3') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv3.tcl")
            }
            stage('CONV3_STREAM') {   
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv_stream.tcl")
            }
            stage('CONVMMV') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_convmmv.tcl")
            }
            stage('DWSCONV') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv_dws.tcl")
            }
            stage('NON_SQUARE_CONV') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv_nonsquare.tcl")
            }
            stage('NON_SQUARE_DWS_CONV') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv_nonsquare_dws.tcl")
            }
            stage('CONV3 INJ STMR') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv3_inj_stmr.tcl")
            }
            stage('CONV3 NOINJ STMR') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_conv3_noinj_stmr.tcl")
            }
        }, seventhBranch: {
            stage('DWCNM') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_dwcnm.tcl")
            }
        }, eighthBranch: {
            stage('QDMA') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_qdma_stream.tcl")
            }
        }, ninthBranch: {
            stage('LabelSelect Batch') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_label_select.tcl")
            }
        }, tenthBranch: {
            stage('UPSAMPLE') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_upsample.tcl")
            }
            stage('UPSAMPLE_1D') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_upsample_1d.tcl")
            }
        }, eleventhBranch: {
            stage('CHANNELWISE OP') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_channelwise_op.tcl")
            }
        }, twelfthBranch: {
            stage('TMR CHECK') {
                sh("source ${env.HLS_ENV_SRC}; cd tb; vitis_hls -f test_tmrc_stmr.tcl")
            }
        }
    }
}

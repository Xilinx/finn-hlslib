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
        checkout scm
    }
    withEnv([
        'LC_ALL=C',
        'LANG=C',
        "FINN_HLS_ROOT=${env.WORKSPACE}",
        "HLS_ENV_SRC=/proj/xbuilds/2022.1_released/installs/lin64/Vitis/2022.1/settings64.sh",
        "HOME=${env.WORKSPACE}"
    ]){
        def run = { tcl -> sh("source ${env.HLS_ENV_SRC}; cd tb; vitis-run --tcl ${tcl}") }
        parallel firstBranch: {
            stage('MVAU')               { run 'mvau.tcl' }
            stage('VVAU')               { run 'vvau.tcl' }
        }, secondBranch: {
            stage('POOL')               { run 'pool.tcl' }
            stage('MAXPOOL')            { run 'maxpool.tcl' }
        }, thirdBranch: {
            stage('DWC')                { run 'dwc.tcl' }
            stage('CONCAT')             { run 'concat.tcl' }
            stage('SPLIT')              { run 'split.tcl' }
            stage('DUP')                { run 'dup.tcl' }
        }, fourthBranch: {
            stage('ELTWISE')            { run 'eltwise.tcl' }
            stage('CHANNELWISE OP')     { run 'channelwise_op.tcl' }
            stage('SOFTMAX')            { run 'softmax.tcl' }
            stage('LAYERNORM')          { run 'layernorm.tcl' }
        }, fifthBranch: {
            stage('BIPOLAR')            { run 'bipolar.tcl' }
            stage('REQUANT')            { run 'requant.tcl' }
            stage('LABEL SELECT')       { run 'label_select.tcl' }
        }, sixthBranch: {
            stage('FM PIXEL PADDING')   { run 'fm_pixel_padding.tcl' }
            stage('DECONV')             { run 'deconv.tcl' }
            stage('DECONV 2D')          { run 'deconv2d.tcl' }
            stage('FLATTEN')            { run 'flatten.tcl' }
        }, seventhBranch: {
            stage('CHECKSUM')           { run 'checksum.tcl' }
            stage('QDMA')               { run 'qdma_stream.tcl' }
        }
    }
}

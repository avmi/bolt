R"(// Copyright (C) 2019. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "kernel_def.h"

__kernel void activation(const int h,
    const int w,
    const int ih_str,
    const int iw_str,
    const int ih_off,
    const int iw_off,
    const int oh_str,
    const int ow_str,
    const int oh_off,
    const int ow_off,
#if defined(USE_SOFTPLUS)
	const T beta,
	const T threshold,
#endif
    __global T *input,
    __global T *output)
{
	int idx = get_global_id(0);
    int idy = get_global_id(1);
    int idz = get_global_id(2);
    if (idx >= ((w + 3) >> 2) || idy >= h) {
        return;
    }
	char ew = ((idx << 2) + 4 <= iw_str) ? 4 : (iw_str & 3);
	LOAD_VAL_T4(ew, idx, idy, idz, ih_str, iw_str, ih_off, iw_off, input, val);
    ACTIVATION_V4(val);
    STORE_VAL_T4(ew, idx, idy, idz, oh_str, ow_str, oh_off, ow_off, output, val);
}

#if !defined(USE_HSIGMOID) && !defined(USE_HSWISH) && !defined(USE_ROUND)
__kernel void activation_backward(const int h,
    const int w,
    const int ih_str,
    const int iw_str,
    const int ih_off,
    const int iw_off,
    const int oh_str,
    const int ow_str,
    const int oh_off,
    const int ow_off,
#if defined(USE_SOFTPLUS)
	const T beta,
	const T threshold,
#endif
    __global T *data,
    __global T *delta,
    __global T *prevLayerDelta)
{
	int idx = get_global_id(0);
    int idy = get_global_id(1);
    int idz = get_global_id(2);
    if (idx >= ((w + 3) >> 2) || idy >= h) {
        return;
    }
	char ew = ((idx << 2) + 4 <= iw_str) ? 4 : (iw_str & 3);
    LOAD_VAL_T4(ew, idx, idy, idz, ih_str, iw_str, ih_off, iw_off, data, val);
    LOAD_VAL_T4(ew, idx, idy, idz, ih_str, iw_str, ih_off, iw_off, delta, del);
    LOAD_VAL_T4(ew, idx, idy, idz, ih_str, iw_str, ih_off, iw_off, prevLayerDelta, prev);
    ACTIVATION_BACKWARD_V4(val, del);
	prev += val;
    STORE_VAL_T4(ew, idx, idy, idz, oh_str, ow_str, oh_off, ow_off, prevLayerDelta, prev);
}
#endif

)"
// Copyright (C) 2019. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <math.h>

#include "cpu/x86/int8/tensor_computing_int8.h"

EE dequantizeI32ToF32(TensorDesc qDesc, I32 *qData, const F32 *scale, TensorDesc dDesc, F32 *data)
{
    U32 dataNum = tensorNumElements(dDesc);
    U32 num16 = dataNum / 16;
    U32 resMask = pow(2, dataNum % 16) - 1;
    F32 factor = 1 / *scale;
    __asm__ __volatile__("vbroadcastss (%2), %%zmm2              \n\t"
                         ".align 16                              \n\t"
                         "0:                                     \n\t"
                         "vmovups (%0), %%zmm0                   \n\t"
                         "vcvtdq2ps %%zmm0, %%zmm1               \n\t"
                         "vmulps %%zmm2, %%zmm1, %%zmm0          \n\t"
                         "vmovups %%zmm0, (%1)                   \n\t"
                         "add $0x40, %0                          \n\t"
                         "add $0x40, %1                          \n\t"
                         "dec %%ebx                              \n\t"
                         "jg 0b                                  \n\t"

                         "vxorps %%zmm0, %%zmm0, %%zmm0          \n\t"
                         "cmp $0x0, %%eax                       \n\t"
                         "je 1f                                  \n\t"
                         "kmovw %%eax, %%k1                      \n\t"
                         "vmovups (%0), %%zmm0 %{%%k1%}             \n\t"
                         "vcvtdq2ps %%zmm0, %%zmm1               \n\t"
                         "vmulps %%zmm2, %%zmm1, %%zmm0          \n\t"
                         "vmovups %%zmm0, (%1) %{%%k1%}         \n\t"
                         ".align 16                              \n\t"
                         "1:                                     \n\t"
                         : "+r"(qData), "+r"(data)
                         : "r"(&factor), "b"(num16), "a"(resMask)
                         : "%k1", "%zmm0", "%zmm1", "%zmm2", "memory", "cc");
    return SUCCESS;
}

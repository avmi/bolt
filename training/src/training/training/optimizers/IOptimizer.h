// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef IOPTIMIZER_H
#define IOPTIMIZER_H

#include <iostream>

#include <training/common/Common.h>
#include <training/common/MemoryManager.h>
#include <training/common/MemoryManagerGPU.h>
#include <training/opencl/OpenCLKernelManager.h>

namespace raul::optimizers
{
/**
 * @brief Optimizer interface
 */
struct IOptimizer
{
    virtual ~IOptimizer() = default;
    virtual void operator()(MemoryManager& memory_manager, Tensor& param, Tensor& grad) = 0;
    virtual void operator()(MemoryManagerGPU& memory_manager, TensorGPU& param, TensorGPU& grad) = 0;
    virtual void operator()(OpenCLKernelManager& kernel_manager, MemoryManagerGPU& memory_manager, TensorGPU& param, TensorGPU& grad) = 0;
    virtual void operator()(MemoryManagerFP16& memory_manager, TensorFP16& param, TensorFP16& grad) = 0;
    virtual void setLearningRate(dtype lr) = 0;
    [[nodiscard]] virtual dtype getLearningRate() = 0;
};

} // raul::optimizers

#endif // IOPTIMIZER_H
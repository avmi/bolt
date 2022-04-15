// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "ReLUActivationImpl.h"
#include "../ReLUActivation.h"

namespace raul
{

template<typename MM>
void ReLUActivationImpl<MM>::forwardComputeImpl(NetworkMode)
{
    Workflow& work = mLayer.mNetworkParams.mWorkflow;

    auto& output = work.getMemoryManager<MM>().getTensor(mLayer.mOutputName);
    const auto& input = work.getMemoryManager<MM>().getTensor(mLayer.mInputName);

    Common::ReLU(&work.getKernelManager(), mLayer.mTypeName + "[" + mLayer.mName + "::forwardComputeImpl]", work.getBatchSize(), mLayer.mDepth, mLayer.mHeight, mLayer.mWidth, input, output);
}

template<typename MM>
void ReLUActivationImpl<MM>::backwardComputeImpl()
{
    Workflow& work = mLayer.mNetworkParams.mWorkflow;

    const auto& output = work.getMemoryManager<MM>().getTensor(mLayer.mOutputName);
    const auto& delta = work.getMemoryManager<MM>().getTensor(mLayer.mOutputName.grad());
    auto& prevLayerDelta = work.getMemoryManager<MM>().getTensor(mLayer.mInputName.grad());

    Common::ReLUBackward(
        &work.getKernelManager(), mLayer.mTypeName + "[" + mLayer.mName + "::backwardComputeImpl]", work.getBatchSize(), mLayer.mDepth, mLayer.mHeight, mLayer.mWidth, output, delta, prevLayerDelta);
}

template<typename MM>
void ReLU6ActivationImpl<MM>::forwardComputeImpl(NetworkMode)
{
    Workflow& work = mLayer.mNetworkParams.mWorkflow;

    auto& output = work.getMemoryManager<MM>().getTensor(mLayer.mOutputName);
    const auto& input = work.getMemoryManager<MM>().getTensor(mLayer.mInputName);

    Common::ReLU6(&work.getKernelManager(), mLayer.mTypeName + "[" + mLayer.mName + "::forwardComputeImpl]", work.getBatchSize(), mLayer.mDepth, mLayer.mHeight, mLayer.mWidth, input, output);
}

template<typename MM>
void ReLU6ActivationImpl<MM>::backwardComputeImpl()
{
    Workflow& work = mLayer.mNetworkParams.mWorkflow;

    const auto& output = work.getMemoryManager<MM>().getTensor(mLayer.mOutputName);
    const auto& delta = work.getMemoryManager<MM>().getTensor(mLayer.mOutputName.grad());
    auto& prevLayerDelta = work.getMemoryManager<MM>().getTensor(mLayer.mInputName.grad());

    Common::ReLU6Backward(
        &work.getKernelManager(), mLayer.mTypeName + "[" + mLayer.mName + "::backwardComputeImpl]", work.getBatchSize(), mLayer.mDepth, mLayer.mHeight, mLayer.mWidth, output, delta, prevLayerDelta);
}

INSTANTIATE_IMPL(ReLUActivationImpl)
INSTANTIATE_IMPL(ReLU6ActivationImpl)

} // namespace raul

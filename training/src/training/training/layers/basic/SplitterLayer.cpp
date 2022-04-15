// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "SplitterLayer.h"

#include "impl/SplitterLayerCPU.h"
#include "impl/SplitterLayerGPU.h"

#include <algorithm>

#include <training/common/MemoryManager.h>

namespace raul
{

SplitterLayer::SplitterLayer(const Name& name, const BasicParams& params, NetworkParameters& networkParameters)
    : BasicLayer(name, "Splitter", params, networkParameters)
{
    auto prefix = mTypeName + "[" + mName + "::ctor]: ";

    if (mInputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of input names");
    }

    DECLARE_IMPL(SplitterLayer, SplitterLayerCPU<MemoryManager>, SplitterLayerGPU, SplitterLayerCPU<MemoryManagerFP16>)

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputs[0], raul::Workflow::Usage::Forward, raul::Workflow::Mode::Read);

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputs[0], mInputs[0].grad(), DEC_BACK_WRIT_ZERO);

    for (const auto& output : mOutputs)
    {
        mNetworkParams.mWorkflow.copyDeclaration(mName, mInputs[0], output, DEC_FORW_WRIT);

        mNetworkParams.mWorkflow.copyDeclaration(mName, output, output.grad(), DEC_BACK_READ);
    }

    mDepth = mNetworkParams.mWorkflow.getDepth(mInputs[0]);
    mHeight = mNetworkParams.mWorkflow.getHeight(mInputs[0]);
    mWidth = mNetworkParams.mWorkflow.getWidth(mInputs[0]);
}

} // namespace raul
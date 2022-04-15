// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "SoftMaxActivation.h"

#include "impl/SoftMaxActivationCPU.h"
#include "impl/SoftMaxActivationGPU.h"

#include <training/common/Common.h>
#include <training/common/MemoryManager.h>

namespace raul
{

SoftMaxActivation::SoftMaxActivation(const Name& name, const BasicParamsWithDim& params, NetworkParameters& networkParameters)
    : BasicLayer(name, "SoftMax", params, networkParameters)
    , mDimension(params.dim)
{
    MEASURE_BLOCK(mTypeName + "[" + mName + "::ctor]")

    mCaller = mTypeName + "[" + mName + "::ctor]: ";
    if (mInputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of input names");
    }
    if (mOutputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of output names");
    }

    if (mNetworkParams.mWorkflow.getExecutionTarget() == ExecutionTarget::GPU && mDimension != Dimension::Default && mDimension != Dimension::Width)
    {
        THROW(mTypeName, mName, "unsupported dimension in GPU mode provided");
    }

    DECLARE_IMPL(SoftMaxActivation, SoftMaxActivationCPU<MemoryManager>, SoftMaxActivationGPU, SoftMaxActivationCPU<MemoryManagerFP16>)

    mInputName = mInputs[0];
    mOutputName = mOutputs[0];

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, raul::Workflow::Usage::Forward, raul::Workflow::Mode::Read);

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mInputName.grad(), DEC_BACK_WRIT_ZERO);

    // reduce output tensor dimensions
    if (mDimension == Dimension::Default)
    {
        shape inputShape = shape{ 1u, mNetworkParams.mWorkflow.getDepth(mInputName), mNetworkParams.mWorkflow.getHeight(mInputName), mNetworkParams.mWorkflow.getWidth(mInputName) };

        mInputsCount = inputShape.total_size();

        mNetworkParams.mWorkflow.tensorNeeded(mName, mOutputName, raul::WShape{ raul::BS(), mInputsCount, 1u, 1u }, DEC_FORW_WRIT_NOMEMOPT);
        mNetworkParams.mWorkflow.copyDeclaration(mName, mOutputName, DEC_BACK_READ_NOMEMOPT);

        mNetworkParams.mWorkflow.copyDeclaration(mName, mOutputName, mOutputName.grad(), DEC_BACK_READ);
    }
    else
    {
        mNetworkParams.mWorkflow.copyDec(mName, mInputName, mOutputName, DEC_FORW_WRIT_NOMEMOPT);
        mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mOutputName, DEC_BACK_READ_NOMEMOPT);

        mNetworkParams.mWorkflow.copyDec(mName, mInputName, mOutputName.grad(), DEC_BACK_READ);
    }

    mDepth = mNetworkParams.mWorkflow.getDepth(mInputName);
    mHeight = mNetworkParams.mWorkflow.getHeight(mInputName);
    mWidth = mNetworkParams.mWorkflow.getWidth(mInputName);
}

} // namespace raul

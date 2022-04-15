// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "LogSoftMaxActivation.h"

#include <algorithm>

#include <training/common/MemoryManager.h>

namespace raul
{

LogSoftMaxActivation::LogSoftMaxActivation(const Name& name, const BasicParamsWithDim& params, NetworkParameters& networkParameters)
    : BasicLayer(name, "LogSoftMax", params, networkParameters)
    , mDimension(params.dim)
{

    auto prefix = "LogSoftMaxActivation[" + mName + "::ctor]: ";

    if (mDimension != Dimension::Default && mDimension != Dimension::Width)
    {
        THROW(mTypeName, mName, "specified dimension not implemented");
    }

    if (mInputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of input names");
    }
    if (mOutputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of output names");
    }

    if (mDimension == Dimension::Height || mDimension == Dimension::Depth)
    {
        THROW(mTypeName, mName, "unsupported dimension");
    }

    mInputName = mInputs[0];
    mOutputName = mOutputs[0];

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, raul::Workflow::Usage::Forward, raul::Workflow::Mode::Read);

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mInputName.grad(), DEC_BACK_WRIT_ZERO);

    // reduce output tensor dimensions
    if (mDimension == Dimension::Default)
    {
        shape inputShape = shape{ 1u, mNetworkParams.mWorkflow.getDepth(mInputName), mNetworkParams.mWorkflow.getHeight(mInputName), mNetworkParams.mWorkflow.getWidth(mInputName) };
        mInputsCount = inputShape.total_size();

        mNetworkParams.mWorkflow.tensorNeeded(mName, mOutputName, raul::WShape{ raul::BS(), 1u, 1u, mInputsCount }, DEC_FORW_WRIT_NOMEMOPT);
        mNetworkParams.mWorkflow.copyDeclaration(mName, mOutputName, DEC_BACK_READ_NOMEMOPT);

        mNetworkParams.mWorkflow.copyDeclaration(mName, mOutputName, mOutputName.grad(), DEC_BACK_READ);
    }
    else
    {
        mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mOutputName, DEC_FORW_WRIT_NOMEMOPT);
        mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mOutputName, DEC_BACK_READ_NOMEMOPT);

        mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mOutputName.grad(), DEC_BACK_READ);
    }
    mDepth = mNetworkParams.mWorkflow.getDepth(mInputName);
    mHeight = mNetworkParams.mWorkflow.getHeight(mInputName);
    mWidth = mNetworkParams.mWorkflow.getWidth(mInputName);
}

void LogSoftMaxActivation::initNotBSTensors()
{
    if (mNetworkParams.mWorkflow.getExecutionTarget() == ExecutionTarget::GPU)
    {
        if (!mNetworkParams.mWorkflow.getKernelManager().hasKernel(mTypeName / "forward"))
        {
            std::string source = "__kernel void logsoftmaxForward(const int x, "
                                 "    const int y, "
                                 "    const int z, "
                                 "    const int externalDimSize, "
                                 "    const int internalDimSize, "
                                 "    __global T *input, "
                                 "    __global T *output) "
                                 "{ "
                                 "    int idx = get_global_id(0); "
                                 "    int idy = get_global_id(1); "
                                 "    int idz = get_global_id(2); "
                                 "    if (x == idx && y == idy && z == idz) "
                                 "    { "
                                 "        for (int q = 0; q < externalDimSize; ++q) "
                                 "        { "
                                 "            T sum = 0.0f; "
                                 "            for (int i = 0; i < internalDimSize; ++i) "
                                 "            { "
                                 "                sum += exp(input[q * internalDimSize + i]); "
                                 "            } "
                                 "            sum = log(sum); "
                                 "            for (int i = 0; i < internalDimSize; ++i) "
                                 "            { "
                                 "                output[q * internalDimSize + i] = input[q * internalDimSize + i] - sum; "
                                 "            } "
                                 "        } "
                                 "    } "
                                 "} ";
            mNetworkParams.mWorkflow.getKernelManager().registerProgram(mTypeName / "forward", source);
        }
        if (!mNetworkParams.mWorkflow.getKernelManager().hasKernel(mTypeName / "backward"))
        {
            std::string source = "__kernel void logsoftmaxBackward(const int x, "
                                 "    const int y, "
                                 "    const int z, "
                                 "    const int externalDimSize, "
                                 "    const int internalDimSize, "
                                 "    __global T *output, "
                                 "    __global T *deltas, "
                                 "    __global T *prevLayerDelta) "
                                 "{ "
                                 "    int idx = get_global_id(0); "
                                 "    int idy = get_global_id(1); "
                                 "    int idz = get_global_id(2); "
                                 "    if (x == idx && y == idy && z == idz) "
                                 "    { "
                                 "        for (int q = 0; q < externalDimSize; ++q) "
                                 "        { "
                                 "            for (int i = 0; i < internalDimSize; ++i) "
                                 "            { "
                                 "                T sum = 0.0f; "
                                 "                const T e = exp(output[q * internalDimSize + i]);"
                                 "                for (int j = 0; j < internalDimSize; ++j) "
                                 "                { "
                                 "                    sum += deltas[q * internalDimSize + j] * e; "
                                 "                } "
                                 "                prevLayerDelta[q * internalDimSize + i] += deltas[q * internalDimSize + i] - sum;"
                                 "            } "
                                 "        } "
                                 "    } "
                                 "} ";
            mNetworkParams.mWorkflow.getKernelManager().registerProgram(mTypeName / "backward", source);
        }
    }
}

void LogSoftMaxActivation::forwardComputeImpl(NetworkMode)
{
    auto& work = mNetworkParams.mWorkflow;

    if (work.getExecutionTarget() == ExecutionTarget::CPU)
    {
        const size_t batchSize = mNetworkParams.mWorkflow.getBatchSize();

        Tensor& output = mNetworkParams.mMemoryManager[mOutputName];

        const Tensor& inputs = mNetworkParams.mMemoryManager[mInputName];

        if (mDimension == Dimension::Default)
        {
            for (size_t q = 0; q < batchSize; ++q)
            {
                size_t batchOffset = q * mInputsCount;

                dtype sum = 0.0_dt;
                dtype max = (*std::max_element(inputs.begin() + batchOffset, inputs.begin() + batchOffset + mInputsCount));

                for (size_t i = 0; i < mInputsCount; ++i)
                {
                    sum += std::exp(inputs[batchOffset + i] - max);
                }
                sum = static_cast<dtype>(log(sum) + max);
                for (size_t i = 0; i < mInputsCount; ++i)
                {
                    output[batchOffset + i] = inputs[batchOffset + i] - sum;
                }
            }
        }
        else if (mDimension == Dimension::Width)
        {
            size_t size = batchSize * inputs.getDepth() * inputs.getHeight();
            auto input2D = inputs.reshape(yato::dims(size, inputs.getWidth()));
            auto output2D = output.reshape(yato::dims(size, inputs.getWidth()));

            for (size_t q = 0; q < size; ++q)
            {
                dtype sum = 0.0_dt;
                dtype max = (*std::max_element(inputs.begin() + q * inputs.getWidth(), inputs.begin() + (q + 1) * inputs.getWidth()));

                for (size_t i = 0; i < inputs.getWidth(); ++i)
                    sum += std::exp(input2D[q][i] - max);
                sum = static_cast<dtype>(log(sum) + max);
                for (size_t i = 0; i < inputs.getWidth(); ++i)
                    output2D[q][i] = input2D[q][i] - sum;
            }
        }
    }
    else if (work.getExecutionTarget() == ExecutionTarget::GPU)
    {
        raul::TensorGPU& output = mNetworkParams.mMemoryManagerGPU(mOutputs[0]);
        const raul::TensorGPU& input = mNetworkParams.mMemoryManagerGPU(mInputs[0]);
        auto logsoftmaxForwardKernel = work.getKernelManager().getKernel(mTypeName / "forward", "logsoftmaxForward", mTypeName + "[" + mName + "::forwardComputeImpl]");
        size_t externalDimSize = work.getBatchSize();
        size_t internalDimSize = mWidth;
        if (mDimension == Dimension::Width)
        {
            externalDimSize *= mDepth * mHeight;
        }
        else
        {
            internalDimSize *= mDepth * mHeight;
        }

        work.getKernelManager().callKernel(logsoftmaxForwardKernel,
                                           cl::NDRange{ mHeight, mWidth, work.getBatchSize() * mDepth },
                                           mTypeName + "[" + mName + "::forwardComputeImpl]",
                                           0,
                                           0,
                                           0,
                                           (cl_int)externalDimSize,
                                           (cl_int)internalDimSize,
                                           input.getBuffer(),
                                           output.getBuffer());
    }
    else if (work.getExecutionTarget() == ExecutionTarget::CPUFP16)
    {
        const size_t batchSize = mNetworkParams.mWorkflow.getBatchSize();

        auto& output = work.getMemoryManager<MemoryManagerFP16>()[mOutputName];

        const auto& inputs = work.getMemoryManager<MemoryManagerFP16>()[mInputName];

        if (mDimension == Dimension::Default)
        {
            for (size_t q = 0; q < batchSize; ++q)
            {
                size_t batchOffset = q * mInputsCount;

                dtype sum = 0.0_dt;
                half max = (*std::max_element(inputs.begin() + batchOffset, inputs.begin() + batchOffset + mInputsCount));

                for (size_t i = 0; i < mInputsCount; ++i)
                {
                    sum += std::exp(TODTYPE(inputs[batchOffset + i] - max));
                }
                sum = static_cast<dtype>(log(sum) + TODTYPE(max));
                for (size_t i = 0; i < mInputsCount; ++i)
                {
                    output[batchOffset + i] = inputs[batchOffset + i] - TOHTYPE(sum);
                }
            }
        }
        else if (mDimension == Dimension::Width)
        {
            size_t size = batchSize * inputs.getDepth() * inputs.getHeight();
            auto input2D = inputs.reshape(yato::dims(size, inputs.getWidth()));
            auto output2D = output.reshape(yato::dims(size, inputs.getWidth()));

            for (size_t q = 0; q < size; ++q)
            {
                dtype sum = 0.0_dt;
                half max = (*std::max_element(inputs.begin() + q * inputs.getWidth(), inputs.begin() + (q + 1) * inputs.getWidth()));

                for (size_t i = 0; i < inputs.getWidth(); ++i)
                    sum += std::exp(TODTYPE(input2D[q][i] - max));
                sum = static_cast<dtype>(log(sum) + TODTYPE(max));
                for (size_t i = 0; i < inputs.getWidth(); ++i)
                    output2D[q][i] = input2D[q][i] - TOHTYPE(sum);
            }
        }
    }
    else
    {
        THROW_NONAME("LogSoftMaxActivation", "unsupported execution target");
    }
}

void LogSoftMaxActivation::backwardComputeImpl()
{
    auto& work = mNetworkParams.mWorkflow;

    if (work.getExecutionTarget() == ExecutionTarget::CPU)
    {
        const Tensor& output = mNetworkParams.mMemoryManager[mOutputName];

        ////if (mNetworkParams.isGradNeeded(mInputName))
        {
            const Tensor& deltas = mNetworkParams.mMemoryManager[mOutputName.grad()];
            Tensor& prevLayerDelta = mNetworkParams.mMemoryManager[mInputName.grad()];

            const size_t batchSize = prevLayerDelta.getBatchSize();

            if (mDimension == Dimension::Default)
            {
                for (size_t q = 0; q < batchSize; ++q)
                {
                    size_t batchOffset = q * mInputsCount;

                    for (size_t i = 0; i < mInputsCount; ++i)
                    {
                        dtype sum = 0.0_dt;
                        dtype e = static_cast<dtype>(exp(output[batchOffset + i]));
                        for (size_t j = 0; j < mInputsCount; ++j)
                        {
                            sum += deltas[batchOffset + j] * e;
                        }
                        prevLayerDelta[batchOffset + i] += deltas[batchOffset + i] - sum;
                    }
                }
            }
            else if (mDimension == Dimension::Width)
            {
                size_t size = batchSize * deltas.getDepth() * deltas.getHeight();

                auto deltas2D = deltas.reshape(yato::dims(size, deltas.getWidth()));
                auto prevLayerDelta2D = prevLayerDelta.reshape(yato::dims(size, deltas.getWidth()));
                auto output2D = output.reshape(yato::dims(size, deltas.getWidth()));

                for (size_t q = 0; q < size; ++q)
                {
                    for (size_t i = 0; i < deltas.getWidth(); ++i)
                    {
                        dtype sum = 0.0_dt;
                        dtype e = static_cast<dtype>(exp(output2D[q][i]));
                        for (size_t j = 0; j < deltas.getWidth(); ++j)
                        {
                            sum += deltas2D[q][j] * e;
                        }
                        prevLayerDelta2D[q][i] += deltas2D[q][i] - sum;
                    }
                }
            }
        }
    }
    else if (work.getExecutionTarget() == ExecutionTarget::GPU)
    {
        ////if (mNetworkParams.isGradNeeded(mInputName))
        {
            const raul::TensorGPU& output = mNetworkParams.mMemoryManagerGPU(mOutputs[0]);
            const raul::TensorGPU& deltas = mNetworkParams.mMemoryManagerGPU(mOutputs[0].grad());
            raul::TensorGPU& prevLayerDelta = mNetworkParams.mMemoryManagerGPU(mInputs[0].grad());
            auto logsoftmaxBackwardKernel = work.getKernelManager().getKernel(mTypeName / "backward", "logsoftmaxBackward", mTypeName + "[" + mName + "::backwardComputeImpl]");
            size_t externalDimSize = work.getBatchSize();
            size_t internalDimSize = mWidth;
            if (mDimension == Dimension::Width)
            {
                externalDimSize *= mDepth * mHeight;
            }
            else
            {
                internalDimSize *= mDepth * mHeight;
            }

            work.getKernelManager().callKernel(logsoftmaxBackwardKernel,
                                               cl::NDRange{ mHeight, mWidth, work.getBatchSize() * mDepth },
                                               mTypeName + "[" + mName + "::backwardComputeImpl]",
                                               0,
                                               0,
                                               0,
                                               (cl_int)externalDimSize,
                                               (cl_int)internalDimSize,
                                               output.getBuffer(),
                                               deltas.getBuffer(),
                                               prevLayerDelta.getBuffer());
        }
    }
    else if (work.getExecutionTarget() == ExecutionTarget::CPUFP16)
    {
        const auto& output = work.getMemoryManager<MemoryManagerFP16>()[mOutputName];

        ////if (mNetworkParams.isGradNeeded(mInputName))
        {
            const auto& deltas = work.getMemoryManager<MemoryManagerFP16>()[mOutputName.grad()];
            auto& prevLayerDelta = work.getMemoryManager<MemoryManagerFP16>()[mInputName.grad()];

            const size_t batchSize = prevLayerDelta.getBatchSize();

            if (mDimension == Dimension::Default)
            {
                for (size_t q = 0; q < batchSize; ++q)
                {
                    size_t batchOffset = q * mInputsCount;

                    for (size_t i = 0; i < mInputsCount; ++i)
                    {
                        dtype sum = 0.0_dt;
                        dtype e = static_cast<dtype>(exp(TODTYPE(output[batchOffset + i])));
                        for (size_t j = 0; j < mInputsCount; ++j)
                        {
                            sum += TODTYPE(deltas[batchOffset + j]) * e;
                        }
                        prevLayerDelta[batchOffset + i] += deltas[batchOffset + i] - TOHTYPE(sum);
                    }
                }
            }
            else if (mDimension == Dimension::Width)
            {
                size_t size = batchSize * deltas.getDepth() * deltas.getHeight();

                auto deltas2D = deltas.reshape(yato::dims(size, deltas.getWidth()));
                auto prevLayerDelta2D = prevLayerDelta.reshape(yato::dims(size, deltas.getWidth()));
                auto output2D = output.reshape(yato::dims(size, deltas.getWidth()));

                for (size_t q = 0; q < size; ++q)
                {
                    for (size_t i = 0; i < deltas.getWidth(); ++i)
                    {
                        dtype sum = 0.0_dt;
                        dtype e = static_cast<dtype>(exp(TODTYPE(output2D[q][i])));
                        for (size_t j = 0; j < deltas.getWidth(); ++j)
                        {
                            sum += TODTYPE(deltas2D[q][j]) * e;
                        }
                        prevLayerDelta2D[q][i] += deltas2D[q][i] - TOHTYPE(sum);
                    }
                }
            }
        }
    }
    else
    {
        THROW_NONAME("LogSoftMaxActivation", "unsupported execution target");
    }
}

} // namespace raul

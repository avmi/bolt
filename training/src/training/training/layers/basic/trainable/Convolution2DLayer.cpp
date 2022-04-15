// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "Convolution2DLayer.h"

#include <functional>

#if defined(_OPENMP)
#include <omp.h>
#endif

#include "impl/Convolution2DLayerCPU.h"

namespace raul
{

Convolution2DLayer::Convolution2DLayer(const Name& name, const Convolution2DParams& params, NetworkParameters& networkParameters)
    : Convolution2DLayer(name, "Convolution2D", params, networkParameters){ MEASURE_BLOCK("Convolution2DLayer[" + mName + "::ctor]") }

    Convolution2DLayer::Convolution2DLayer(const Name& name, const std::string& typeName, const Convolution2DParams& params, NetworkParameters& networkParameters)
    : TrainableLayer(name, typeName, params, networkParameters)
    , mKernelWidth(params.kernelWidth)
    , mKernelHeight(params.kernelHeight)
    , mKernelsCount(params.kernelsCount)
    , mStrideW(params.strideW)
    , mStrideH(params.strideH)
    , mPaddingW(params.paddingW)
    , mPaddingH(params.paddingH)
    , mDilationW(params.mDilationW)
    , mDilationH(params.mDilationH)
    , mGroups(params.mGroups)
    , mUseBias(params.bias)
    , mQuantizeWeights(params.quantizeWeights)
    , mDilationEnabled(false)
{
    MEASURE_BLOCK(mTypeName + "[" + mName + "::ctor]")
    auto prefix = mTypeName + "[" + mName + "::ctor]: ";

    DECLARE_IMPL(Convolution2DLayer, Convolution2DLayerCPU<MemoryManager>, NotImplemented, Convolution2DLayerCPU<MemoryManagerFP16>)

    if (mInputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of input names");
    }
    if (mOutputs.size() != 1)
    {
        THROW(mTypeName, mName, "wrong number of output names");
    }
    if (mDilationW < 1 || mDilationH < 1)
    {
        THROW(mTypeName, mName, "dilation parameter should be at least 1");
    }

    if (mGroups < 1)
    {
        THROW(mTypeName, mName, "zero groups");
    }
    mInputName = mInputs[0];
    mOutputName = mOutputs[0];

    mInputDepth = mNetworkParams.mWorkflow.getDepth(mInputName);
    mInputHeight = mNetworkParams.mWorkflow.getHeight(mInputName);
    mInputWidth = mNetworkParams.mWorkflow.getWidth(mInputName);

    mEffectiveReceptiveFieldW = mDilationW * (mKernelWidth - 1) + 1;
    mEffectiveReceptiveFieldH = mDilationH * (mKernelHeight - 1) + 1;
    mOutputWidth = (mInputWidth + 2 * mPaddingW - mEffectiveReceptiveFieldW) / mStrideW + 1;
    mOutputHeight = (mInputHeight + 2 * mPaddingH - mEffectiveReceptiveFieldH) / mStrideH + 1;

    if (mInputDepth % mGroups != 0 || mKernelsCount % mGroups != 0)
    {
        THROW(mTypeName, mName, "bad number of groups");
    }
    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, raul::Workflow::Usage::ForwardAndBackward, raul::Workflow::Mode::Read);

    mNetworkParams.mWorkflow.copyDeclaration(mName, mInputName, mInputName.grad(), DEC_BACK_WRIT_ZERO);

    mNetworkParams.mWorkflow.tensorNeeded(mName, mOutputName, raul::WShape{ raul::BS(), mKernelsCount, mOutputHeight, mOutputWidth }, DEC_FORW_WRIT_COMP);

    mNetworkParams.mWorkflow.copyDeclaration(mName, mOutputName, mOutputName.grad(), DEC_BACK_READ);

#if !defined(RAUL_NAIVE_CONV_FORWARD) && !defined(RAUL_NAIVE_CONV_BACKWARD)
    if (mDilationW > 1 || mDilationH > 1)
    {
        mDilationEnabled = true;
        mDilationTensor = "Dilated" + mWeightsName;

        mNetworkParams.mWorkflow.tensorNeeded(mName, mDilationTensor, raul::WShape{ mKernelsCount, mInputDepth / mGroups, mEffectiveReceptiveFieldH, mEffectiveReceptiveFieldW }, DEC_FRBC_WRIT);
    }
#endif

    size_t numThreads = 1;
#if defined(_OPENMP)
#pragma omp parallel
    {
        numThreads = omp_get_num_threads();
    }
#endif
    for (size_t i = 0; i < numThreads; ++i)
    {
        Name im2ColF = mName / "Im2ColFor" / Conversions::toString(i);
        mIm2ColForward.push_back(im2ColF);

        Name im2ColB = mName / "Im2ColBack" / Conversions::toString(i);
        mIm2ColBackward.push_back(im2ColB);

        mNetworkParams.mWorkflow.tensorNeeded(
            mName, im2ColF, raul::WShape{ 1u, 1u, 1u, mOutputHeight * mOutputWidth * mInputDepth * mEffectiveReceptiveFieldH * mEffectiveReceptiveFieldW }, DEC_FORW_WRIT);

        mNetworkParams.mWorkflow.copyDeclaration(mName, im2ColF, im2ColB, DEC_BACK_WRIT);
    }
#if defined(RAUL_NAIVE_CONV_BACKWARD)
    mTmpTensorName = mName / "TmpTensor";
    size_t inputWidthPadded = mInputWidth + 2 * mPaddingW;
    size_t inputHeightPadded = mInputHeight + 2 * mPaddingH;
    mNetworkParams.mWorkflow.tensorNeeded(mName, mTmpTensorName, raul::WShape{ 1u, 1u, 1u, mInputDepth * inputHeightPadded * inputWidthPadded }, DEC_BACK_WRIT_ZERO);
#endif
    mNetworkParams.mWorkflow.tensorNeeded(mName, mWeightsName, raul::WShape{ mKernelsCount, mInputDepth / mGroups, mKernelHeight, mKernelWidth }, DEC_TRAINABLE);

    if (mUseBias)
    {
        mNetworkParams.mWorkflow.tensorNeeded(mName, mBiasesName, raul::WShape{ 1u, mKernelsCount, 1u, 1u }, DEC_TRAINABLE);
    }

    if (!mFrozen)
    {
        mNetworkParams.mWorkflow.copyDeclaration(mName, mWeightsName, mWeightsName.grad(), DEC_TRAINABLE_GRAD);

        if (mUseBias)
        {
            mNetworkParams.mWorkflow.copyDeclaration(mName, mBiasesName, mBiasesName.grad(), DEC_TRAINABLE_GRAD);
        }
    }

    if (mQuantizeWeights)
    {
        if (!mNetworkParams.mQuantizerPtr)
        {
            THROW(mTypeName, mName, "quantizer is not defined");
        }
        mWeightsBackup = mWeightsName + "_backup";

        mNetworkParams.mWorkflow.copyDeclaration(mName, mWeightsName, mWeightsBackup, DEC_FORW_WRIT);
    }
}

void Convolution2DLayer::onBatchSizeChanged(size_t newBatchSize)
{
    if (mImpl)
    {
        mImpl->onBatchSizeChanged(newBatchSize);
    }
}

// TODO(d.polubotko): consider move naive into mImpl
#ifdef RAUL_NAIVE_CONV_FORWARD
void Convolution2DLayer::forwardComputeImpl(NetworkMode)
{
    const size_t batchSize = mNetworkParams.mWorkflow.getBatchSize();

    Tensor& output = mNetworkParams.mMemoryManager[mOutputName];

    Tensor& inputs = mNetworkParams.mMemoryManager[mInputName];

    Tensor& weights = mNetworkParams.mMemoryManager[mWeightsName];

    if (mQuantizeWeights)
    {
        mNetworkParams.mMemoryManager[mWeightsBackup] = TORANGE(weights);
        mNetworkParams.mQuantizerPtr->quantize(weights.begin(), weights.end());
        mNetworkParams.mQuantizerPtr->dequantize(weights.begin(), weights.end());
    }

    Common::conv2d(&inputs[0],
                   &output[0],
                   &weights[0],
                   mUseBias ? &mNetworkParams.mMemoryManager[mBiasesName][0] : nullptr,
                   batchSize,
                   mInputWidth,
                   mInputHeight,
                   mInputDepth,
                   mOutputWidth,
                   mOutputHeight,
                   mKernelsCount,
                   mKernelWidth,
                   mKernelHeight,
                   mPaddingW,
                   mPaddingH,
                   mStrideW,
                   mStrideH,
                   mDilationW,
                   mDilationH,
                   mGroups);

    if (mQuantizeWeights)
    {
        weights = TORANGE(mNetworkParams.mMemoryManager[mWeightsBackup]);
    }
}
#else
void Convolution2DLayer::forwardComputeImpl(NetworkMode mode)
{
    mImpl->forwardComputeImpl(mode);
}
#endif

// TODO(d.polubotko): consider move naive into mImpl
#ifdef RAUL_NAIVE_CONV_BACKWARD
void Convolution2DLayer::backwardComputeImpl()
{
    const size_t batchSize = mNetworkParams.mWorkflow.getBatchSize();

    Tensor& deltas = mNetworkParams.mMemoryManager[mOutputName.grad()];

    auto deltas4D = deltas.reshape(yato::dims(batchSize, mKernelsCount, mOutputHeight, mOutputWidth));

    const Tensor& weights = mNetworkParams.mMemoryManager[mWeightsName];

    auto kernelsWeights4D = weights.reshape(yato::dims(mKernelsCount, mInputDepth / mGroups, mKernelHeight, mKernelWidth));

    size_t inputWidthPadded = mInputWidth + 2 * mPaddingW;
    size_t inputHeightPadded = mInputHeight + 2 * mPaddingH;

    Tensor& prevDeltaTmp = mNetworkParams.mMemoryManager[mTmpTensorName];
    auto prevDeltaTmp3D = prevDeltaTmp.reshape(yato::dims(mInputDepth, inputHeightPadded, inputWidthPadded));

    // prevDelta
    ////if (mNetworkParams.isGradNeeded(mInputName))
    {
        Tensor& prevLayerDelta = mNetworkParams.mMemoryManager[mInputName.grad()];

        auto prevDeltas3D = prevLayerDelta.reshape(yato::dims(batchSize, mInputDepth, mInputHeight * mInputWidth));

        for (size_t i = 0; i < batchSize; ++i)
        {
            std::fill(prevDeltaTmp.begin(), prevDeltaTmp.end(), 0.0_dt);

            for (size_t group = 0; group < mGroups; ++group)
            {
                for (size_t d = 0; d < mInputDepth / mGroups; ++d)
                {
                    for (size_t kernelIndex = 0; kernelIndex < mKernelsCount / mGroups; ++kernelIndex)
                    {
                        for (size_t oy = 0; oy < mOutputHeight; ++oy)
                        {
                            for (size_t ox = 0; ox < mOutputWidth; ++ox)
                            {
                                for (size_t ky = 0; ky < mKernelHeight; ++ky)
                                {
                                    for (size_t kx = 0; kx < mKernelWidth; ++kx)
                                    {
                                        prevDeltaTmp3D[d + group * mInputDepth / mGroups][oy * mStrideH + ky * mDilationH][ox * mStrideW + kx * mDilationW] +=
                                            deltas4D[i][kernelIndex + group * mKernelsCount / mGroups][oy][ox] * kernelsWeights4D[kernelIndex + group * mKernelsCount / mGroups][d][ky][kx];
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Common::removePadding2D(&prevDeltaTmp3D[0][0][0], &prevDeltas3D[i][0][0], mInputDepth, inputWidthPadded, inputHeightPadded, mInputWidth, mInputHeight, false);
        }
    }

    if (!mFrozen)
    {
        Tensor& inputs = mNetworkParams.mMemoryManager[mInputName];

        Tensor& gradWeights = mNetworkParams.mMemoryManager[mWeightsName.grad()];

        auto inputs3D = inputs.reshape(yato::dims(batchSize, mInputDepth, mInputHeight * mInputWidth));
        auto gradWeights4D = gradWeights.reshape(yato::dims(mKernelsCount, mInputDepth / mGroups, mKernelHeight, mKernelWidth));

        // gradients weights
        for (size_t i = 0; i < batchSize; ++i)
        {
            Tensor& inputPadded = mNetworkParams.mMemoryManager[mTmpTensorName];
            inputPadded = 0_dt;

            Common::addPadding2D(&inputs3D[i][0][0], inputPadded.data(), mInputDepth, mInputWidth, mInputHeight, inputWidthPadded, inputHeightPadded);

            auto inputPadded3D = inputPadded.reshape(yato::dims(mInputDepth, inputHeightPadded, inputWidthPadded));

            for (size_t group = 0; group < mGroups; ++group)
            {
                for (size_t d = 0; d < mInputDepth / mGroups; ++d)
                {
                    for (size_t kernelIndex = 0; kernelIndex < mKernelsCount / mGroups; ++kernelIndex)
                    {
                        for (size_t ky = 0; ky < mKernelHeight; ++ky)
                        {
                            for (size_t kx = 0; kx < mKernelWidth; ++kx)
                            {
                                for (size_t oy = 0; oy < mOutputHeight; ++oy)
                                {
                                    for (size_t ox = 0; ox < mOutputWidth; ++ox)
                                    {
                                        gradWeights4D[kernelIndex + group * mKernelsCount / mGroups][d][ky][kx] +=
                                            deltas4D[i][kernelIndex + group * mKernelsCount / mGroups][oy][ox] *
                                            inputPadded3D[d + group * mInputDepth / mGroups][oy * mStrideH + ky * mDilationH][ox * mStrideW + kx * mDilationW];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // gradients biases
        if (!mFrozen && mUseBias)
        {
            Tensor& gradBiases = mNetworkParams.mMemoryManager[mBiasesName.grad()];

            for (size_t kernelIndex = 0; kernelIndex < mKernelsCount; ++kernelIndex)
            {
                for (size_t i = 0; i < batchSize; ++i)
                {
                    for (size_t oh = 0; oh < mOutputHeight; ++oh)
                    {
                        for (size_t ow = 0; ow < mOutputWidth; ++ow)
                        {
                            gradBiases[kernelIndex] += deltas4D[i][kernelIndex][oh][ow];
                        }
                    }
                }
            }
        }
    }
}
#else
void Convolution2DLayer::backwardComputeImpl()
{
    mImpl->backwardComputeImpl();
}
#endif

} // namespace raul

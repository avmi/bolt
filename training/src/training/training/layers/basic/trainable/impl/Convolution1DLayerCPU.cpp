// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "Convolution1DLayerCPU.h"
#include "../Convolution1DLayer.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

namespace raul
{

template<typename MM>
void Convolution1DLayerCPU<MM>::forwardComputeImpl(NetworkMode)
{
    Workflow& work = mLayer.mNetworkParams.mWorkflow;

    auto& weights = work.getMemoryManager<MM>()[mLayer.mWeightsName];

    if (mLayer.mQuantizeWeights)
    {
        work.getMemoryManager<MM>()[mLayer.mWeightsBackup] = TORANGE_MM(weights);
        mLayer.mNetworkParams.mQuantizerPtr->quantize(weights.begin(), weights.end());
        mLayer.mNetworkParams.mQuantizerPtr->dequantize(weights.begin(), weights.end());
    }

    const size_t batchSize = work.getBatchSize();

    auto& output = work.getMemoryManager<MM>()[mLayer.mOutputName];

    const auto& inputs = work.getMemoryManager<MM>()[mLayer.mInputName];
    auto inputs3D = mLayer.mTFStyle ? inputs.reshape(yato::dims(batchSize, mLayer.mInputSize, mLayer.mInputChannels)) : inputs.reshape(yato::dims(batchSize, mLayer.mInputChannels, mLayer.mInputSize));
    auto outputs3D =
        mLayer.mTFStyle ? output.reshape(yato::dims(batchSize, mLayer.mOutputSize, mLayer.mOutputChannels)) : output.reshape(yato::dims(batchSize, mLayer.mOutputChannels, mLayer.mOutputSize));

    auto kernelsWeights3D = mLayer.mTFStyle ? weights.reshape(yato::dims(mLayer.mKernelSize, mLayer.mInputChannels / mLayer.mGroups, mLayer.mOutputChannels))
                                            : weights.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize));
    auto finalKernelsWeights3D = kernelsWeights3D;

    if (mLayer.mDilationEnabled)
    {
        auto dilatedKernelsWeights3D =
            work.getMemoryManager<MM>()[mLayer.mDilationTensor].reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mEffectiveReceptiveField));
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (size_t kernelIndex = 0; kernelIndex < mLayer.mOutputChannels; ++kernelIndex)
        {
            for (size_t d = 0; d < mLayer.mInputChannels / mLayer.mGroups; ++d)
            {
                for (size_t kx = 0; kx < mLayer.mKernelSize; ++kx)
                {
                    if (mLayer.mTFStyle)
                    {
                        dilatedKernelsWeights3D[kernelIndex][d][kx * mLayer.mDilation] = kernelsWeights3D[kx][d][kernelIndex];
                    }
                    else
                    {
                        dilatedKernelsWeights3D[kernelIndex][d][kx * mLayer.mDilation] = kernelsWeights3D[kernelIndex][d][kx];
                    }
                }
            }
        }
        finalKernelsWeights3D = dilatedKernelsWeights3D;
    }
    else if (mLayer.mTFStyle)
    {
        finalKernelsWeights3D = work.getMemoryManager<MM>()[mLayer.mTempWeights].reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize));
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (size_t i = 0; i < mLayer.mOutputChannels; ++i)
        {
            for (size_t j = 0; j < mLayer.mInputChannels / mLayer.mGroups; ++j)
            {
                for (size_t k = 0; k < mLayer.mKernelSize; ++k)
                {
                    finalKernelsWeights3D[i][j][k] = kernelsWeights3D[k][j][i];
                }
            }
        }
    }

#if defined(_OPENMP)
#pragma omp parallel for
#endif
    for (size_t q = 0; q < batchSize; ++q)
    {
        size_t index = 0;
#if defined(_OPENMP)
        index = omp_get_thread_num();
#endif
        auto& im2ColFor = work.getMemoryManager<MM>()[mLayer.mIm2ColForward[index]];
        Common::im2col(&inputs3D[q][0][0], mLayer.mInputSize, 1u, mLayer.mInputChannels, mLayer.mEffectiveReceptiveField, 1u, mLayer.mStride, 1u, mLayer.mPadding, 0, &im2ColFor[0], mLayer.mTFStyle);

        for (size_t group = 0; group < mLayer.mGroups; ++group)
        {
            if (mLayer.mTFStyle)
            {
                Common::gemm(&mLayer.mNetworkParams.mWorkflow.getKernelManager(),
                             "",
                             CblasTrans,
                             CblasTrans,
                             mLayer.mOutputSize,
                             mLayer.mOutputChannels / mLayer.mGroups,
                             mLayer.mEffectiveReceptiveField * mLayer.mInputChannels / mLayer.mGroups,
                             1.0_dt,
                             &im2ColFor[0] + group * mLayer.mInputChannels / mLayer.mGroups * mLayer.mEffectiveReceptiveField * mLayer.mOutputSize,
                             &finalKernelsWeights3D[group * mLayer.mOutputChannels / mLayer.mGroups][0][0],
                             0.0_dt,
                             &outputs3D[q][0][group * mLayer.mOutputChannels / mLayer.mGroups]);
            }
            else
            {
                Common::gemm(&mLayer.mNetworkParams.mWorkflow.getKernelManager(),
                             "",
                             CblasNoTrans,
                             CblasNoTrans,
                             mLayer.mOutputChannels / mLayer.mGroups,
                             mLayer.mOutputSize,
                             mLayer.mEffectiveReceptiveField * mLayer.mInputChannels / mLayer.mGroups,
                             1.0_dt,
                             &finalKernelsWeights3D[group * mLayer.mOutputChannels / mLayer.mGroups][0][0],
                             &im2ColFor[0] + group * mLayer.mInputChannels / mLayer.mGroups * mLayer.mEffectiveReceptiveField * mLayer.mOutputSize,
                             0.0_dt,
                             &outputs3D[q][group * mLayer.mOutputChannels / mLayer.mGroups][0]);
            }
        }
    }

    if (mLayer.mUseBias)
    {
        const auto& biases = work.getMemoryManager<MM>()[mLayer.mBiasesName];
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (size_t q = 0; q < batchSize; ++q)
        {
            for (size_t i = 0; i < mLayer.mOutputSize; ++i)
            {
                for (size_t kernelIndex = 0; kernelIndex < mLayer.mOutputChannels; ++kernelIndex)
                {
                    if (mLayer.mTFStyle)
                    {
                        outputs3D[q][i][kernelIndex] += biases[kernelIndex];
                    }
                    else
                    {
                        outputs3D[q][kernelIndex][i] += biases[kernelIndex];
                    }
                }
            }
        }
    }

    if (mLayer.mQuantizeWeights)
    {
        weights = TORANGE_MM(work.getMemoryManager<MM>()[mLayer.mWeightsBackup]);
    }
}

template<typename MM>
void Convolution1DLayerCPU<MM>::backwardComputeImpl()
{
    Workflow& work = mLayer.mNetworkParams.mWorkflow;

    const size_t batchSize = work.getBatchSize();

    const auto& deltas = work.getMemoryManager<MM>()[mLayer.mOutputName.grad()];
    auto deltas3D =
        mLayer.mTFStyle ? deltas.reshape(yato::dims(batchSize, mLayer.mOutputSize, mLayer.mOutputChannels)) : deltas.reshape(yato::dims(batchSize, mLayer.mOutputChannels, mLayer.mOutputSize));
    const auto& weights = mLayer.mDilationEnabled ? work.getMemoryManager<MM>()[mLayer.mDilationTensor]
                                                  : mLayer.mTFStyle ? work.getMemoryManager<MM>()[mLayer.mTempWeights] : work.getMemoryManager<MM>()[mLayer.mWeightsName];

    auto finalKernelsWeights3D = mLayer.mDilationEnabled ? weights.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mEffectiveReceptiveField))
                                                         : mLayer.mTFStyle ? weights.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize))
                                                                           : weights.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize));

    // prevDelta
    // if (mLayer.mNetworkParams.isGradNeeded(mLayer.mInputName))
    {
        auto& prevLayerDelta = work.getMemoryManager<MM>()[mLayer.mInputName.grad()];
        auto prevDeltas3D = mLayer.mTFStyle ? prevLayerDelta.reshape(yato::dims(batchSize, mLayer.mInputChannels, mLayer.mInputSize))
                                            : prevLayerDelta.reshape(yato::dims(batchSize, mLayer.mInputSize, mLayer.mInputChannels));
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (size_t q = 0; q < batchSize; ++q)
        {
            size_t index = 0;
#if defined(_OPENMP)
            index = omp_get_thread_num();
#endif
            auto& im2ColBack = work.getMemoryManager<MM>()[mLayer.mIm2ColBackward[index]];
            for (size_t group = 0; group < mLayer.mGroups; ++group)
            {
                if (mLayer.mTFStyle)
                {
                    Common::gemm(&mLayer.mNetworkParams.mWorkflow.getKernelManager(),
                                 "",
                                 CblasTrans,
                                 CblasTrans,
                                 mLayer.mEffectiveReceptiveField * mLayer.mInputChannels / mLayer.mGroups,
                                 mLayer.mOutputSize,
                                 mLayer.mOutputChannels / mLayer.mGroups,
                                 1.0_dt,
                                 &finalKernelsWeights3D[group * mLayer.mOutputChannels / mLayer.mGroups][0][0],
                                 &deltas3D[q][0][group * mLayer.mOutputChannels / mLayer.mGroups],
                                 0.0_dt,
                                 &im2ColBack[0] + group * mLayer.mEffectiveReceptiveField * mLayer.mInputChannels * mLayer.mOutputSize / mLayer.mGroups);
                }
                else
                {
                    Common::gemm(&mLayer.mNetworkParams.mWorkflow.getKernelManager(),
                                 "",
                                 CblasTrans,
                                 CblasNoTrans,
                                 mLayer.mEffectiveReceptiveField * mLayer.mInputChannels / mLayer.mGroups,
                                 mLayer.mOutputSize,
                                 mLayer.mOutputChannels / mLayer.mGroups,
                                 1.0_dt,
                                 &finalKernelsWeights3D[group * mLayer.mOutputChannels / mLayer.mGroups][0][0],
                                 &deltas3D[q][group * mLayer.mOutputChannels / mLayer.mGroups][0],
                                 0.0_dt,
                                 &im2ColBack[0] + group * mLayer.mEffectiveReceptiveField * mLayer.mInputChannels * mLayer.mOutputSize / mLayer.mGroups);
                }
            }
            Common::col2im(&im2ColBack[0],
                           mLayer.mInputSize,
                           1u,
                           mLayer.mInputChannels,
                           mLayer.mEffectiveReceptiveField,
                           1u,
                           mLayer.mStride,
                           1u,
                           mLayer.mPadding,
                           0,
                           &prevDeltas3D[q][0][0],
                           mLayer.mTFStyle,
                           false);
        }
    }

    // gradients weights
    if (!mLayer.mFrozen)
    {
        const auto& inputs = work.getMemoryManager<MM>()[mLayer.mInputName];
        auto inputs3D =
            mLayer.mTFStyle ? inputs.reshape(yato::dims(batchSize, mLayer.mInputSize, mLayer.mInputChannels)) : inputs.reshape(yato::dims(batchSize, mLayer.mInputChannels, mLayer.mInputSize));

        auto& im2colMatrix = work.getMemoryManager<MM>()[mLayer.mTmpIm2ColName];
        auto im2colMatrix2D = im2colMatrix.reshape(yato::dims(batchSize, mLayer.mOutputSize * mLayer.mInputChannels * mLayer.mEffectiveReceptiveField));

        auto& gradWeights = work.getMemoryManager<MM>()[mLayer.mWeightsName.grad()];
        auto& tmpWeightsGrad = mLayer.mDilationEnabled ? work.getMemoryManager<MM>()[mLayer.mDilationTensor] : mLayer.mTFStyle ? work.getMemoryManager<MM>()[mLayer.mTempWeights.grad()] : gradWeights;
        auto gradWeights3D = mLayer.mDilationEnabled ? tmpWeightsGrad.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mEffectiveReceptiveField))
                                                     : mLayer.mTFStyle ? tmpWeightsGrad.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize))
                                                                       : tmpWeightsGrad.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize));

        if (mLayer.mDilationEnabled || mLayer.mTFStyle)
        {
            tmpWeightsGrad = TOMMTYPE(0);
        }
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (size_t q = 0; q < batchSize; ++q)
        {
            Common::im2col(
                &inputs3D[q][0][0], mLayer.mInputSize, 1u, mLayer.mInputChannels, mLayer.mEffectiveReceptiveField, 1u, mLayer.mStride, 1u, mLayer.mPadding, 0, &im2colMatrix2D[q][0], mLayer.mTFStyle);
        }

        for (size_t q = 0; q < batchSize; ++q)
        {
            for (size_t group = 0; group < mLayer.mGroups; ++group)
            {
                if (mLayer.mTFStyle)
                {
                    Common::gemm(&mLayer.mNetworkParams.mWorkflow.getKernelManager(),
                                 "",
                                 CblasTrans,
                                 CblasTrans,
                                 mLayer.mOutputChannels / mLayer.mGroups,
                                 mLayer.mEffectiveReceptiveField * mLayer.mInputChannels / mLayer.mGroups,
                                 mLayer.mOutputSize,
                                 1.0_dt,
                                 &deltas3D[q][0][group * mLayer.mOutputChannels / mLayer.mGroups],
                                 &im2colMatrix2D[q][0] + group * mLayer.mOutputSize * mLayer.mInputChannels * mLayer.mEffectiveReceptiveField / mLayer.mGroups,
                                 1.0_dt,
                                 &gradWeights3D[group * mLayer.mOutputChannels / mLayer.mGroups][0][0]);
                }
                else
                {
                    Common::gemm(&mLayer.mNetworkParams.mWorkflow.getKernelManager(),
                                 "",
                                 CblasNoTrans,
                                 CblasTrans,
                                 mLayer.mOutputChannels / mLayer.mGroups,
                                 mLayer.mEffectiveReceptiveField * mLayer.mInputChannels / mLayer.mGroups,
                                 mLayer.mOutputSize,
                                 1.0_dt,
                                 &deltas3D[q][group * mLayer.mOutputChannels / mLayer.mGroups][0],
                                 &im2colMatrix2D[q][0] + group * mLayer.mOutputSize * mLayer.mInputChannels * mLayer.mEffectiveReceptiveField / mLayer.mGroups,
                                 1.0_dt,
                                 &gradWeights3D[group * mLayer.mOutputChannels / mLayer.mGroups][0][0]);
                }
            }
        }

        if (mLayer.mTFStyle)
        {
            auto realGradWeights3D = gradWeights.reshape(yato::dims(mLayer.mKernelSize, mLayer.mInputChannels / mLayer.mGroups, mLayer.mOutputChannels));
#if defined(_OPENMP)
#pragma omp parallel for
#endif
            for (size_t kx = 0; kx < mLayer.mKernelSize; ++kx)
            {
                for (size_t d = 0; d < mLayer.mInputChannels / mLayer.mGroups; ++d)
                {
                    for (size_t kernelIndex = 0; kernelIndex < mLayer.mOutputChannels; ++kernelIndex)
                    {
                        realGradWeights3D[kx][d][kernelIndex] += gradWeights3D[kernelIndex][d][kx * mLayer.mDilation];
                    }
                }
            }
        }
        else
        {
            if (mLayer.mDilationEnabled)
            {
                auto realGradWeights3D = gradWeights.reshape(yato::dims(mLayer.mOutputChannels, mLayer.mInputChannels / mLayer.mGroups, mLayer.mKernelSize));
#if defined(_OPENMP)
#pragma omp parallel for
#endif
                for (size_t kernelIndex = 0; kernelIndex < mLayer.mOutputChannels; ++kernelIndex)
                {
                    for (size_t d = 0; d < mLayer.mInputChannels / mLayer.mGroups; ++d)
                    {
                        for (size_t kx = 0; kx < mLayer.mKernelSize; ++kx)
                        {
                            realGradWeights3D[kernelIndex][d][kx] += gradWeights3D[kernelIndex][d][kx * mLayer.mDilation];
                        }
                    }
                }
            }
        }

        if (mLayer.mUseBias)
        {
            auto& gradBiases = work.getMemoryManager<MM>()[mLayer.mBiasesName.grad()];
            for (size_t kernelIndex = 0; kernelIndex < mLayer.mOutputChannels; ++kernelIndex)
            {
                auto gradBias = 0.0_dt;
#if defined(_OPENMP)
#pragma omp parallel for reduction(+ : gradBias)
#endif
                for (size_t i = 0; i < batchSize; ++i)
                {
                    for (size_t j = 0; j < mLayer.mOutputSize; ++j)
                    {
                        if (mLayer.mTFStyle)
                        {
                            gradBias += deltas3D[i][j][kernelIndex];
                        }
                        else
                        {
                            gradBias += deltas3D[i][kernelIndex][j];
                        }
                    }
                }
                gradBiases[kernelIndex] += TOMMTYPE(gradBias);
            }
        }
    }
}

template class Convolution1DLayerCPU<MemoryManager>;
template class Convolution1DLayerCPU<MemoryManagerFP16>;

} // namespace raul
// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "TileLayerCPU.h"
#include "../TileLayer.h"

namespace raul
{

template<typename MM>
TileLayerCPU<MM>::TileLayerCPU(TileLayer& layer)
    : mLayer(layer)
{
}

template<typename MM>
void TileLayerCPU<MM>::forwardComputeImpl(NetworkMode)
{
    auto& work = mLayer.mNetworkParams.mWorkflow;

    auto& output = work.getMemoryManager<MM>()[mLayer.mOutputs[0]];
    const auto& input = work.getMemoryManager<MM>()[mLayer.mInputs[0]];

    // Get 4D view
    const auto input4D = input.get4DView();
    auto output4D = output.get4DView();
    // Need shapes to get indices
    const auto outputShape = output.getShape();
    const auto inputShape = input.getShape();
#if defined(_OPENMP)
#pragma omp parallel for
#endif
    for (size_t i = 0; i < outputShape[0]; ++i)
    {
        for (size_t j = 0; j < outputShape[1]; ++j)
        {
            for (size_t k = 0; k < outputShape[2]; ++k)
            {
                for (size_t q = 0; q < outputShape[3]; ++q)
                {
                    output4D[i][j][k][q] = input4D[i][j % inputShape[1]][k % inputShape[2]][q % inputShape[3]];
                }
            }
        }
    }
}

template<typename MM>
void TileLayerCPU<MM>::backwardComputeImpl()
{
    auto& work = mLayer.mNetworkParams.mWorkflow;

    const auto& delta = work.getMemoryManager<MM>()[mLayer.mOutputs[0].grad()];

    // if (mNetworkParams.isGradNeeded(mInputs[0]))
    {
        auto& prevLayerNabla = work.getMemoryManager<MM>()[mLayer.mInputs[0].grad()];
        // Get 4D view
        const auto delta4D = delta.get4DView();
        auto prevLayerNabla4D = prevLayerNabla.get4DView();
        // Need shapes to get indices
        const auto deltaShape = delta.getShape();
        const auto inputShape = prevLayerNabla.getShape();
#if defined(_OPENMP)
#pragma omp parallel for
#endif
        for (size_t i = 0; i < deltaShape[0]; ++i)
        {
            for (size_t j = 0; j < deltaShape[1]; ++j)
            {
                for (size_t k = 0; k < deltaShape[2]; ++k)
                {
                    for (size_t q = 0; q < deltaShape[3]; ++q)
                    {
                        prevLayerNabla4D[i][j % inputShape[1]][k % inputShape[2]][q % inputShape[3]] += delta4D[i][j][k][q];
                    }
                }
            }
        }
    }
}

template class TileLayerCPU<MemoryManager>;
template class TileLayerCPU<MemoryManagerFP16>;
} // namespace raul

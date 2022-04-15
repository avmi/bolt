// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef TENSOR_LAYER_H
#define TENSOR_LAYER_H

#include <training/common/Common.h>
#include <training/layers/BasicLayer.h>

#include <training/layers/parameters/TensorParams.h>

namespace raul
{

/**
 * @brief Tensor Layer
 *
 * A technical operation which inserts a constant tensor into a topology.
 */
class TensorLayer : public BasicLayer
{
  public:
    TensorLayer(const Name& name, const TensorParams& params, NetworkParameters& networkParameters);

    TensorLayer(TensorLayer&&) = default;
    TensorLayer(const TensorLayer&) = delete;
    TensorLayer& operator=(const TensorLayer&) = delete;

  private:
    bool mInit;
    dtype mInitValue;

    template<typename MM>
    friend class TensorLayerCPU;
    friend class TensorLayerGPU;
};

} // raul namespace

#endif // TENSOR_LAYER_H
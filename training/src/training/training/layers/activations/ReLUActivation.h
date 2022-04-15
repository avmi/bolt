// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef RELU_ACTIVATION_H
#define RELU_ACTIVATION_H

#include "training/layers/BasicLayer.h"

#include <training/common/Common.h>

namespace raul
{

/**
 * @brief Rectified linear unit activation function
 *
 * The layer applies the element-wise rectified linear unit.
 */
class ReLUActivation : public BasicLayer
{
  public:
    ReLUActivation(const Name& name, const BasicParams& params, NetworkParameters& networkParameters);

    ReLUActivation(ReLUActivation&&) = default;
    ReLUActivation(const ReLUActivation&) = delete;
    ReLUActivation& operator=(const ReLUActivation&) = delete;

  protected:
    ReLUActivation(const Name& name, const std::string& typeName, const BasicParams& params, NetworkParameters& networkParameters);

    Name mInputName;
    Name mOutputName;

    size_t mDepth;
    size_t mHeight;
    size_t mWidth;

    template<typename MM>
    friend class ReLUActivationImpl;
};

class ReLU6Activation : public ReLUActivation
{
  public:
    ReLU6Activation(const Name& name, const BasicParams& params, NetworkParameters& networkParameters);

    ReLU6Activation(ReLU6Activation&&) = default;
    ReLU6Activation(const ReLU6Activation&) = delete;
    ReLU6Activation& operator=(const ReLU6Activation&) = delete;

  protected:
    template<typename MM>
    friend class ReLU6ActivationImpl;
};
} // raul namespace
#endif

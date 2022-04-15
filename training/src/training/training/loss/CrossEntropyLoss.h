// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef CROSS_ENTROPY_LOSS_H
#define CROSS_ENTROPY_LOSS_H

#include "LossWrapper.h"
#include <training/common/Common.h>
#include <training/layers/BasicLayer.h>

namespace raul
{

/**
 * @brief Cross-entropy Loss Function
 */
class CrossEntropyLoss : public BasicLayer
{
  public:
    CrossEntropyLoss(const Name& name, const LossParams& params, NetworkParameters& networkParameters);

    CrossEntropyLoss(CrossEntropyLoss&&) = default;
    CrossEntropyLoss(const CrossEntropyLoss&) = delete;
    CrossEntropyLoss& operator=(const CrossEntropyLoss&) = delete;

    void forwardComputeImpl(NetworkMode mode) override;
    void backwardComputeImpl() override;
    void initNotBSTensors() override;

  private:
    Name mInputName;
    std::string mLabelName;
    Name mOutputName;
    std::unique_ptr<LossWrapper<CrossEntropyLoss>> wrapper;
    bool mIsFinal;

    size_t mDepth;
    size_t mHeight;
    size_t mWidth;
};
} // raul namespace

#endif

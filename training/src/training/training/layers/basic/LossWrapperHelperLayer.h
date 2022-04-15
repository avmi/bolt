// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef LOSS_WRAPPER_HELPER_LAYER_H
#define LOSS_WRAPPER_HELPER_LAYER_H

#include <training/common/Common.h>
#include <training/layers/BasicLayer.h>

namespace raul
{

class LossWrapperHelperLayer : public BasicLayer
{
  public:
    LossWrapperHelperLayer(const Name& name, const BasicParams& params, bool isFinal, NetworkParameters& networkParameters);

    LossWrapperHelperLayer(LossWrapperHelperLayer&&) = default;
    LossWrapperHelperLayer(const LossWrapperHelperLayer&) = delete;
    LossWrapperHelperLayer& operator=(const LossWrapperHelperLayer&) = delete;

  private:
    bool mIsFinal;

    template<typename MM>
    friend class LossWrapperHelperLayerCPU;
    friend class LossWrapperHelperLayerGPU;
};

} // raul namespace

#endif
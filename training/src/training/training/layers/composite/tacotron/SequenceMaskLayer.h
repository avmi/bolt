// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef TACOTRON_SEQUENCE_MASK_H
#define TACOTRON_SEQUENCE_MASK_H

#include "Miscellaneous.h"
#include <training/common/Common.h>
#include <training/layers/BasicLayer.h>
#include <training/layers/parameters/trainable/TacotronParams.h>

namespace raul
{
namespace tacotron
{
class SequenceMaskLayer : public BasicLayer
{
  public:
    SequenceMaskLayer(const Name& name, const BasicParams& params, size_t outputsPerStep, NetworkParameters& networkParameters);

    SequenceMaskLayer(SequenceMaskLayer&&) = default;
    SequenceMaskLayer(const SequenceMaskLayer&) = delete;
    SequenceMaskLayer& operator=(const SequenceMaskLayer&) = delete;

  private:
    std::string mLengthsName;
    std::string mOutputMaskName;

    size_t mOutputsPerStep;

    template<typename MM>
    friend class SequenceMaskLayerCPU;
    friend class SequenceMaskLayerGPU;
};

}
} // namespace raul::tacotron

#endif

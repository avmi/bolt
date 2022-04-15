// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "L2.h"
#include "training/common/Common.h"
#include "training/layers/BasicLayer.h"

namespace raul::optimizers::Regularizers::Strategies
{

dtype L2::operator()(dtype weight) const
{
    return 2.0_dt * this->mLambda * weight;
}

dtype L2::getPenalty(Workflow& net) const
{
    auto loss_delta = 0.0_dt;
    auto paramsAndWeights = net.getTrainableParameters();
    for (auto s : paramsAndWeights)
    {
        const auto n = s.Param.size();
        for (size_t i = 0; i < n; i++)
        {
            loss_delta += s.Param[i] * s.Param[i];
        }
    }
    return this->mLambda * loss_delta;
}

std::ostream& L2::as_ostream(std::ostream& out) const
{
    out << "L2(lambda=" << std::scientific << this->mLambda << ")";
    return out;
}

} // raul::loss::regularization
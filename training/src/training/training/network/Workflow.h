// Copyright (C) 2021. Huawei Technologies Co., Ltd. All rights reserved.

// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef WORKFLOW_H
#define WORKFLOW_H

#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <training/common/MemoryManager.h>
#include <training/common/MemoryManagerGPU.h>
#include <training/common/Name.h>
#include <training/common/quantization/IQuantizer.h>
#include <training/network/NetworkParameters.h>

#include <training/opencl/OpenCLKernelManager.h>

#include <training/loss/scaling/ScalingStrategy.h>

namespace raul
{
class BasicLayer;
struct BasicParams;
typedef std::shared_ptr<BasicLayer> LayerMem;

template<bool B, class T, class F>
using if_t = typename std::conditional<B, T, F>::type;

/**
 * @brief Placeholder for batch size
 */
struct BS
{
    BS()
        : multiplier(1)
    {
    }
    explicit BS(size_t multi)
        : multiplier(multi)
    {
    }

    size_t multiplier;
};

class Workflow;

/**
 * @brief Shape of tensor with batch size placeholders and multiplier possible
 */
class WShape
{
  public:
    WShape();

    WShape(const shape& shapeVal);

    template<typename T, typename U, typename V, typename W>
    WShape(T a, U b, V c, W d)
    {
        struct CheckNone
        {
            static void fill(WShape& shape, size_t val, size_t index)
            {
                shape.mShape[index] = val;
                shape.mIsBS[index] = false;
                shape.mMultiplier[index] = 1u;
            }
        };

        struct CheckBS
        {
            static void fill(WShape& shape, const BS& val, size_t index)
            {
                shape.mShape[index] = 0u; // getShape will recalculate
                shape.mIsBS[index] = true;
                shape.mMultiplier[index] = val.multiplier;
            }
        };

        typedef if_t<std::is_same<T, BS>::value, CheckBS, CheckNone> TypeA;

        typedef if_t<std::is_same<U, BS>::value, CheckBS, CheckNone> TypeB;

        typedef if_t<std::is_same<V, BS>::value, CheckBS, CheckNone> TypeC;

        typedef if_t<std::is_same<W, BS>::value, CheckBS, CheckNone> TypeD;

        TypeA::fill(*this, a, 0);
        TypeB::fill(*this, b, 1);
        TypeC::fill(*this, c, 2);
        TypeD::fill(*this, d, 3);
    }

    [[nodiscard]] bool isBSDependent() const;

    /**
     * @brief Get shape of tensor. If placeholder used and no BS defined - exception
     */
    [[nodiscard]] shape getShape(const Workflow& work) const;

    void selectMaxShape(WShape& other)
    {
        for (size_t q = 0; q < shape::dimensions_number; ++q)
        {
            if (!mIsBS[q] && !other.mIsBS[q])
            {
                mMultiplier[q] = std::max(mMultiplier[q], other.mMultiplier[q]);
                mShape[q] = std::max(mShape[q], other.mShape[q]);
                other.mMultiplier[q] = mMultiplier[q];
                other.mShape[q] = mShape[q];
            }
            else if ((!mIsBS[q] && other.mIsBS[q]) || (mIsBS[q] && !other.mIsBS[q]))
            {
                THROW_NONAME("WShape", "not BS layout in shapes");
            }
        }
    }

    bool operator==(const WShape&) const;
    bool operator!=(const WShape& other) const { return !operator==(other); }

    [[nodiscard]] std::string toString() const;

    friend class Workflow;

    friend std::ostream& operator<<(std::ostream& out, const WShape& instance) { return out << instance.toString(); }

  private:
    shape mShape;
    bool mIsBS[shape::dimensions_number];
    size_t mMultiplier[shape::dimensions_number];
};

class WorkflowListener
{
  public:
    virtual void BeforeForward(Workflow&) {}
    virtual void AfterForward(Workflow&) {}
    virtual void BeforeBackward(Workflow&) {}
    virtual void AfterBackward(Workflow&) {}
};

class WorkflowDB;
class WorkflowBasicPool;
template<typename MM>
class WorkflowPool;
class WorkflowPoolGPU;

/**
 * @brief Class to store all layers of deep neural network (DNN) and declarations of tensors.
 *
 * Topology of network represented as directed acyclic graph (DAG). Sequence of
 * layers should be added in topologically sorted order. Declaration of tensors needed
 * for automatic memory management. To declare tensor layer name and tensor name with
 * additional information should be defined. Layer name is a name of layer added into DAG.
 * Execution of test or train of DNN performed by pipelines - sequences of actions.
 * Action is an object to manipulate memory (allocate, deallocate, zero etc.) or
 * to execute layer (forward, backward).
 *
 * @todo(ck): simplify the class
 */
class Workflow
{
  public:
    /**
     * Training stage where tensor is used
     */
    enum class Usage
    {
        Forward = 0,  // mapped to std::vector<size_t> in TLTable
        Backward = 1, // mapped to std::vector<size_t> in TLTable
        ForwardAndBackward
    };

    /**
     * Tensor access mode
     */
    enum class Mode
    {
        Read,
        Write
    };

    /**
     * Pipeline execution mode
     */
    enum class Execution
    {
        Normal,
        Checkpointed
    };

    struct ActionParam
    {
        ActionParam() = default;
    };

    /**
     * @brief Action interface.
     *
     * Action is an object to manipulate memory.
     */
    struct Action
    {
        virtual ~Action() = default;
        virtual void execute(const ActionParam& param) = 0;
        [[nodiscard]] virtual std::string type() const = 0;
    };

    typedef std::shared_ptr<Action> ActionMem;
    typedef std::vector<ActionMem> Pipeline;

    enum class Pipelines
    {
        CreateBatched,    // create tensors with batch size in N
        CreateNotBatched, // create tensors with arbitrary number in N (once per workflow)
        DeleteBatched,    // delete tensors with batch size in N
        Zero,             // zero tensors (mostly gradients, once per iteration at the beginning)
        ForwardTest,
        ForwardTrain,
        BackwardTrain
    };

    /**
     * @brief Workflow ctor
     *
     * @param compressionMode
     * @param calculationMode
     * @param allocationMode memory allocation mode
     * @param executionTarget
     * @param quantizer
     *
     * A memory pool and other buffers are initialized in ctor if they are required.
     *
     */
    explicit Workflow(CompressionMode compressionMode = CompressionMode::NONE,
                      CalculationMode calculationMode = CalculationMode::DETERMINISTIC,
                      AllocationMode allocationMode = AllocationMode::STANDARD,
                      ExecutionTarget executionTarget = ExecutionTarget::CPU,
                      quantization::IQuantizer* quantizer = nullptr);

    Workflow(const Workflow&) = delete;
    Workflow& operator=(const Workflow&) = delete;

    virtual ~Workflow();

    /**
     * @brief Add layer helper
     */
    template<typename T, class... Args>
    void add(const Name& name, Args&&... _args)
    {
        if (mOverridedLayerExecutionTarget != LayerExecutionTarget::Default)
        {
            alterLayerExecutionTargetInParam(_args...);
        }

        addLayer(std::make_shared<T>(name, std::forward<Args>(_args)..., mNetworkParameters));
    }

    /**
     * @brief Declare tensor
     * layerName - name of layer tensor needed by
     * tensorName - name of tensor to create
     * shapeVar - shape of tensor
     * usage - tensor usage location
     * mode - tensor access mode
     * isOptimizeGraph - tensor might be removed if it is not used for mode read
     * isOptimizeMem - dynamically allocate / deallocate memory during training / testing, not possible together with isTrainable
     * isTrainable - indicate tensor will be updated by optimizer, not possible together with isOptimizeMem
     * isZero - fill tensor with zeros once per training iteration
     * isCompress - compress tensor after last usage in forward, decompress before first usage in backward
     *
     * @todo(ck): what if we make this function private? It requires rewrite layer appending logic
     */
    void tensorNeeded(const Name& layerName,
                      const Name& tensorName,
                      WShape shape,
                      Usage usage,
                      Mode mode,
                      bool isOptimizeGraph,
                      bool isOptimizeMem,
                      bool isTrainable,
                      bool isZero,
                      bool isCompress,
                      LayerExecutionTarget layerExecutionTarget = LayerExecutionTarget::Default);

    /**
     * @brief Declare tensor if not exists or copy declaration with maximizing shape
     * layerName - name of layer tensor neeeded by
     * tensorName - name of tensor to create
     * shapeVar - shape of tensor
     * usage - tensor usage location
     * mode - tensor access mode
     * isOptimizeGraph - tensor might be removed if it is not used for mode read
     * isOptimizeMem - dynamicly allocate / deallocate memory during training / testing, not possible together with isTrainable
     * isTrainable - indicate tensor will be updated by optimizer, not possible together with isOptimizeMem
     * isZero - fill tensor with zeros once per training iteration
     * isCompress - compress tensor after last usage in forward, decompress before first usage in backward
     */
    void tensorNeededMaxShape(const Name& layerName,
                              const Name& tensorName,
                              WShape shape,
                              Usage usage,
                              Mode mode,
                              bool isOptimizeGraph,
                              bool isOptimizeMem,
                              bool isTrainable,
                              bool isZero,
                              bool isCompress,
                              LayerExecutionTarget layerExecutionTarget = LayerExecutionTarget::Default);

    /**
     * @brief Copy declaration of tensor to different layer. Tensor name and shape will remain the same
     * Different tensors should be equivalent in terms of shape and flags (except zero)
     * Typical usage - change isZero flag using general tensorNeeded interface
     */
    void copyDeclaration(const Name& layerName, const Name& tensorName, Usage usage, Mode mode, bool isOptimizeGraph, bool isOptimizeMem, bool isTrainable, bool isZero, bool isCompress);

    /**
     * @brief Copy declaration of tensor to different layer. Tensor name, shape, flags will remain the same
     * Typical usage - set declaration for input CHW tensor
     */
    void copyDeclaration(const Name& layerName, const Name& tensorName, Usage usage, Mode mode);

    /**
     * @brief Copy declaration of tensor to different layer. Tensor name, shape, flags will remain the same
     * Typical usage - set declaration for input CHW tensor
     */
    void copyDeclaration(const Name& layerName, const Name& tensorName, Usage usage, Mode mode, LayerExecutionTarget layerExecutionTarget);

    /**
     * @brief Copy declaration of tensor to new tensor to some layer. Shape will remain the same
     * Typical usage - copy declaration of input tensor to declaration of output tensor without shape (CHW or NCHW) analysis
     */
    void copyDeclaration(const Name& layerName,
                         const Name& fromTensorName,
                         const Name& toTensorName,
                         Usage usage,
                         Mode mode,
                         bool isOptimizeGraph,
                         bool isOptimizeMem,
                         bool isTrainable,
                         bool isZero,
                         bool isCompress,
                         LayerExecutionTarget layerExecutionTarget = LayerExecutionTarget::Default);

    /**
     * @brief Copy declaration to layer if declaration exists (flags in params are ignored) or to tensor (flags in params are used) if not.
     * fromTensorName - if toTensorName declared then fromTensorName ignored, if not then source of copy
     * toTensorName - if toTensorName declared then source of copy, if not then destination
     */
    void copyDec(const Name& layerName,
                 const Name& fromTensorName,
                 const Name& toTensorName,
                 Usage usage,
                 Mode mode,
                 bool isOptimizeGraph,
                 bool isOptimizeMem,
                 bool isTrainable,
                 bool isZero,
                 bool isCompress);

    /**
     * @brief Create pipelines for training process
     */
    virtual void preparePipelines(Execution execution = Execution::Normal);

    /**
     * @brief Flush pipelines
     */
    void flush();

    /**
     * @brief Create tensors and allocate memory for not optimized not batched tensors, create shapes for optimized not batched tensors
     */
    virtual void prepareMemoryForTraining();

    template<typename MM = MemoryManager>
    std::vector<ParamAndGradImpl<typename MM::tensor>> getTrainableParameters();

    /**
     * @brief Get trainable params without exception if paramter is missed
     */
    template<typename MM = MemoryManager>
    std::vector<ParamAndGradImpl<typename MM::tensor>> getTrainableParametersSafe();

    /**
     * @brief Getter to names of trainable parameters
     *
     * @return names of trainable parameters
     *
     * @todo d.polubotko (TODO): speedup necessary
     */
    Names getTrainableParameterNames() const;

    /**
     * @brief Check if tensor is a trainable parameter
     */
    bool isTensorTrainable(const Name& name) const;

    /**
     * @brief List of layer parameters names (both trainable and non-trainable)
     *
     * Layer parameters = trainable or (non-optimizable and non-zero)
     *
     * DataLayer has no parameters
     */
    Names getLayerParameterNames(const Name& layerName) const;

    /**
     * @brief Check if tensor has been declared previously
     */
    bool isTensorDeclared(const Name& tensorName) const;

    /**
     * @brief Get N of NCHW tensor. If placeholder used and no BS defined - exception
     */
    size_t getBatch(const Name& tensorName) const;

    /**
     * @brief Check if placeholder used for N
     *
     * @todo(ck): rename function, there is no verb `placehold`
     */
    bool isBatchPlaceholded(const Name& tensorName) const;

    /**
     * @brief Get C of NCHW tensor. If placeholder used and no BS defined - exception
     */
    size_t getDepth(const Name& tensorName) const;

    /**
     * @brief Check if placeholder used for C
     *
     * @todo(ck): rename function, there is no verb `placehold`
     */
    bool isDepthPlaceholded(const Name& tensorName) const;

    /**
     * @brief Get H of NCHW tensor. If placeholder used and no BS defined - exception
     */
    size_t getHeight(const Name& tensorName) const;

    /**
     * @brief Check if placeholder used for H
     *
     * @todo(ck): rename function, there is no verb `placehold`
     */
    bool isHeightPlaceholded(const Name& tensorName) const;

    /**
     * @brief Get W of NCHW tensor. If placeholder used and no BS defined - exception
     */
    size_t getWidth(const Name& tensorName) const;

    /**
     * @brief Check if placeholder used for W
     *
     * @todo(ck): rename function, there is no verb `placehold`
     */
    bool isWidthPlaceholded(const Name& tensorName) const;

    WShape getShape(const Name& tensorName) const;

    /**
     * @brief Get batch size assigned to workflow
     */
    size_t getBatchSize() const;

    /**
     * @brief Assign batch size to workflow
     * Create tensors and allocate memory for not optimized batched tensors, create shapes for optimized batched tensors
     */
    void setBatchSize(size_t batchSize);

    /**
     * @brief Execute forward test pipeline
     */
    virtual void forwardPassTesting();

    /**
     * @brief Execute forward train pipeline
     *
     * @param performZero true/false, global override of zeroing for not optimizable tensors in terms of memory (typically, gradients for weights), param is useful for microbatching mode
     */
    virtual void forwardPassTraining(bool performZero = true);

    /**
     * @brief Execute backward train pipeline
     */
    virtual void backwardPassTraining();

    /**
     * @brief Select checkpointed tensors among activations (layers outputs)
     */
    void setCheckpoints(const Names& checkpoints);

    /**
     * @brief Get list of activations, available for checkpointing (not ordered)
     */
    Names getPotentialCheckpoints() const;

    MemoryManager& getMemoryManager() { return mMemoryManager; }
    template<typename MM>
    MM& getMemoryManager();

    const MemoryManager& getMemoryManager() const { return mMemoryManager; }
    template<typename MM>
    const MM& getMemoryManager() const;

    NetworkParameters& getNetworkParameters() { return mNetworkParameters; }
    const NetworkParameters& getNetworkParameters() const { return mNetworkParameters; }

    AllocationMode getAllocationMode() const { return mAllocationMode; }

    ExecutionTarget getExecutionTarget() const { return mExecutionTarget; }

    /**
     * Override LayerExecutionTarget for further added layers. Memory manager and implementation behavior will be altered.
     * Use together with ConvertPrecisionLayer
     */
    void overrideLayerExecutionTarget(LayerExecutionTarget layerExecutionTarget);
    void resetLayerExecutionTargetOverride() { mOverridedLayerExecutionTarget = LayerExecutionTarget::Default; }
    LayerExecutionTarget getOverrideLayerExecutionTarget() const { return mOverridedLayerExecutionTarget; }

    const BasicLayer* operator[](const raul::Name& name) const;
    BasicLayer* operator[](const raul::Name& name);

    void printInfo(std::ostream& stream) const;

    std::unordered_set<std::string> getSetOfLayers() const;

    const Pipeline& getPipeline(Pipelines pipeline) const;

    void addCallback(const Name& layerName, WorkflowListener& listener);
    // used by actions
    typedef std::vector<WorkflowListener*> Listeners;
    Listeners getListeners(const Name& layerName) const;

    cl::CommandQueue& getGpuCommandQueue();

    const OpenCLKernelManager& getKernelManager() const { return mKernelManager; }
    OpenCLKernelManager& getKernelManager() { return mKernelManager; }

    void setScaling(const Name& layerName, ScalingStrategy scaling) { mScalingStrategies.insert({ layerName, scaling }); }

  protected:
    /**
     * @brief Add layer explicitly
     *
     * @param layer shared pointer to layer
     */
    void addLayer(LayerMem layer);

    template<typename T, typename... Ts>
    void alterLayerExecutionTargetInParam(T& arg, Ts... args)
    {
        (void)arg;

        if constexpr (std::is_base_of<BasicParams, T>::value)
        {
            alterBasicParam(arg);
        }

        alterLayerExecutionTargetInParam(args...);
    }

    template<typename T>
    void alterLayerExecutionTargetInParam(T& arg)
    {
        (void)arg;

        if constexpr (std::is_base_of<BasicParams, T>::value)
        {
            alterBasicParam(arg);
        }
    }

    void alterBasicParam(BasicParams& param);

    /**
     * @brief Fill a list of dangling input tensors
     *
     * This is an auxiliary protection procedure.
     */
    void fillExternalInputs();

    /**
     * @brief Check a feature of incorrect graph
     *
     * Graph is incorrect if there is at least one dangling input tensor.
     *
     * @return true/false
     */
    [[nodiscard]] bool isGraphCorrected() const { return mExternalInputs.empty(); }

    NameSet mExternalInputs;

    bool checkOutputsNeeded(const BasicLayer* layer) const;

    [[nodiscard]] size_t getDimension(const Name& tensorName, size_t dim) const;
    [[nodiscard]] bool isDimensionPlaceholded(const Name& tensorName, size_t dim) const;

    MemoryManager mMemoryManager;
    MemoryManagerFP16 mMemoryManagerFP16;
    MemoryManagerGPU mMemoryManagerGPU;
    NetworkParameters mNetworkParameters;

    const AllocationMode mAllocationMode;

    const ExecutionTarget mExecutionTarget;
    LayerExecutionTarget mOverridedLayerExecutionTarget;

    std::shared_ptr<WorkflowDB> mWorkflowDB;
    std::shared_ptr<WorkflowPool<MemoryManager>> mWorkflowPoolTest;
    std::shared_ptr<WorkflowPool<MemoryManager>> mWorkflowPoolTrain;
    std::shared_ptr<WorkflowPoolGPU> mWorkflowPoolTestGPU;
    std::shared_ptr<WorkflowPoolGPU> mWorkflowPoolTrainGPU;
    std::shared_ptr<WorkflowPool<MemoryManagerFP16>> mWorkflowPoolTestFP16;
    std::shared_ptr<WorkflowPool<MemoryManagerFP16>> mWorkflowPoolTrainFP16;

    bool mIsPipelinesPrepared;

    size_t mBatchSize;
    bool mIsBatchSizeInited;

    bool mIsMemoryPrepared;

    bool mIsForwardCalled;

    std::vector<LayerMem> mLayers;                     ///< Ordered list of layers
    std::unordered_map<Name, BasicLayer*> mLayersDict; ///< Layer search dictionary by name

    /**
     * @brief Fill auxiliary pipelines with commands
     *
     */
    void createAuxPipelines();

    /**
     * @brief Fill test pipeline with commands
     *
     * @see execTargetCreateForwardTestPipeline
     *
     */
    void createForwardTestPipeline();

    /**
     * @brief Fill train pipeline with commands
     *
     * @see fillTrainPipeline
     * @see fillTrainPipelineCompression
     *
     */
    void createTrainPipeline();

    /**
     * @brief Check equality of attributes and throw exception if they are not equal
     * @param indexA index in workflow DB of the 1st usage
     * @param indexB index in workflow DB of the 2nd usage
     * @param name tensor name (only for message generation)
     *
     * @note: called from createAuxPipelines()
     */
    void checkAttributesInequality(size_t indexA, size_t indexB, const Name& name) const;

    /**
     * @brief Map of tensor names to their usage interval in layers
     *
     * Tensor name -> [first, last] layer
     *
     * @todo(ck): rename to timelines, create a timeline class to increase readability
     *
     * @note Order is important to simplify checkpointing tests
     *
     * @see Name
     */
    typedef std::map<Name, std::pair<Name, Name>> Timeline;

    /**
     * @brief Tensors collection of the layer [Layer name -> tensor names]
     *
     * @todo(ck): may we move it to layer property?
     *
     * @see Name
     * @see Names
     */
    typedef std::unordered_map<Name, Names> Appearance;

    /**
     * @brief Extract names of tensors
     * @return Names
     */
    [[nodiscard]] Names getLayerNames() const;

    /**
     * @brief Generate timeline (a map of tensor names and their usage interval in layers)
     * @param layers
     * @param usage
     * @return
     *
     * @see Names
     * @see Usage
     */
    [[nodiscard]] Timeline getTimeline(const Names& layers, Usage usage) const;

    /**
     * @brief Generate pair of appearances for first and last layers of each timeline
     *
     * Steps:
     * - Iterate over timeline collection (a timeline per tensor)
     * - Fill two appearances (starting and ending) for layers
     *
     * Appearance describes what tensors are allocated and deallocated on a layer.
     *
     * @param timelines
     * @return pair of appearances
     *
     * @see Timeline
     * @see Appearance
     */
    [[nodiscard]] std::pair<Appearance, Appearance> timelineToAppearance(const Timeline& timelines) const;
    [[nodiscard]] std::tuple<Timeline, Timeline, Timeline> appearanceToTimeline(const std::pair<Appearance, Appearance>& forward, const std::pair<Appearance, Appearance>& backward) const;

    Pipeline mPipelineCreateBatched;
    Pipeline mPipelineCreateNotBatched;
    Pipeline mPipelineDeleteBatched;
    Pipeline mPipelineZeroTensors;

    Pipeline mPipelineForwardTest;
    Pipeline mPipelineForwardTrain;
    Pipeline mPipelineBackwardTrain;

    void executePipeline(Pipelines pipeline, const ActionParam& param = ActionParam()) const;

    void createTrainCheckpointedPipeline();

    Names mCheckpoints;

    [[nodiscard]] std::pair<bool, size_t> findLayerByOutput(const Name& tensor) const; // d.polubotko (TODO): speedup necessary
    [[nodiscard]] bool isCheckpoint(const Name& tensor) const;                         // d.polubotko (TODO): speedup necessary
    [[nodiscard]] bool isPersistent(const Name& tensor) const;

    typedef std::vector<BasicLayer*> Layers;
    [[nodiscard]] Layers findPathFromActivationTillCheckpoint(const Name& tensor, const std::unordered_set<Name>& alreadyFound) const;
    void traverseGraphTillCheckpoint(BasicLayer* layer, Layers& names, const std::unordered_set<Name>& alreadyFound) const;
    void removeDuplications(const Names& tNames, Appearance& appear) const;
    void removeDuplications(const Names& tNames, Appearance& appearA, Appearance& appearB) const;
    void removeDuplications(const std::unordered_set<Name>& tNames, Appearance& appearA, Appearance& appearB) const;

    /**
     * @brief Checker
     *
     * @param names
     * @return true/false
     *
     * @todo(ck): is it a really necessary to this function inside Workflow?
     * @todo(ck): Let's declare set of inputs which has uniqueness as a property.
     */
    [[nodiscard]] bool isUniqueNames(const Names& names) const;

    /**
     * @brief Fill training pipeline
     *
     * @param pipeline
     * @param layer
     * @param first
     * @param last
     * @param usage
     * @param pool
     * @param poolGPU
     * @param poolfp16
     */
    void fillTrainPipeline(Pipeline& pipeline, BasicLayer* layer, const Appearance& first, const Appearance& last, Usage usage);

    void fillTrainPipelineCheckpointed(Pipeline& pipeline,
                                       BasicLayer* layer,
                                       const Appearance& first,
                                       const Appearance& last,
                                       std::shared_ptr<WorkflowPool<MemoryManager>>& pool,
                                       size_t recalcAmount,
                                       const Name& recalcTensor);

    void fillTrainPipelineCompression(Pipeline& pipeline, BasicLayer* layer, const Appearance& appear, Usage usage);

    /**
     * @brief Clean pipelines
     *
     * Delete commands in sequences of actions
     */
    void clearPipelines();

    /**
     * @brief Fill pipelines (mPipelineCreateBatched, mPipelineCreateNotBatched, mPipelineZeroTensors)
     *
     * @tparam MM
     * @tparam CreateShape
     * @tparam CreateTensor
     * @tparam DeleteTensor
     * @tparam Zero
     * @param uniqueTensors
     */
    void execTargetCreateAuxPipelines(const std::unordered_map<Name, size_t>& uniqueTensors);

    /**
     * @brief Fill test pipeline
     *
     * Create a list of action respective to the provided appearances.
     *
     * Steps:
     * - Iterate over layers
     * - Check whether any tensor timeline starts at the layer or not
     * - Do allocate actions for such tensors
     * - Do forward action for the layer
     * - Check whether any tensor timeline ends at the layer or not
     * - Do deallocate actions for such tensors
     *
     * @tparam MM memory manager type
     * @tparam Allocate Allocator pipeline action type
     * @tparam Deallocate Deallocator pipeline action type
     * @tparam Pool Memory pool type
     *
     * @param timelineLayersFirst Appearance of timeline starting layers
     * @param timelineLayersLast Appearance of timeline ending layers
     * @param pool Memory pool
     *
     * @see Appearance
     * @see Pool
     */
    void execTargetCreateForwardTestPipeline(const Appearance& timelineLayersFirst, const Appearance& timelineLayersLast);

    /**
     * @brief Fill train pipeline
     *
     * @tparam MM
     * @tparam Allocate
     * @tparam Deallocate
     * @tparam Zero
     * @tparam Pool
     * @param pipeline
     * @param layer
     * @param first
     * @param last
     * @param usage
     * @param pool
     */
    void execTargetFillTrainPipeline(Pipeline& pipeline, BasicLayer* layer, const Appearance& first, const Appearance& last, Usage usage);

    ActionMem newActionCreateShape(const Name& name, LayerExecutionTarget layerExecutionTarget, const WShape& shape);
    ActionMem newActionCreateTensor(const Name& name, LayerExecutionTarget layerExecutionTarget, const WShape& shape);
    ActionMem newActionDeleteTensor(const Name& name, LayerExecutionTarget layerExecutionTarget);
    ActionMem newActionZero(const Name& name, LayerExecutionTarget layerExecutionTarget);
    ActionMem newActionAllocate(const Name& name, LayerExecutionTarget layerExecutionTarget, bool isTrain);
    ActionMem newActionDeallocate(const Name& name, LayerExecutionTarget layerExecutionTarget);

    std::unordered_map<Name, Listeners> mListeners;

    cl::Platform mGpuPlatform;
    cl::Device mGpuDevice;
    cl::Context mGpuContext;
    cl::CommandQueue mGpuQueue;
    OpenCLKernelManager mKernelManager;

    std::unordered_map<Name, ScalingStrategy> mScalingStrategies;
};

#define DEC_FORW_READ raul::Workflow::Usage::Forward, raul::Workflow::Mode::Read, true, true, false, false, false
#define DEC_FORW_READ_NOMEMOPT raul::Workflow::Usage::Forward, raul::Workflow::Mode::Read, false, false, false, false, false
#define DEC_FORW_WRIT raul::Workflow::Usage::Forward, raul::Workflow::Mode::Write, true, true, false, false, false
#define DEC_FORW_WRIT_COMP raul::Workflow::Usage::Forward, raul::Workflow::Mode::Write, true, true, false, false, params.isCompressOutput()
#define DEC_FORW_WRIT_NOMEMOPT raul::Workflow::Usage::Forward, raul::Workflow::Mode::Write, false, false, false, false, false

#define DEC_BACK_READ raul::Workflow::Usage::Backward, raul::Workflow::Mode::Read, true, true, false, false, false
#define DEC_BACK_READ_COMP raul::Workflow::Usage::Backward, raul::Workflow::Mode::Read, true, true, false, false, params.isCompressOutput()
#define DEC_BACK_READ_NOMEMOPT raul::Workflow::Usage::Backward, raul::Workflow::Mode::Read, false, false, false, false, false
#define DEC_BACK_WRIT raul::Workflow::Usage::Backward, raul::Workflow::Mode::Write, true, true, false, false, false
#define DEC_BACK_WRIT_ZERO raul::Workflow::Usage::Backward, raul::Workflow::Mode::Write, true, true, false, true, false

#define DEC_FRBC_WRIT raul::Workflow::Usage::ForwardAndBackward, raul::Workflow::Mode::Write, true, true, false, false, false
#define DEC_FRBC_READ_NOMEMOPT raul::Workflow::Usage::ForwardAndBackward, raul::Workflow::Mode::Read, false, false, false, false, false
#define DEC_FRBC_WRIT_NOMEMOPT raul::Workflow::Usage::ForwardAndBackward, raul::Workflow::Mode::Write, false, false, false, false, false

#define DEC_TRAINABLE raul::Workflow::Usage::ForwardAndBackward, raul::Workflow::Mode::Read, false, false, !mFrozen, false, false
#define DEC_TRAINABLE_GRAD raul::Workflow::Usage::Backward, raul::Workflow::Mode::Write, false, false, false, true, false

#define NETWORK_PARAMS_DEFINE(varName) [[maybe_unused]] raul::NetworkParameters& varName = work.getNetworkParameters();

#define TENSORS_CREATE(batch_size)                                                                                                                                                                     \
    work.preparePipelines();                                                                                                                                                                           \
    work.setBatchSize(batch_size);                                                                                                                                                                     \
    work.prepareMemoryForTraining();

#define COPY_TO_CPU(name, varGPU, varCPU)                                                                                                                                                              \
    auto varGPU = mNetworkParams.mMemoryManagerGPU[name];                                                                                                                                              \
    Tensor varCPU(varGPU);

#define DECLARE_IMPL(name, typeCPU, typeGPU, typeCPUFP16)                                                                                                                                              \
    if (mNetworkParams.mWorkflow.getExecutionTarget() == ExecutionTarget::CPU && std::string(#typeCPU) != "NotImplemented")                                                                            \
    {                                                                                                                                                                                                  \
        mImpl = std::make_shared<typeCPU>(*this);                                                                                                                                                      \
    }                                                                                                                                                                                                  \
    else if (mNetworkParams.mWorkflow.getExecutionTarget() == ExecutionTarget::GPU && std::string(#typeGPU) != "NotImplemented")                                                                       \
    {                                                                                                                                                                                                  \
        mImpl = std::make_shared<typeGPU>(*this);                                                                                                                                                      \
    }                                                                                                                                                                                                  \
    else if (mNetworkParams.mWorkflow.getExecutionTarget() == ExecutionTarget::CPUFP16 && std::string(#typeCPUFP16) != "NotImplemented")                                                               \
    {                                                                                                                                                                                                  \
        mImpl = std::make_shared<typeCPUFP16>(*this);                                                                                                                                                  \
    }                                                                                                                                                                                                  \
    else                                                                                                                                                                                               \
    {                                                                                                                                                                                                  \
        THROW_NONAME(#name, "unsupported execution target");                                                                                                                                           \
    }

#define INSTANTIATE_IMPL(impl)                                                                                                                                                                         \
    template class impl<MemoryManager>;                                                                                                                                                                \
    template class impl<MemoryManagerFP16>;                                                                                                                                                            \
    template class impl<MemoryManagerGPU>;

} // raul namespace

#endif
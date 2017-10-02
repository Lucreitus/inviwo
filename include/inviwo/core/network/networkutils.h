/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2015-2017 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/

#ifndef IVW_NETWORKUTILS_H
#define IVW_NETWORKUTILS_H

#include <inviwo/core/common/inviwocoredefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/util/stdextensions.h>

namespace inviwo {

namespace util {

namespace detail {

/**
 * Helper class for a connection referring to a port outsize the network it's in
 * used in Copy/Paste for copying subparts of a network but still keeping connections.
 */
struct PartialConnection : public Serializable {
    PartialConnection();
    PartialConnection(std::string path, Inport* inport);
    std::string outportPath_ = "";
    Inport* inport_ = nullptr;

    virtual void serialize(Serializer& s) const override;
    virtual void deserialize(Deserializer& d) override;
};

/**
 * Helper class for a links referring to a properties outsize the network it's in
 * used in Copy/Paste for copying subparts of a network but still keeping links.
 */
struct PartialSrcLink : public Serializable {
    PartialSrcLink();
    PartialSrcLink(Property* src, std::string path);
    Property* src_ = nullptr;
    std::string dstPath_ = "";

    virtual void serialize(Serializer& s) const override;
    virtual void deserialize(Deserializer& d) override;
};

/**
 * Helper class for a links referring to a properties outsize the network it's in
 * used in Copy/Paste for copying subparts of a network but still keeping links.
 */
struct PartialDstLink : public Serializable {
    PartialDstLink();
    PartialDstLink(std::string path, Property* dst);
    std::string srcPath_ = "";
    Property* dst_ = nullptr;

    virtual void serialize(Serializer& s) const override;
    virtual void deserialize(Deserializer& d) override;
};

/**
 * Helper class for Copy/Pasting a network with sub parts referring to stuff outside of the network.
 */
struct PartialProcessorNetwork : public Serializable {
    PartialProcessorNetwork(ProcessorNetwork* network);

    std::vector<Processor*> getAddedProcessors() const;

    virtual void serialize(Serializer& s) const override;
    virtual void deserialize(Deserializer& d) override;

private:
    ProcessorNetwork* network_;
    std::vector<Processor*> addedProcessors_;
};

}  // namespace

struct IVW_CORE_API ProcessorStates {
    bool hasBeenVisited(Processor* processor) const;
    void setProcessorVisited(Processor* processor);
    void clear();

private:
    std::unordered_set<Processor*> visited_;
};

IVW_CORE_API std::unordered_set<Processor*> getDirectPredecessors(Processor* processor);
IVW_CORE_API std::unordered_set<Processor*> getDirectSuccessors(Processor* processor);

IVW_CORE_API std::unordered_set<Processor*> getPredecessors(Processor* processor);
IVW_CORE_API std::unordered_set<Processor*> getSuccessors(Processor* processor);

enum class TraversalDirection {Up, Down};
enum class VisitPattern {Pre, Post};

#include <warn/push>
#include <warn/ignore/constant-conditional>
template <TraversalDirection D, VisitPattern V, typename Func>
void traverseNetwork(ProcessorStates& state, Processor* processor, Func f) {
    if (!state.hasBeenVisited(processor)) {
        state.setProcessorVisited(processor);

        if (V == VisitPattern::Pre) f(processor);

        switch (D) {
            case TraversalDirection::Up: {
                for (auto port : processor->getInports()) {
                    for (auto connectedPort : port->getConnectedOutports()) {
                        traverseNetwork<D, V, Func>(state, connectedPort->getProcessor(), f);
                    }
                }
                break;
            }

            case TraversalDirection::Down: {
                for (auto port : processor->getOutports()) {
                    for (auto connectedPort : port->getConnectedInports()) {
                        traverseNetwork<D, V, Func>(state, connectedPort->getProcessor(), f);
                    }
                }
                break;
            }
        }

        if (V == VisitPattern::Post) f(processor);
    }
}
#include <warn/pop>

IVW_CORE_API std::vector<Processor*> topologicalSort(ProcessorNetwork* network);

struct IVW_CORE_API PropertyDistanceSorter {
    PropertyDistanceSorter();
    void setTarget(const Property* target);
    bool operator()(const Property* a, const Property* b);

private:
    vec2 getPosition(const Property* p);
    vec2 getPosition(const Processor* processor);

    vec2 pos_ = {0, 0};
    std::map<const Property*, vec2> cache_;
};

IVW_CORE_API void autoLinkProcessor(ProcessorNetwork* network, Processor* processor);

IVW_CORE_API void serializeSelected(ProcessorNetwork* network, std::ostream& os,
                                    const std::string& refPath);

// return the appended processors.
IVW_CORE_API std::vector<Processor*> appendDeserialized(ProcessorNetwork* network, std::istream& is,
                                                        const std::string& refPath,
                                                        InviwoApplication* app);

}  // namespace

}  // namespace

#endif  // IVW_NETWORKUTILS_H

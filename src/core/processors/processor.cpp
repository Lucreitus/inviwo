/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2012-2017 Inviwo Foundation
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

#include <inviwo/core/processors/processor.h>
#include <inviwo/core/metadata/metadatafactory.h>
#include <inviwo/core/metadata/processormetadata.h>
#include <inviwo/core/interaction/interactionhandler.h>
#include <inviwo/core/interaction/events/event.h>
#include <inviwo/core/interaction/events/interactionevent.h>
#include <inviwo/core/interaction/events/pickingevent.h>
#include <inviwo/core/processors/processorwidget.h>
#include <inviwo/core/util/factory.h>
#include <inviwo/core/util/stdextensions.h>
#include <inviwo/core/util/utilities.h>
#include <inviwo/core/ports/imageport.h>

namespace inviwo {

std::unordered_set<std::string> Processor::usedIdentifiers_;

Processor::Processor()
    : PropertyOwner()
    , ProcessorObservable()
    , processorWidget_(nullptr)
    , identifier_("")
    , network_(nullptr) {
    createMetaData<ProcessorMetaData>(ProcessorMetaData::CLASS_IDENTIFIER);
}

Processor::~Processor() {
    usedIdentifiers_.erase(util::stripIdentifier(identifier_));
}

void Processor::addPort(Inport* port, const std::string& portGroup) {
    if (getPort(port->getIdentifier()) != nullptr) {
        throw Exception("Processor \"" + getIdentifier() + "\" Can't add inport, identifier \"" +
                        port->getIdentifier() + "\" already exist.",
                        IvwContext);
    }
    if (port->getIdentifier().empty()) {
        throw Exception("Adding port with empty identifier", IvwContext);
    }
    util::validateIdentifier(port->getIdentifier(), "Port", IvwContext);
    
    port->setProcessor(this);
    inports_.push_back(port);
    addPortToGroup(port, portGroup);

    notifyObserversProcessorPortAdded(this, port);
}

void Processor::addPort(Inport& port, const std::string& portGroup) { addPort(&port, portGroup); }
void Processor::addPort(std::unique_ptr<Inport> port, const std::string& portGroup) {
    addPort(port.get(), portGroup);
    ownedInports_.push_back(std::move(port));
}

void Processor::addPort(Outport* port, const std::string& portGroup) {
    if (getPort(port->getIdentifier()) != nullptr) {
        throw Exception("Processor \"" + getIdentifier() + "\" Can't add outport, identifier \"" +
                        port->getIdentifier() + "\" already exist.",
                        IvwContext);
    }
    if (port->getIdentifier().empty()) {
        throw Exception("Adding port with empty identifier", IvwContext);
    }
    util::validateIdentifier(port->getIdentifier(), "Port", IvwContext);

    port->setProcessor(this);
    outports_.push_back(port);
    addPortToGroup(port, portGroup);

    notifyObserversProcessorPortAdded(this, port);
}

void Processor::addPort(Outport& port, const std::string& portDependencySet) {
    addPort(&port, portDependencySet);
}

void Processor::addPort(std::unique_ptr<Outport> port, const std::string& portGroup) {
    addPort(port.get(), portGroup);
    ownedOutports_.push_back(std::move(port));
}

Port* Processor::removePort(const std::string& identifier) {
    if (auto port = getPort(identifier)) {
        if (auto inport = dynamic_cast<Inport*>(port)) return removePort(inport);
        if (auto outport = dynamic_cast<Outport*>(port)) return removePort(outport);
    }
    return nullptr;
}

Inport* Processor::removePort(Inport* port) {   
    notifyObserversProcessorPortRemoved(this, port);
    port->setProcessor(nullptr);
    util::erase_remove(inports_, port);
    removePortFromGroups(port);

    // This will delete the port if owned
    util::erase_remove_if(ownedInports_, [&port](const std::unique_ptr<Inport>& p) {
        if (p.get() == port) {
            port = nullptr;
            return true;
        } else {
            return false;
        }
    });

    return port;
}

Outport* Processor::removePort(Outport* port) {
    notifyObserversProcessorPortRemoved(this, port);
    port->setProcessor(nullptr);
    util::erase_remove(outports_, port);
    removePortFromGroups(port);

    // This will delete the port if owned
    util::erase_remove_if(ownedOutports_, [&port](const std::unique_ptr<Outport>& p) {
        if (p.get() == port) {
            port = nullptr;
            return true;
        } else {
            return false;
        }
    });

    return port;
}

void Processor::addPortToGroup(Port* port, const std::string& portGroup) {
    portGroups_[port] = portGroup;
    groupPorts_[portGroup].push_back(port);
}

void Processor::removePortFromGroups(Port* port) {
    auto group = portGroups_[port];
    util::erase_remove(groupPorts_[group], port);
    if(groupPorts_[group].empty()) groupPorts_.erase(group);
    portGroups_.erase(port);
    
}

std::string Processor::setIdentifier(const std::string& identifier) {
    if (identifier == identifier_) return identifier_;  // nothing changed


    util::validateIdentifier(identifier, "Processor", IvwContext, " ()=&");
    
    usedIdentifiers_.erase(util::stripIdentifier(identifier_));  // remove old identifier

    std::string baseIdentifier = identifier;
    std::string newIdentifier = identifier;
    int i = 2;

    auto parts = splitString(identifier, ' ');
    if (parts.size() > 1 &&
        util::all_of(parts.back(), [](const char& c) { return std::isdigit(c); })) {
        i = std::stoi(parts.back());
        baseIdentifier = joinString(parts.begin(), parts.end() - 1, " ");
        newIdentifier = baseIdentifier + " " + toString(i);
    }

    std::string stripedIdentifier = util::stripIdentifier(newIdentifier);

    while (usedIdentifiers_.find(stripedIdentifier) != usedIdentifiers_.end()) {
        newIdentifier = baseIdentifier + " " + toString(i++);
        stripedIdentifier = util::stripIdentifier(newIdentifier);
    }

    usedIdentifiers_.insert(stripedIdentifier);
    identifier_ = newIdentifier;

    notifyObserversIdentifierChange(this);
    return identifier_;
}

std::string Processor::getIdentifier() {
    if (identifier_.empty()) setIdentifier(getDisplayName());
    return identifier_;
}

void Processor::setProcessorWidget(std::unique_ptr<ProcessorWidget> processorWidget) {
    processorWidget_ = std::move(processorWidget);
}

ProcessorWidget* Processor::getProcessorWidget() const { return processorWidget_.get(); }

bool Processor::hasProcessorWidget() const { return (processorWidget_ != nullptr); }

void Processor::setNetwork(ProcessorNetwork* network) { network_ = network; }

Port* Processor::getPort(const std::string& identifier) const {
    for (auto port : inports_) if (port->getIdentifier() == identifier) return port;
    for (auto port : outports_) if (port->getIdentifier() == identifier) return port;
    return nullptr;
}

Inport* Processor::getInport(const std::string& identifier) const {
    for (auto port : inports_) if (port->getIdentifier() == identifier) return port;
    return nullptr;
}

Outport* Processor::getOutport(const std::string& identifier) const {
    for (auto port : outports_) if (port->getIdentifier() == identifier) return port;
    return nullptr;
}

const std::vector<Inport*>& Processor::getInports() const { return inports_; }

const std::vector<Outport*>& Processor::getOutports() const { return outports_; }

const std::string& Processor::getPortGroup(Port* port) const {
    auto it = portGroups_.find(port);
    if (it != portGroups_.end()) {
        return it->second;
    } else {
        throw Exception("Can't find group for port: \"" + port->getIdentifier() + "\".",
                        IvwContext);
    }
}

std::vector<std::string> Processor::getPortGroups() const {
    std::vector<std::string> groups;
    for (const auto& item : groupPorts_) {
        groups.push_back(item.first);
    }
    return groups;
}

const std::vector<Port*>& Processor::getPortsInGroup(const std::string& portGroup) const {
    auto it = groupPorts_.find(portGroup);
    if (it != groupPorts_.end()) {
        return it->second;
    } else {
        throw Exception("Can't find port group: \"" + portGroup  + "\".", IvwContext);
    }
}

const std::vector<Port*>& Processor::getPortsInSameGroup(Port* port) const {
    return getPortsInGroup(getPortGroup(port));
}

void Processor::invalidate(InvalidationLevel invalidationLevel, Property* modifiedProperty) {
    notifyObserversInvalidationBegin(this);
    PropertyOwner::invalidate(invalidationLevel, modifiedProperty);
    if (!isValid()) {
        for (auto& port : outports_) port->invalidate(InvalidationLevel::InvalidOutput);
    }
    notifyObserversInvalidationEnd(this);

    if (!isValid() && isSink()) {
        performEvaluateRequest();
    }
}

bool Processor::isSource() const { return inports_.empty(); }

bool Processor::isSink() const { return outports_.empty(); }

bool Processor::isReady() const { return allInportsAreReady(); }

bool Processor::allInportsAreReady() const {
    return util::all_of(
        inports_, [](Inport* p) { return (p->isOptional() && !p->isConnected()) || p->isReady(); });
}


bool Processor::allInportsConnected() const {
    return util::all_of(inports_, [](Inport* p) { return p->isConnected(); });
}

void Processor::addInteractionHandler(InteractionHandler* interactionHandler) {
    util::push_back_unique(interactionHandlers_, interactionHandler);
}

void Processor::removeInteractionHandler(InteractionHandler* interactionHandler) {
    util::erase_remove(interactionHandlers_, interactionHandler);
}

bool Processor::hasInteractionHandler() const { return !interactionHandlers_.empty(); }

const std::vector<InteractionHandler*>& Processor::getInteractionHandlers() const {
    return interactionHandlers_;
}

void Processor::serialize(Serializer& s) const {
    s.serialize("type", getClassIdentifier(), SerializationTarget::Attribute);
    s.serialize("identifier", identifier_, SerializationTarget::Attribute);

    s.serialize("InteractonHandlers", interactionHandlers_, "InteractionHandler");

    std::map<std::string, std::string> portGroups;
    for (auto& item : portGroups_) portGroups[item.first->getIdentifier()] = item.second;
    s.serialize("PortGroups", portGroups, "PortGroup");

    auto ownedInportIds = util::transform(
        ownedInports_, [](const std::unique_ptr<Inport>& p) { return p->getIdentifier(); });
    s.serialize("OwnedInportIdentifiers", ownedInportIds, "InportIdentifier");

    auto ownedOutportIds = util::transform(
        ownedOutports_, [](const std::unique_ptr<Outport>& p) { return p->getIdentifier(); });
    s.serialize("OwnedOutportIdentifiers", ownedOutportIds, "OutportIdentifier");

    s.serialize("InPorts", inports_, "InPort");
    s.serialize("OutPorts", outports_, "OutPort");

    PropertyOwner::serialize(s);
    MetaDataOwner::serialize(s); 
}

void Processor::deserialize(Deserializer& d) {
    std::string identifier;
    d.deserialize("identifier", identifier, SerializationTarget::Attribute);
    setIdentifier(identifier);  // Need to use setIdentifier to make sure we get a unique id.

    d.deserialize("InteractonHandlers", interactionHandlers_, "InteractionHandler");

    std::map<std::string, std::string> portGroups;
    d.deserialize("PortGroups", portGroups, "PortGroup");

    {
        std::vector<std::string> ownedInportIds;
        d.deserialize("OwnedInportIdentifiers", ownedInportIds, "InportIdentifier");

        auto desInports =
            util::IdentifiedDeserializer<std::string, Inport*>("InPorts", "InPort")
                .setGetId([](Inport* const& port) { return port->getIdentifier(); })
                .setMakeNew([]() { return nullptr; })
                .setNewFilter([&](const std::string& id, size_t /*ind*/) {
                    return util::contains(ownedInportIds, id);
                })
                .onNew([&](Inport*& port) {
                    addPort(std::unique_ptr<Inport>(port), portGroups[port->getIdentifier()]);
                })
                .onRemove([&](const std::string& id) {
                    if (util::contains_if(ownedInports_, [&](std::unique_ptr<Inport>& op) {
                            return op->getIdentifier() == id;
                        })) {
                        delete removePort(id);
                    }
                });

        desInports(d, inports_);
    }
    {
        std::vector<std::string> ownedOutportIds;
        d.deserialize("OwnedOutportIdentifiers", ownedOutportIds, "OutportIdentifier");

        auto desOutports =
            util::IdentifiedDeserializer<std::string, Outport*>("OutPorts", "OutPort")
                .setGetId([](Outport* const& port) { return port->getIdentifier(); })
                .setMakeNew([]() { return nullptr; })
                .setNewFilter([&](const std::string& id, size_t /*ind*/) {
                    return util::contains(ownedOutportIds, id);
                })
                .onNew([&](Outport*& port) {
                    addPort(std::unique_ptr<Outport>(port), portGroups[port->getIdentifier()]);
                })
                .onRemove([&](const std::string& id) {
                    if (util::contains_if(ownedOutports_, [&](std::unique_ptr<Outport>& op) {
                            return op->getIdentifier() == id;
                        })) {
                        delete removePort(id);
                    }
                });

        desOutports(d, outports_);
    }

    PropertyOwner::deserialize(d);
    MetaDataOwner::deserialize(d);
}

void Processor::setValid() {
    PropertyOwner::setValid();
    for (auto inport : inports_) inport->setChanged(false);
    for (auto outport : outports_) outport->setValid();
}

void Processor::performEvaluateRequest() { notifyObserversRequestEvaluate(this); }

void Processor::invokeEvent(Event* event) {
    if (event->hash() == PickingEvent::chash()) {
        static_cast<PickingEvent*>(event)->invoke(this);
    }
    if (event->hasBeenUsed()) return;
    
    PropertyOwner::invokeEvent(event);
    if (event->hasBeenUsed()) return;
    
    for (auto elem : interactionHandlers_) elem->invokeEvent(event);
}

void Processor::propagateEvent(Event* event, Outport* source) {
    if (event->hasVisitedProcessor(this)) return;
    event->markAsVisited(this);
    
    invokeEvent(event);
    if (event->hasBeenUsed()) return;

    bool used = event->hasBeenUsed();
    for (auto inport : getInports()) {
        if (event->shouldPropagateTo(inport, this, source)) {
            inport->propagateEvent(event);
            used |= event->hasBeenUsed();
            event->markAsUnused();
        }
    }
    if (used) event->markAsUsed();
}

const std::string Processor::getCodeStateString(CodeState state) {
    std::stringstream ss;
    ss << state;
    return ss.str();
}

std::vector<std::string> Processor::getPath() const {
    std::vector<std::string> path;
    path.push_back(util::stripIdentifier(identifier_));
    return path;
}

}  // namespace

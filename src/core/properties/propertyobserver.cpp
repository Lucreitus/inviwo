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

#include <inviwo/core/properties/propertyobserver.h>

namespace inviwo {

void PropertyObservable::notifyObserversOnSetIdentifier(const std::string& identifier) {
    forEachObserver([&](PropertyObserver* o) { o->onSetIdentifier(identifier); });
}

void PropertyObservable::notifyObserversOnSetDisplayName(const std::string& displayName) {
    forEachObserver([&](PropertyObserver* o) { o->onSetDisplayName(displayName); });
}

void PropertyObservable::notifyObserversOnSetSemantics(const PropertySemantics& semantics) {
    forEachObserver([&](PropertyObserver* o) { o->onSetSemantics(semantics); });
}

void PropertyObservable::notifyObserversOnSetReadOnly(bool readonly) {
    forEachObserver([&](PropertyObserver* o) { o->onSetReadOnly(readonly); });
}

void PropertyObservable::notifyObserversOnSetVisible(bool visible) {
    forEachObserver([&](PropertyObserver* o) { o->onSetVisible(visible); });
}

void PropertyObservable::notifyObserversOnSetUsageMode(UsageMode usageMode) {
    forEachObserver([&](PropertyObserver* o) { o->onSetUsageMode(usageMode); });
}

void PropertyObserver::onSetIdentifier(const std::string&) {}

void PropertyObserver::onSetDisplayName(const std::string&) {}

void PropertyObserver::onSetSemantics(const PropertySemantics&) {}

void PropertyObserver::onSetReadOnly(bool) {}

void PropertyObserver::onSetVisible(bool) {}

void PropertyObserver::onSetUsageMode(UsageMode) {}

}  // namespace

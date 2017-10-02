/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2014-2017 Inviwo Foundation
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

#include <inviwo/qt/editor/linkdialog/linkdialogscene.h>
#include <inviwo/qt/editor/linkdialog/linkdialoggraphicsitems.h>
#include <inviwo/qt/editor/linkdialog/linkdialogprocessorgraphicsitems.h>
#include <inviwo/qt/editor/linkdialog/linkdialogpropertygraphicsitems.h>
#include <inviwo/qt/editor/linkdialog/linkdialogcurvegraphicsitems.h>

#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/network/processornetworkobserver.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/propertyowner.h>
#include <inviwo/core/properties/property.h>
#include <inviwo/core/links/propertylink.h>
#include <inviwo/core/util/stdextensions.h>
#include <inviwo/qt/editor/linkdialog/linkdialoggraphicsitems.h>

#include <warn/push>
#include <warn/ignore/all>
#include <QTimeLine>
#include <QMenu>
#include <QAction>
#include <warn/pop>

namespace inviwo {

LinkDialogGraphicsScene::LinkDialogGraphicsScene(QWidget* parent, ProcessorNetwork* network,
                                                 Processor* srcProcessor, Processor* dstProcessor)
    : QGraphicsScene(parent)
    , currentScrollSteps_(0)
    , linkCurve_(nullptr)
    , startProperty_(nullptr)
    , endProperty_(nullptr)
    , srcProcessor_(nullptr)
    , dstProcessor_(nullptr)
    , network_(network)
    , expandProperties_(false)
    , mouseOnLeftSide_(false) {
    // The default bsp tends to crash...
    setItemIndexMethod(QGraphicsScene::NoIndex);

    network_->addObserver(this);

    srcProcessor_ =
        new LinkDialogProcessorGraphicsItem(LinkDialogTreeItem::Side::Left, srcProcessor);
    dstProcessor_ =
        new LinkDialogProcessorGraphicsItem(LinkDialogTreeItem::Side::Right, dstProcessor);

    float xPos1 = linkdialog::dialogWidth / 10.0 + linkdialog::processorWidth/2;
    addItem(srcProcessor_);
    srcProcessor_->setPos(QPointF(xPos1, 2*linkdialog::processorHeight));
    srcProcessor_->show();

    float xPos2 = linkdialog::dialogWidth * 9.0 / 10.0 - linkdialog::processorWidth/2;
    addItem(dstProcessor_);
    dstProcessor_->setPos(QPointF(xPos2, 2*linkdialog::processorHeight));
    dstProcessor_->show();


    std::function<void(LinkDialogPropertyGraphicsItem*)> buildCache = [this, &buildCache](
        LinkDialogPropertyGraphicsItem* item) {
        propertyMap_[item->getItem()] = item;
        for (auto i : item->getSubPropertyItemList()) {
            buildCache(i);
        }
    };

    for (auto item : srcProcessor_->getPropertyItemList()) buildCache(item);
    for (auto item : dstProcessor_->getPropertyItemList()) buildCache(item);

    // add links
    auto links = network_->getLinksBetweenProcessors(srcProcessor, dstProcessor);
    for (auto& link : links) {
        initializePropertyLinkRepresentation(link);
    }
    update();

}

LinkDialogGraphicsScene::~LinkDialogGraphicsScene() {
    network_->removeObserver(this);

    // We need to make sure to delete the links first. since they refer to the properties
    for (auto& elem : connections_) {
        removeItem(elem);
        delete elem;
    }
    connections_.clear();
}

LinkDialogPropertyGraphicsItem* LinkDialogGraphicsScene::getPropertyGraphicsItemOf(
    Property* property) const {
    return util::map_find_or_null(propertyMap_, property);
}

ProcessorNetwork* LinkDialogGraphicsScene::getNetwork() const { return network_; }

void LinkDialogGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent* e) {
    auto tempProperty = getSceneGraphicsItemAt<LinkDialogPropertyGraphicsItem>(e->scenePos());

    if (!startProperty_ && tempProperty) {
        startProperty_ = tempProperty;

        if (linkCurve_) {
            linkCurve_->hide();
            removeItem(linkCurve_);
            delete linkCurve_;
            linkCurve_ = nullptr;
        }

        QPointF center = startProperty_->getConnectionPoint();
        linkCurve_ = new DialogCurveGraphicsItem(center, e->scenePos());
        linkCurve_->setZValue(linkdialog::connectionDragDepth);
        addItem(linkCurve_);
        linkCurve_->show();
        e->accept();
    } else {
        QGraphicsScene::mousePressEvent(e);
    }
}

void LinkDialogGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent* e) {
    QPointF pos = e->scenePos();

    

    if (pos.x() > linkdialog::dialogWidth / 2.0) {
        mouseOnLeftSide_ = false;
    } else {
        mouseOnLeftSide_ = true;
    }

    if (linkCurve_) {
        QPointF center = startProperty_->getConnectionPoint();
        linkCurve_->setStartPoint(center);
        linkCurve_->setEndPoint(pos);
        e->accept();
    } else {
        QGraphicsScene::mouseMoveEvent(e);
    }
}

void LinkDialogGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* e) {
    if (linkCurve_) {
        delete linkCurve_;
        linkCurve_ = nullptr;
        endProperty_ = getSceneGraphicsItemAt<LinkDialogPropertyGraphicsItem>(e->scenePos());

        if (endProperty_ && (endProperty_ != startProperty_)) {
            Property* sProp = startProperty_->getItem();
            Property* eProp = endProperty_->getItem();

            if (sProp->getOwner()->getProcessor() != eProp->getOwner()->getProcessor()) {
                bool src2dst = SimpleCondition::canLink(sProp, eProp);
                bool dst2src = SimpleCondition::canLink(eProp, sProp);
                if (src2dst && dst2src) {
                    addPropertyLink(sProp, eProp, true);
                } else if (src2dst) {
                    addPropertyLink(sProp, eProp, false);
                } else if (dst2src) {
                    addPropertyLink(eProp, sProp, false);
                }
            }
        }

        startProperty_ = nullptr;
        endProperty_ = nullptr;
        e->accept();
    } else {
        QGraphicsScene::mouseReleaseEvent(e);
    }

    startProperty_ = nullptr;
    endProperty_ = nullptr;
}

void LinkDialogGraphicsScene::keyPressEvent(QKeyEvent* keyEvent) {
    if (keyEvent->key() == Qt::Key_Delete) {
        keyEvent->accept();
        QList<QGraphicsItem*> selectedGraphicsItems = selectedItems();

        for (auto& selectedGraphicsItem : selectedGraphicsItems) {
            if (auto item =
                    qgraphicsitem_cast<DialogConnectionGraphicsItem*>(selectedGraphicsItem)) {
                removePropertyLink(item);
                removePropertyLinkRepresentation(item->getPropertyLink());
            }
        }
    } else if (keyEvent->key() == Qt::Key_Escape) {
        emit closeDialog();    
    }

    QGraphicsScene::keyPressEvent(keyEvent);
}

void LinkDialogGraphicsScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* e) {
    if (auto linkGraphicsItem =
            getSceneGraphicsItemAt<DialogConnectionGraphicsItem>(e->scenePos())) {
        QMenu menu;
        QAction* deleteAction = menu.addAction("Delete");
        QAction* biDirectionAction = menu.addAction("Bi Directional");
        biDirectionAction->setCheckable(true);
        QAction* switchDirection = menu.addAction("Switch Direction");

        if (isPropertyLinkBidirectional(linkGraphicsItem)) {
            biDirectionAction->setChecked(true);
        } else {
            biDirectionAction->setChecked(false);
        }

        QAction* result = menu.exec(QCursor::pos());

        if (result == deleteAction) {
            removePropertyLink(linkGraphicsItem);
            removePropertyLinkRepresentation(linkGraphicsItem->getPropertyLink());
        } else if (result == biDirectionAction) {
            if (biDirectionAction->isChecked()) {
                makePropertyLinkBidirectional(linkGraphicsItem, true);
            } else {
                makePropertyLinkBidirectional(linkGraphicsItem, false);
            }
        } else if (result == switchDirection) {
            switchPropertyLinkDirection(linkGraphicsItem);
        }
    }
}

void LinkDialogGraphicsScene::wheelAction(float offset) {
    currentScrollSteps_ = offset;

    QTimeLine* anim = new QTimeLine(750, this);
    anim->setUpdateInterval(20);
    connect(anim, SIGNAL(valueChanged(qreal)), SLOT(executeTimeLine(qreal)));
    connect(anim, SIGNAL(finished()), SLOT(terminateTimeLine()));
    anim->start();
}

void LinkDialogGraphicsScene::offsetItems(float yIncrement, bool scrollLeft) {
    auto proc = scrollLeft ? srcProcessor_ : dstProcessor_;

    auto rect = proc->rect() | proc->childrenBoundingRect();
    auto procTop = proc->scenePos().y();
    auto procBottom = proc->scenePos().y() + rect.height();
    
    auto view = views().front();
    auto top = static_cast<qreal>(2*linkdialog::processorHeight);
    auto bottom = view->mapToScene(QPoint(0, view->rect().height())).y();

    if ((yIncrement > 0 && procTop + yIncrement < top) ||
        (yIncrement < 0 && procBottom + yIncrement > bottom)) {
        proc->setPos(proc->scenePos().x(), proc->scenePos().y() + yIncrement);
    }
    return;
}

void LinkDialogGraphicsScene::executeTimeLine(qreal /*x*/) {
    float yIncrement = linkdialog::processorHeight * (0.09f) * (currentScrollSteps_);
    offsetItems(yIncrement, mouseOnLeftSide_);
}

void LinkDialogGraphicsScene::terminateTimeLine() { delete sender(); }

void LinkDialogGraphicsScene::addPropertyLink(Property* sProp, Property* eProp,
                                              bool bidirectional) {
    network_->addLink(sProp, eProp);
    network_->evaluateLinksFromProperty(sProp);

    if (bidirectional) network_->addLink(eProp, sProp);
}

void LinkDialogGraphicsScene::showHidden(bool val) {
    showHidden_ = val;

    LinkDialogTreeItem* item = srcProcessor_;
    while (item) {
        item->updatePositions();
        item = item->next();
    }
    
    item = dstProcessor_;
    while (item) {
        item->updatePositions();
        item = item->next();
    }
}

bool LinkDialogGraphicsScene::isShowingHidden() const {return showHidden_;}

void LinkDialogGraphicsScene::toggleExpand() {
    expandProperties_ = !expandProperties_;

    LinkDialogTreeItem* item = srcProcessor_;
    while (item) {
        item->setExpanded(expandProperties_);
        item->updatePositions();
        item = item->next();
    }
    
    item = dstProcessor_;
    while (item) {
        item->setExpanded(expandProperties_);
        item->updatePositions();
        item = item->next();
    }
}

bool LinkDialogGraphicsScene::isPropertyExpanded(Property* property) const {
    if (auto propItem = getPropertyGraphicsItemOf(property)) return propItem->isExpanded();
    return false;
}

void LinkDialogGraphicsScene::removeAllPropertyLinks() {
    std::vector<DialogConnectionGraphicsItem*> tempList = connections_;

    for (auto propertyLink : tempList) {
        removePropertyLink(propertyLink);
    }
}

void LinkDialogGraphicsScene::removePropertyLink(DialogConnectionGraphicsItem* propertyLink) {
    Property* start = propertyLink->getStartProperty()->getItem();
    Property* end = propertyLink->getEndProperty()->getItem();

    network_->removeLink(start, end);
    network_->removeLink(end, start);
}

void LinkDialogGraphicsScene::updateAll() {
    for (auto& elem : connections_) elem->updateConnectionDrawing();
    update();
}

bool LinkDialogGraphicsScene::isPropertyLinkBidirectional(
    DialogConnectionGraphicsItem* propertyLink) const {
    LinkDialogPropertyGraphicsItem* startProperty = propertyLink->getStartProperty();
    LinkDialogPropertyGraphicsItem* endProperty = propertyLink->getEndProperty();

    return network_->isLinkedBidirectional(startProperty->getItem(), endProperty->getItem());
}

void LinkDialogGraphicsScene::makePropertyLinkBidirectional(
    DialogConnectionGraphicsItem* propertyLink, bool isBidirectional) {
    LinkDialogPropertyGraphicsItem* startProperty = propertyLink->getStartProperty();
    LinkDialogPropertyGraphicsItem* endProperty = propertyLink->getEndProperty();

    if (isBidirectional) {
        if (!network_->isLinked(endProperty->getItem(), startProperty->getItem())) {
            network_->addLink(endProperty->getItem(), startProperty->getItem());
        }
    } else {
        if (network_->isLinked(endProperty->getItem(), startProperty->getItem())){
            network_->removeLink(endProperty->getItem(), startProperty->getItem());
        }
    }

    propertyLink->updateConnectionDrawing();
    update();
}

void LinkDialogGraphicsScene::switchPropertyLinkDirection(
    DialogConnectionGraphicsItem* propertyLink) {
    if (!propertyLink->isBidirectional()) {
        const auto link = propertyLink->getPropertyLink();
        network_->removeLink(link.getSource(), link.getDestination());
        network_->addLink(link.getDestination(), link.getSource());
    }
}

DialogConnectionGraphicsItem* LinkDialogGraphicsScene::getConnectionGraphicsItem(
    LinkDialogPropertyGraphicsItem* outProperty, LinkDialogPropertyGraphicsItem* inProperty) const {
    for (auto& elem : connections_) {
        if ((elem->getStartProperty() == outProperty && elem->getEndProperty() == inProperty) ||
            (elem->getStartProperty() == inProperty && elem->getEndProperty() == outProperty))
            return elem;
    }

    return nullptr;
}

DialogConnectionGraphicsItem* LinkDialogGraphicsScene::initializePropertyLinkRepresentation(
    const PropertyLink& propertyLink) {
    auto start = qgraphicsitem_cast<LinkDialogPropertyGraphicsItem*>(
        getPropertyGraphicsItemOf(propertyLink.getSource()));
    auto end = qgraphicsitem_cast<LinkDialogPropertyGraphicsItem*>(
        getPropertyGraphicsItemOf(propertyLink.getDestination()));

    if (start == nullptr || end == nullptr) return nullptr;

    DialogConnectionGraphicsItem* cItem = getConnectionGraphicsItem(start, end);
    if (!cItem) {
        cItem = new DialogConnectionGraphicsItem(start, end, propertyLink);
        addItem(cItem);
        connections_.push_back(cItem);
    }

    cItem->show();

    updateAll();

    return cItem;
}

void LinkDialogGraphicsScene::removePropertyLinkRepresentation(const PropertyLink& propertyLink) {
    auto it = util::find_if(connections_, [&propertyLink](DialogConnectionGraphicsItem* i){
        return i->getPropertyLink() == propertyLink;
    });
    
    if (it != connections_.end()) {
        DialogConnectionGraphicsItem* cItem = *it;

        cItem->hide();
        util::erase_remove(connections_, cItem);

        LinkDialogPropertyGraphicsItem* start = cItem->getStartProperty();
        LinkDialogPropertyGraphicsItem* end = cItem->getEndProperty();

        removeItem(cItem);
        updateAll();
        delete cItem;

        start->prepareGeometryChange();
        end->prepareGeometryChange();
    }
}

void LinkDialogGraphicsScene::onProcessorNetworkDidAddLink(const PropertyLink& propertyLink) {
    initializePropertyLinkRepresentation(propertyLink);
}

void LinkDialogGraphicsScene::onProcessorNetworkDidRemoveLink(const PropertyLink& propertyLink) {
    removePropertyLinkRepresentation(propertyLink);
}

void LinkDialogGraphicsScene::onProcessorNetworkWillRemoveProcessor(Processor* processor) {
    if (processor == srcProcessor_->getItem() || processor == dstProcessor_->getItem()) {
        emit closeDialog();
    }
}

// Manage various tooltips.
void LinkDialogGraphicsScene::helpEvent(QGraphicsSceneHelpEvent* e) {
    QList<QGraphicsItem*> graphicsItems = items(e->scenePos());
    for (auto item : graphicsItems) {
        switch (item->type()) {
            case LinkDialogPropertyGraphicsItem::Type:
                qgraphicsitem_cast<LinkDialogPropertyGraphicsItem*>(item)->showToolTip(e);
                return;

        };
    }
    QGraphicsScene::helpEvent(e);
}


}  // namespace

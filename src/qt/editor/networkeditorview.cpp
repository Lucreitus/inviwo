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


#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/util/settings/linksettings.h>
#include <inviwo/core/util/settings/systemsettings.h>
#include <inviwo/qt/editor/networkeditorview.h>
#include <inviwo/core/network/processornetwork.h>
#include <inviwo/qt/editor/inviwomainwindow.h>


#include <warn/push>
#include <warn/ignore/all>
#include <QMatrix>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <qmath.h>
#include <warn/pop>

namespace inviwo {

NetworkEditorView::NetworkEditorView(NetworkEditor* networkEditor, InviwoMainWindow* parent)
    : QGraphicsView(parent)
    , NetworkEditorObserver()
    , mainwindow_(parent)
    , networkEditor_(networkEditor) {

    NetworkEditorObserver::addObservation(networkEditor_);
    QGraphicsView::setScene(networkEditor_);
    
    setRenderHint(QPainter::Antialiasing, true);
    setMouseTracking(true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    loadHandle_ = mainwindow_->getInviwoApplication()->getWorkspaceManager()->onLoad(
        [this](Deserializer&) { fitNetwork(); });
}

NetworkEditorView::~NetworkEditorView() { QGraphicsView::setScene(nullptr); }

void NetworkEditorView::hideNetwork(bool hide) {
    if (hide) {
        if (scene()) {
            scrollPos_.x = horizontalScrollBar()->value();
            scrollPos_.y = verticalScrollBar()->value();
            QGraphicsView::setScene(nullptr);
        }
    } else {
        if (scene() != networkEditor_) {
            QGraphicsView::setScene(networkEditor_);
            horizontalScrollBar()->setValue(scrollPos_.x);
            verticalScrollBar()->setValue(scrollPos_.y);
        }
    }
}

void NetworkEditorView::mouseDoubleClickEvent(QMouseEvent* e) {
    QGraphicsView::mouseDoubleClickEvent(e);

    if (!e->isAccepted()) {
        fitNetwork();
        e->accept();
    }
}

void NetworkEditorView::resizeEvent(QResizeEvent* e) {
    QGraphicsView::resizeEvent(e);
}

void NetworkEditorView::fitNetwork() {
    const ProcessorNetwork* network = InviwoApplication::getPtr()->getProcessorNetwork();
    if (network) {
        if (network->getProcessors().size() > 0) {
            QRectF br = networkEditor_->itemsBoundingRect().adjusted(-50, -50, 50, 50);
            QSizeF viewsize = size();
            QSizeF brsize = br.size();

            if (brsize.width() < viewsize.width()) {
                br.setLeft(br.left() - 0.5*(viewsize.width() - brsize.width()));
                br.setRight(br.right() + 0.5*(viewsize.width() - brsize.width()));
            }
            if (brsize.height() < viewsize.height()) {
                br.setTop(br.top() - 0.5*(viewsize.height() - brsize.height()));
                br.setBottom(br.bottom() + 0.5*(viewsize.height() - brsize.height()));
            }

            setSceneRect(br);
            fitInView(br, Qt::KeepAspectRatio);
        }
    }
}

void NetworkEditorView::setupAction(std::string tag, bool enable, std::function<void()> fun) {
    auto actions = mainwindow_->getActions();
    auto action = actions[tag];
    if(!connections_[tag]){
        connections_[tag] = connect(action, &QAction::triggered, fun);
    }
    action->setEnabled(enable);
}

void NetworkEditorView::takeDownAction(std::string tag) {
    auto actions = mainwindow_->getActions();
    auto action = actions[tag];
    disconnect(connections_[tag]);
    action->setEnabled(false);
}

void NetworkEditorView::focusInEvent(QFocusEvent* e) {
    auto enable = networkEditor_->selectedItems().size() > 0;

    setupAction("Cut", enable, [&]() {
        auto data = networkEditor_->cut();

        auto mimedata = util::make_unique<QMimeData>();
        mimedata->setData(QString("application/x.vnd.inviwo.network+xml"), data);
        mimedata->setData(QString("text/plain"), data);
        QApplication::clipboard()->setMimeData(mimedata.release());
    });
    
    setupAction("Copy", enable, [&]() {
        auto data = networkEditor_->copy();

        auto mimedata = util::make_unique<QMimeData>();
        mimedata->setData(QString("application/x.vnd.inviwo.network+xml"), data);
        mimedata->setData(QString("text/plain"), data);
        QApplication::clipboard()->setMimeData(mimedata.release());
    });

    setupAction("Paste", true, [&]() {
        auto clipboard = QApplication::clipboard();
        auto mimeData = clipboard->mimeData();
        if (mimeData->formats().contains(QString("application/x.vnd.inviwo.network+xml"))) {
            networkEditor_->paste(mimeData->data(QString("application/x.vnd.inviwo.network+xml")));
        } else if (mimeData->formats().contains(QString("text/plain"))) {
            networkEditor_->paste(mimeData->data(QString("text/plain")));
        }
    });

    setupAction("Delete", enable, [&]() { networkEditor_->deleteSelection(); });

    QGraphicsView::focusInEvent(e);
}

void NetworkEditorView::focusOutEvent(QFocusEvent *e) {
    if(networkEditor_->doingContextMenu()) return;

    setDragMode(QGraphicsView::RubberBandDrag);
    
    takeDownAction("Cut");
    takeDownAction("Copy");
    takeDownAction("Paste");
    takeDownAction("Delete");
    
    QGraphicsView::focusOutEvent(e);
}

void NetworkEditorView::wheelEvent(QWheelEvent* e) {
    QPointF numPixels = e->pixelDelta() / 5.0;
    QPointF numDegrees = e->angleDelta() / 8.0 / 15;
    
    if (e->modifiers() == Qt::ControlModifier) {
        if (!numPixels.isNull()) {
            zoom(qPow(1.025, std::max(-15.0, std::min(15.0, numPixels.y()))));
        } else if (!numDegrees.isNull()) {
            zoom(qPow(1.025, std::max(-15.0, std::min(15.0, numDegrees.y()))));
        }
    } else if (e->modifiers() & Qt::ShiftModifier) {
        // horizontal scrolling
        auto modifiers = e->modifiers();
        // remove the shift key temporarily from the event
        e->setModifiers(e->modifiers() ^ Qt::ShiftModifier);
        horizontalScrollBar()->event(e);
        // restore previous modifiers
        e->setModifiers(modifiers);
    } else {
        QGraphicsView::wheelEvent(e);
    }
    e->accept();
}

void NetworkEditorView::keyPressEvent(QKeyEvent* keyEvent) {
     if (keyEvent->modifiers() & Qt::ControlModifier) {
        setDragMode(QGraphicsView::ScrollHandDrag);
     }
     QGraphicsView::keyPressEvent(keyEvent);
}

void NetworkEditorView::keyReleaseEvent(QKeyEvent* keyEvent) {
    setDragMode(QGraphicsView::RubberBandDrag);
    QGraphicsView::keyReleaseEvent(keyEvent);
}

void NetworkEditorView::zoom(double dz) {
    if ((dz > 1.0 && matrix().m11() > 8.0) || (dz < 1.0 && matrix().m11() < 0.125)) return;
    scale(dz, dz);
}

void NetworkEditorView::onNetworkEditorFileChanged(const std::string& /*newFilename*/) {
    fitNetwork();
}

}  // namespace

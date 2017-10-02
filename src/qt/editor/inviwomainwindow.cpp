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

#include <inviwo/qt/editor/inviwomainwindow.h>

#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/common/inviwocore.h>
#include <inviwo/core/util/commandlineparser.h>
#include <inviwo/core/util/filesystem.h>
#include <inviwo/core/util/settings/systemsettings.h>
#include <inviwo/core/util/utilities.h>
#include <inviwo/core/util/systemcapabilities.h>
#include <inviwo/core/util/vectoroperations.h>
#include <inviwo/core/network/workspacemanager.h>
#include <inviwo/qt/editor/consolewidget.h>
#include <inviwo/qt/editor/helpwidget.h>
#include <inviwo/qt/editor/processorpreview.h>
#include <inviwo/qt/editor/networkeditor.h>
#include <inviwo/qt/editor/networkeditorview.h>
#include <inviwo/qt/editor/processorlistwidget.h>
#include <inviwo/qt/editor/resourcemanagerwidget.h>
#include <inviwo/qt/editor/settingswidget.h>
#include <inviwo/qt/applicationbase/inviwoapplicationqt.h>
#include <modules/qtwidgets/inviwofiledialog.h>
#include <modules/qtwidgets/propertylistwidget.h>
#include <inviwo/core/metadata/processormetadata.h>
#include <inviwo/core/common/inviwomodulefactoryobject.h>
#include <inviwo/core/network/workspaceutils.h>
#include <inviwo/core/network/networklock.h>

#include <inviwomodulespaths.h>

#include <warn/push>
#include <warn/ignore/all>

#include <QScreen>
#include <QStandardPaths>

#include <QActionGroup>
#include <QClipboard>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QUrl>
#include <QVariant>

#include <algorithm>

#include <warn/pop>

namespace inviwo {

InviwoMainWindow::InviwoMainWindow(InviwoApplicationQt* app)
    : QMainWindow()
    , app_(app)
    , networkEditor_(nullptr)
    , appUsageModeProp_(nullptr)
    , snapshotArg_("s", "snapshot",
                   "Specify base name of each snapshot, or \"UPN\" string for processor name.",
                   false, "", "file name")
    , screenGrabArg_("g", "screengrab", "Specify default name of each screen grab.", false, "",
                     "file name")
    , saveProcessorPreviews_("", "save-previews", "Save processor previews to the supplied path",
                             false, "", "path")
    , updateWorkspaces_("", "update-workspaces",
                        "Go through and update all workspaces the the latest versions")
    , undoManager_(this) {

    app_->setMainWindow(this);

    // make sure, tooltips are always shown (this includes port inspectors as well)
    this->setAttribute(Qt::WA_AlwaysShowToolTips, true);

    networkEditor_ = std::make_shared<NetworkEditor>(this);
    // initialize console widget first to receive log messages
    consoleWidget_ = std::make_shared<ConsoleWidget>(this);
    LogCentral::getPtr()->registerLogger(consoleWidget_);
    currentWorkspaceFileName_ = "";

    const QDesktopWidget dw;
    auto screen = dw.screenGeometry(this);
    const float maxRatio = 0.8f;

    QApplication::instance()->installEventFilter(&undoManager_);

    QSize size(1920, 1080);
    size.setWidth(std::min(size.width(), static_cast<int>(screen.width() * maxRatio)));
    size.setHeight(std::min(size.height(), static_cast<int>(screen.height() * maxRatio)));

    // Center Window
    QPoint pos{screen.width() / 2 - size.width() / 2, screen.height() / 2 - size.height() / 2};

    resize(size);
    move(pos);

    app->getCommandLineParser().add(&snapshotArg_,
                                    [this]() {
                                        saveCanvases(app_->getCommandLineParser().getOutputPath(),
                                                     snapshotArg_.getValue());
                                    },
                                    1000);

    app->getCommandLineParser().add(&screenGrabArg_,
                                    [this]() {
                                        getScreenGrab(app_->getCommandLineParser().getOutputPath(),
                                                      screenGrabArg_.getValue());
                                    },
                                    1000);

    app->getCommandLineParser().add(&saveProcessorPreviews_,
                                    [this]() {
                                        utilqt::saveProcessorPreviews(
                                            app_, saveProcessorPreviews_.getValue());

                                    },
                                    1200);

    app->getCommandLineParser().add(&updateWorkspaces_, [this]() { util::updateWorkspaces(app_); },
                                    1250);

    networkEditorView_ = new NetworkEditorView(networkEditor_.get(), this);
    NetworkEditorObserver::addObservation(networkEditor_.get());
    setCentralWidget(networkEditorView_);

    resourceManagerWidget_ = new ResourceManagerWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, resourceManagerWidget_);
    resourceManagerWidget_->hide();

    settingsWidget_ = new SettingsWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, settingsWidget_);
    settingsWidget_->hide();

    helpWidget_ = new HelpWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, helpWidget_);

    processorTreeWidget_ = new ProcessorTreeWidget(this, helpWidget_);
    addDockWidget(Qt::LeftDockWidgetArea, processorTreeWidget_);

    propertyListWidget_ = new PropertyListWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, propertyListWidget_);

    addDockWidget(Qt::BottomDockWidgetArea, consoleWidget_.get());
    // load settings and restore window state
    loadWindowState();

    QSettings settings("Inviwo", "Inviwo");
    settings.beginGroup("mainwindow");
    QString firstWorkspace = filesystem::getPath(PathType::Workspaces, "/boron.inv").c_str();
    workspaceOnLastSuccessfulExit_ =
        settings.value("workspaceOnLastSuccessfulExit", QVariant::fromValue(firstWorkspace))
            .toString();
    settings.setValue("workspaceOnLastSuccessfulExit", "");
    settings.endGroup();
    rootDir_ = QString::fromStdString(filesystem::getPath(PathType::Data));
    workspaceFileDir_ = rootDir_ + "/workspaces";

    // initialize menus
    addActions();
    updateRecentWorkspaceMenu();
}

InviwoMainWindow::~InviwoMainWindow() = default;

void InviwoMainWindow::updateForNewModules() {
    settingsWidget_->updateSettingsWidget();
    processorTreeWidget_->addProcessorsToTree();
    helpWidget_->registerQCHFiles();
    fillExampleWorkspaceMenu();
    fillTestWorkspaceMenu();
}

void InviwoMainWindow::showWindow() {
    if (maximized_)
        showMaximized();
    else
        show();
}

void InviwoMainWindow::saveCanvases(std::string path, std::string fileName) {
    if (path.empty()) path = app_->getPath(PathType::Images);

    repaint();
    app_->processEvents();
    util::saveAllCanvases(app_->getProcessorNetwork(), path, fileName);
}

void InviwoMainWindow::getScreenGrab(std::string path, std::string fileName) {
    if (path.empty()) path = filesystem::getPath(PathType::Images);

    repaint();
    app_->processEvents();
    QPixmap screenGrab = QGuiApplication::primaryScreen()->grabWindow(this->winId());
    screenGrab.save(QString::fromStdString(path + "/" + fileName), "png");
}

void InviwoMainWindow::addActions() {
    auto menu = menuBar();

    auto fileMenuItem = menu->addMenu(tr("&File"));
    auto editMenuItem = menu->addMenu(tr("&Edit"));
    auto viewMenuItem = menu->addMenu(tr("&View"));
    auto evalMenuItem = menu->addMenu(tr("&Evaluation"));
    auto helpMenuItem = menu->addMenu(tr("&Help"));

    auto workspaceToolBar = addToolBar("File");
    workspaceToolBar->setObjectName("fileToolBar");
    auto viewModeToolBar = addToolBar("View");
    viewModeToolBar->setObjectName("viewModeToolBar");
    auto evalToolBar = addToolBar("Evaluation");
    evalToolBar->setObjectName("evalToolBar");

    // file menu entries
    {
        auto newAction = new QAction(QIcon(":/icons/new.png"), tr("&New Workspace"), this);
        actions_["New"] = newAction;
        newAction->setShortcut(QKeySequence::New);
        newAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        this->addAction(newAction);
        connect(newAction, &QAction::triggered, this, &InviwoMainWindow::newWorkspace);
        fileMenuItem->addAction(newAction);
        workspaceToolBar->addAction(newAction);
    }

    {
        auto openAction = new QAction(QIcon(":/icons/open.png"), tr("&Open Workspace"), this);
        openAction->setShortcut(QKeySequence::Open);
        openAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        this->addAction(openAction);
        actions_["Open"] = openAction;
        connect(openAction, &QAction::triggered, this,
                static_cast<void (InviwoMainWindow::*)()>(&InviwoMainWindow::openWorkspace));
        fileMenuItem->addAction(openAction);
        workspaceToolBar->addAction(openAction);
    }

    {
        auto saveAction = new QAction(QIcon(":/icons/save.png"), tr("&Save Workspace"), this);
        saveAction->setShortcut(QKeySequence::Save);
        saveAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        this->addAction(saveAction);
        actions_["Save"] = saveAction;
        connect(saveAction, &QAction::triggered, this,
                static_cast<void (InviwoMainWindow::*)()>(&InviwoMainWindow::saveWorkspace));
        fileMenuItem->addAction(saveAction);
        workspaceToolBar->addAction(saveAction);
    }

    {
        auto saveAsAction =
            new QAction(QIcon(":/icons/saveas.png"), tr("&Save Workspace As"), this);
        saveAsAction->setShortcut(QKeySequence::SaveAs);
        saveAsAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        this->addAction(saveAsAction);
        actions_["Save As"] = saveAsAction;
        connect(saveAsAction, &QAction::triggered, this, &InviwoMainWindow::saveWorkspaceAs);
        fileMenuItem->addAction(saveAsAction);
        workspaceToolBar->addAction(saveAsAction);
    }

    {
        auto workspaceActionSaveAsCopy =
            new QAction(QIcon(":/icons/saveas.png"), tr("&Save Workspace As Copy"), this);
        connect(workspaceActionSaveAsCopy, &QAction::triggered, this,
                &InviwoMainWindow::saveWorkspaceAsCopy);
        fileMenuItem->addAction(workspaceActionSaveAsCopy);
    }

    {
        connect(fileMenuItem->addAction("Save Network Image"), &QAction::triggered,
                [&](bool /*state*/) {
                    InviwoFileDialog saveFileDialog(this, "Save Network Image ...", "image");
                    saveFileDialog.setFileMode(FileMode::AnyFile);
                    saveFileDialog.setAcceptMode(AcceptMode::Save);
                    saveFileDialog.setConfirmOverwrite(true);

                    saveFileDialog.addSidebarPath(PathType::Workspaces);
                    saveFileDialog.addSidebarPath(workspaceFileDir_);

                    saveFileDialog.addExtension("png", "PNG");
                    saveFileDialog.addExtension("jpg", "JPEG");
                    saveFileDialog.addExtension("bmp", "BMP");
                    saveFileDialog.addExtension("pdf", "PDF");

                    if (saveFileDialog.exec()) {
                        QString path = saveFileDialog.selectedFiles().at(0);
                        networkEditor_->saveNetworkImage(path.toStdString());
                        LogInfo("Saved image of network as " << path.toStdString());
                    }

                });
    }

    {
        fileMenuItem->addSeparator();
        auto recentWorkspaceMenu = fileMenuItem->addMenu(tr("&Recent Workspaces"));
        // create placeholders for recent workspaces
        workspaceActionRecent_.resize(maxNumRecentFiles_);
        for (auto& action : workspaceActionRecent_) {
            action = new QAction(this);
            action->setVisible(false);
            recentWorkspaceMenu->addAction(action);
            QObject::connect(action, &QAction::triggered, this,
                             &InviwoMainWindow::openRecentWorkspace);
        }
        // action for clearing the recent file menu
        clearRecentWorkspaces_ = recentWorkspaceMenu->addAction("Clear Recent Workspace List");
        clearRecentWorkspaces_->setEnabled(false);
        QObject::connect(clearRecentWorkspaces_, &QAction::triggered, this,
                         &InviwoMainWindow::clearRecentWorkspaceMenu);
    }

    {
        // create list of all example workspaces
        exampleMenu_ = fileMenuItem->addMenu(tr("&Example Workspaces"));
        fillExampleWorkspaceMenu();
    }

    {
        // TODO: need a DEVELOPER flag here!
        // create list of all test workspaces, inviwo-dev and other external modules, i.e.
        // "research"
        testMenu_ = fileMenuItem->addMenu(tr("&Test Workspaces"));
        fillTestWorkspaceMenu();
    }

    {
        fileMenuItem->addSeparator();
        auto exitAction = new QAction(QIcon(":/icons/button_cancel.png"), tr("&Exit"), this);
        exitAction->setShortcut(QKeySequence::Close);
        connect(exitAction, &QAction::triggered, this, &InviwoMainWindow::close);
        fileMenuItem->addAction(exitAction);
    }

    // Edit
    {
        auto undoAction = undoManager_.getUndoAction();
        actions_["Undo"] = undoAction;
        editMenuItem->addAction(undoAction);
    }
    {
        auto redoAction = undoManager_.getRedoAction();
        actions_["Redo"] = redoAction;
        editMenuItem->addAction(redoAction);
    }

    editMenuItem->addSeparator();

    {
        auto cutAction = new QAction(tr("Cu&t"), this);
        actions_["Cut"] = cutAction;
        cutAction->setShortcut(QKeySequence::Cut);
        editMenuItem->addAction(cutAction);
        cutAction->setEnabled(false);
    }

    {
        auto copyAction = new QAction(tr("&Copy"), this);
        actions_["Copy"] = copyAction;
        copyAction->setShortcut(QKeySequence::Copy);
        editMenuItem->addAction(copyAction);
        // add copy action to console widget
        auto widget = consoleWidget_->view();
        auto actions = widget->actions();
        if (actions.isEmpty()) {
            widget->addAction(copyAction);
        } else {
            // insert copy action at the beginning
            widget->insertAction(actions.front(), copyAction);
        }
        copyAction->setEnabled(false);
    }

    {
        auto pasteAction = new QAction(tr("&Paste"), this);
        actions_["Paste"] = pasteAction;
        pasteAction->setShortcut(QKeySequence::Paste);
        editMenuItem->addAction(pasteAction);
    }

    {
        auto deleteAction = new QAction(tr("&Delete"), this);
        actions_["Delete"] = deleteAction;
        deleteAction->setShortcuts(QList<QKeySequence>(
            {QKeySequence::Delete, QKeySequence(Qt::ControlModifier + Qt::Key_Backspace)}));
        editMenuItem->addAction(deleteAction);
        deleteAction->setEnabled(false);
    }

    editMenuItem->addSeparator();

    {
        auto selectAlllAction = new QAction(tr("&Select All"), this);
        actions_["Select All"] = selectAlllAction;
        selectAlllAction->setShortcut(QKeySequence::SelectAll);
        editMenuItem->addAction(selectAlllAction);
        connect(selectAlllAction, &QAction::triggered, [&]() { networkEditor_->selectAll(); });
    }

    editMenuItem->addSeparator();

    {
        auto findAction = new QAction(tr("&Find Processor"), this);
        actions_["Find Processor"] = findAction;
        findAction->setShortcut(QKeySequence::Find);
        editMenuItem->addAction(findAction);
        connect(findAction, &QAction::triggered, [&]() { processorTreeWidget_->focusSearch(); });
    }

    {
        auto addProcessorAction = new QAction(tr("&Add Processor"), this);
        actions_["Add Processor"] = addProcessorAction;
        addProcessorAction->setShortcut(Qt::ControlModifier + Qt::Key_D);
        editMenuItem->addAction(addProcessorAction);
        connect(addProcessorAction, &QAction::triggered,
                [&]() { processorTreeWidget_->addSelectedProcessor(); });
    }

    editMenuItem->addSeparator();

    {
        auto clearLogAction = consoleWidget_->getClearAction();
        actions_["Clear Log"] = clearLogAction;
        editMenuItem->addAction(clearLogAction);
    }

    // View
    {
        // dock widget visibility menu entries
        viewMenuItem->addAction(settingsWidget_->toggleViewAction());
        processorTreeWidget_->toggleViewAction()->setText(tr("&Processor List"));
        viewMenuItem->addAction(processorTreeWidget_->toggleViewAction());
        propertyListWidget_->toggleViewAction()->setText(tr("&Property List"));
        viewMenuItem->addAction(propertyListWidget_->toggleViewAction());
        consoleWidget_->toggleViewAction()->setText(tr("&Output Console"));
        viewMenuItem->addAction(consoleWidget_->toggleViewAction());
        helpWidget_->toggleViewAction()->setText(tr("&Help"));
        viewMenuItem->addAction(helpWidget_->toggleViewAction());
        // Disabled until we figure out what we want to use it for //Peter
        // viewMenuItem->addAction(resourceManagerWidget_->toggleViewAction());
    }

    {
        // application/developer mode menu entries
        QIcon visibilityModeIcon;
        visibilityModeIcon.addFile(":/icons/view-developer.png", QSize(), QIcon::Normal,
                                   QIcon::Off);
        visibilityModeIcon.addFile(":/icons/view-application.png", QSize(), QIcon::Normal,
                                   QIcon::On);
        visibilityModeAction_ = new QAction(visibilityModeIcon, tr("&Application Mode"), this);
        visibilityModeAction_->setCheckable(true);
        visibilityModeAction_->setChecked(false);

        viewMenuItem->addAction(visibilityModeAction_);
        viewModeToolBar->addAction(visibilityModeAction_);

        appUsageModeProp_ = &InviwoApplication::getPtr()
                                 ->getSettingsByType<SystemSettings>()
                                 ->applicationUsageMode_;

        appUsageModeProp_->onChange([&]() { visibilityModeChangedInSettings(); });

        connect(visibilityModeAction_, &QAction::triggered, [&](bool appView) {
            auto selectedIdx = appUsageModeProp_->getSelectedValue();
            if (appView) {
                if (selectedIdx != UsageMode::Application)
                    appUsageModeProp_->setSelectedValue(UsageMode::Application);
            } else {
                if (selectedIdx != UsageMode::Development)
                    appUsageModeProp_->setSelectedValue(UsageMode::Development);
            }
        });

        visibilityModeChangedInSettings();
    }

    // Evaluation
    {
        QIcon enableDisableIcon;
        enableDisableIcon.addFile(":/icons/button_ok.png", QSize(), QIcon::Active, QIcon::Off);
        enableDisableIcon.addFile(":/icons/button_cancel.png", QSize(), QIcon::Active, QIcon::On);
        auto lockNetworkAction = new QAction(enableDisableIcon, tr("&Lock Network"), this);
        lockNetworkAction->setCheckable(true);
        lockNetworkAction->setChecked(false);
        lockNetworkAction->setToolTip("Enable/Disable Network Evaluation");

        lockNetworkAction->setShortcut(Qt::ControlModifier + Qt::Key_L);
        evalMenuItem->addAction(lockNetworkAction);
        evalToolBar->addAction(lockNetworkAction);
        connect(lockNetworkAction, &QAction::triggered, [lockNetworkAction, this]() {
            if (lockNetworkAction->isChecked()) {
                app_->getProcessorNetwork()->lock();
            } else {
                app_->getProcessorNetwork()->unlock();
            }
        });
    }

#if IVW_PROFILING
    {
        auto resetTimeMeasurementsAction =
            new QAction(QIcon(":/icons/stopwatch.png"), tr("Reset All Time Measurements"), this);
        resetTimeMeasurementsAction->setCheckable(false);

        connect(resetTimeMeasurementsAction, &QAction::triggered,
                [&]() { networkEditor_->resetAllTimeMeasurements(); });

        evalToolBar->addAction(resetTimeMeasurementsAction);
        evalMenuItem->addAction(resetTimeMeasurementsAction);
    }
#endif

    // Help
    {
        helpMenuItem->addAction(helpWidget_->toggleViewAction());

        auto aboutBoxAction = new QAction(QIcon(":/icons/about.png"), tr("&About"), this);
        connect(aboutBoxAction, &QAction::triggered, this, &InviwoMainWindow::showAboutBox);
        helpMenuItem->addAction(aboutBoxAction);
    }
}

void InviwoMainWindow::updateWindowTitle() {
    QString windowTitle = QString("Inviwo - Interactive Visualization Workshop - ");
    windowTitle.append(currentWorkspaceFileName_);

    if (getNetworkEditor()->isModified()) windowTitle.append("*");

    if (visibilityModeAction_->isChecked()) {
        windowTitle.append(" (Application mode)");
    } else {
        windowTitle.append(" (Developer mode)");
    }

    setWindowTitle(windowTitle);
}

void InviwoMainWindow::updateRecentWorkspaceMenu() {
    QStringList recentFiles{getRecentWorkspaceList()};

    for (auto elem : workspaceActionRecent_) {
        elem->setVisible(false);
    }

    for (int i = 0; i < recentFiles.size(); ++i) {
        workspaceActionRecent_[i]->setVisible(!recentFiles[i].isEmpty());
        if (!recentFiles[i].isEmpty()) {
            if(QFileInfo(recentFiles[i]).exists()) {
                QString menuEntry = tr("&%1 %2").arg(i + 1).arg(recentFiles[i]);
                workspaceActionRecent_[i]->setText(menuEntry);
            } else {
                QString menuEntry = tr("&%1 %2 (missing)").arg(i + 1).arg(recentFiles[i]);
                workspaceActionRecent_[i]->setText(menuEntry);
                workspaceActionRecent_[i]->setEnabled(false);
            }
            workspaceActionRecent_[i]->setData(recentFiles[i]);
        }
    }
    clearRecentWorkspaces_->setEnabled(!recentFiles.isEmpty());
}

void InviwoMainWindow::clearRecentWorkspaceMenu() {
    for (auto elem : workspaceActionRecent_) {
        elem->setVisible(false);
    }
    // save empty list
    saveRecentWorkspaceList(QStringList());
    clearRecentWorkspaces_->setEnabled(false);
    updateRecentWorkspaceMenu();
}

void InviwoMainWindow::addToRecentWorkspaces(QString workspaceFileName) {
    QStringList recentFiles{getRecentWorkspaceList()};

    recentFiles.removeAll(workspaceFileName);
    recentFiles.prepend(workspaceFileName);

    if (recentFiles.size() > static_cast<int>(maxNumRecentFiles_)) recentFiles.removeLast();
    saveRecentWorkspaceList(recentFiles);

    updateRecentWorkspaceMenu();
}

QStringList InviwoMainWindow::getRecentWorkspaceList() const {
    QSettings settings("Inviwo", "Inviwo");
    settings.beginGroup("mainwindow");
    QStringList list{settings.value("recentFileList").toStringList()};
    settings.endGroup();

    return list;
}

void InviwoMainWindow::saveRecentWorkspaceList(const QStringList& list) {
    QSettings settings("Inviwo", "Inviwo");
    settings.beginGroup("mainwindow");
    settings.setValue("recentFileList", list);
    settings.endGroup();
}

void InviwoMainWindow::setCurrentWorkspace(QString workspaceFileName) {
    workspaceFileDir_ = QFileInfo(workspaceFileName).absolutePath();
    currentWorkspaceFileName_ = workspaceFileName;
    updateWindowTitle();
}

void InviwoMainWindow::fillExampleWorkspaceMenu() {
    exampleMenu_->clear();
    for (const auto& module : app_->getModules()) {
        QMenu* moduleMenu = nullptr;
        auto moduleWorkspacePath = module->getPath(ModulePath::Workspaces);
        if (filesystem::directoryExists(moduleWorkspacePath)) {
            for (auto item : filesystem::getDirectoryContents(moduleWorkspacePath)) {
                // only accept inviwo workspace files
                if (filesystem::getFileExtension(item) == "inv") {
                    if (!moduleMenu)
                        moduleMenu =
                            exampleMenu_->addMenu(QString::fromStdString(module->getIdentifier()));

                    QString filename(QString::fromStdString(item));
                    QAction* action = moduleMenu->addAction(filename);
                    QString path(QString("%1/%2")
                                     .arg(QString::fromStdString(moduleWorkspacePath))
                                     .arg(filename));
                    action->setData(path);

                    QObject::connect(action, &QAction::triggered, this,
                                     &InviwoMainWindow::openExampleWorkspace);
                }
            }
        }
    }
    exampleMenu_->menuAction()->setVisible(!exampleMenu_->isEmpty());
}

void InviwoMainWindow::fillTestWorkspaceMenu() {
    testMenu_->clear();
    for (const auto& module : app_->getModules()) {
        auto moduleTestPath = module->getPath(ModulePath::RegressionTests);
        if (filesystem::directoryExists(moduleTestPath)) {
            QMenu* moduleMenu = nullptr;

            for (auto test : filesystem::getDirectoryContents(moduleTestPath,
                                                              filesystem::ListMode::Directories)) {
                std::string testdir = moduleTestPath + "/" + test;
                // only accept inviwo workspace files
                if (filesystem::directoryExists(testdir)) {
                    for (auto item : filesystem::getDirectoryContents(testdir)) {
                        if (filesystem::getFileExtension(item) == "inv") {
                            if (!moduleMenu) {
                                moduleMenu = testMenu_->addMenu(
                                    QString::fromStdString(module->getIdentifier()));
                            }
                            QAction* action = moduleMenu->addAction(QString::fromStdString(item));
                            action->setData(QString::fromStdString(testdir + "/" + item));
                            QObject::connect(action, &QAction::triggered, this,
                                             &InviwoMainWindow::openRecentWorkspace);
                        }
                    }
                }
            }
        }
    }

    // store path and extracted module name
    std::vector<std::pair<std::string, std::string> > paths;  // we need to keep the order...

    // add default workspace path
    std::string coreWorkspacePath = app_->getPath(PathType::Workspaces) + "/tests";
    if (filesystem::directoryExists(coreWorkspacePath)) {
        // check whether path contains at least one workspace
        bool workspaceExists = false;
        for (auto item : filesystem::getDirectoryContents(coreWorkspacePath)) {
            // only accept inviwo workspace files
            workspaceExists = (filesystem::getFileExtension(item) == "inv");
            if (workspaceExists) {
                break;
            }
        }
        if (workspaceExists) {
            paths.push_back({coreWorkspacePath, "core"});
        }
    }

    // add paths of inviwo modules, avoid duplicates
    for (auto directory : inviwoModulePaths_) {
        std::string moduleName;
        // remove "/modules" from given path
        std::size_t endpos = directory.rfind('/');
        if (endpos != std::string::npos) {
            directory = directory.substr(0, endpos);
            std::size_t pos = directory.rfind('/', endpos - 1);
            // extract module name from, e.g., "e:/projects/inviwo/inviwo-dev/modules" ->
            // "inviwo-dev"
            if (pos != std::string::npos) {
                moduleName = directory.substr(pos + 1, endpos - pos - 1);
            }
        }
        // TODO: remove hard-coded path to test workspaces
        directory += "/data/workspaces/tests";
        if (!filesystem::directoryExists(directory)) {
            continue;
        }

        // TODO: use cannoncial/absolute paths for comparison
        bool duplicate = false;
        for (auto item : paths) {
            if (item.first == directory) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            continue;
        }

        // check whether path contains at least one workspace
        bool workspaceExists = false;
        for (auto item : filesystem::getDirectoryContents(directory)) {
            // only accept inviwo workspace files
            workspaceExists = (filesystem::getFileExtension(item) == "inv");
            if (workspaceExists) {
                break;
            }
        }

        if (workspaceExists) {
            // at least one workspace could be found
            paths.push_back({directory, moduleName});
        }
    }

    // add menu entries
    for (auto& elem : paths) {
        QMenu* baseMenu = testMenu_;
        // add module name as submenu folder for better organization, if it exists
        if (!elem.second.empty()) {
            baseMenu = testMenu_->addMenu(QString::fromStdString(elem.second));
        }

        // add test workspaces to submenu
        auto fileList = filesystem::getDirectoryContents(elem.first);
        for (auto item : fileList) {
            // only accept inviwo workspace files
            if (filesystem::getFileExtension(item) == "inv") {
                QString filename(QString::fromStdString(item));
                QAction* action = baseMenu->addAction(filename);
                QString path(
                    QString("%1/%2").arg(QString::fromStdString(elem.first)).arg(filename));
                action->setData(path);

                QObject::connect(action, &QAction::triggered, this,
                                 &InviwoMainWindow::openRecentWorkspace);
            }
        }
    }
    testMenu_->menuAction()->setVisible(!testMenu_->isEmpty());
}

std::string InviwoMainWindow::getCurrentWorkspace() {
    return currentWorkspaceFileName_.toLocal8Bit().constData();
}

void InviwoMainWindow::newWorkspace() {
    if (currentWorkspaceFileName_ != "")
        if (!askToSaveWorkspaceChanges()) return;

    app_->getWorkspaceManager()->clear();

    setCurrentWorkspace(rootDir_ + "/workspaces/untitled.inv");
    getNetworkEditor()->setModified(false);
}

void InviwoMainWindow::openWorkspace(QString workspaceFileName) {
    openWorkspace(workspaceFileName, false);
}

void InviwoMainWindow::openWorkspace(QString workspaceFileName, bool exampleWorkspace) {
    std::string fileName{workspaceFileName.toStdString()};
    fileName = filesystem::cleanupPath(fileName);
    workspaceFileName = QString::fromStdString(fileName);

    if (!filesystem::fileExists(fileName)) {
        LogError("Could not find workspace file: " << fileName);
        return;
    }

    {
        NetworkLock lock(app_->getProcessorNetwork());
        app_->getWorkspaceManager()->clear();
        try {
            app_->getWorkspaceManager()->load(fileName, [&](ExceptionContext ec) {
                try {
                    throw;
                } catch (const IgnoreException& e) {
                    util::log(
                        e.getContext(),
                        "Incomplete network loading " + fileName + " due to " + e.getMessage(),
                        LogLevel::Error);
                }
            });

            if (exampleWorkspace) {
                setCurrentWorkspace(rootDir_ + "/workspaces/untitled.inv");
            } else {
                setCurrentWorkspace(workspaceFileName);
                addToRecentWorkspaces(workspaceFileName);
            }
        } catch (const Exception& e) {
            util::log(e.getContext(),
                      "Unable to load network " + fileName + " due to " + e.getMessage(),
                      LogLevel::Error);
            app_->getWorkspaceManager()->clear();
            setCurrentWorkspace(rootDir_ + "/workspaces/untitled.inv");
        }
        app_->processEvents();  // make sure the gui is ready before we unlock.
    }
    saveWindowState();
    getNetworkEditor()->setModified(false);
}
void InviwoMainWindow::openExampleWorkspace() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && askToSaveWorkspaceChanges()) {
        openWorkspace(action->data().toString(), true);
    }
}

void InviwoMainWindow::openLastWorkspace(std::string workspace) {
    workspace = filesystem::cleanupPath(workspace);
    if (!workspace.empty()) {
        openWorkspace(QString::fromStdString(workspace));
    } else if (!workspaceOnLastSuccessfulExit_.isEmpty()) {
        openWorkspace(workspaceOnLastSuccessfulExit_);
    } else {
        newWorkspace();
    }
}

void InviwoMainWindow::openWorkspace() {
    if (askToSaveWorkspaceChanges()) {
        InviwoFileDialog openFileDialog(this, "Open Workspace ...", "workspace");
        openFileDialog.addSidebarPath(PathType::Workspaces);
        openFileDialog.addSidebarPath(workspaceFileDir_);
        openFileDialog.addExtension("inv", "Inviwo File");
        openFileDialog.setFileMode(FileMode::AnyFile);

        if (openFileDialog.exec()) {
            QString path = openFileDialog.selectedFiles().at(0);
            openWorkspace(path);
        }
    }
}

void InviwoMainWindow::openRecentWorkspace() {
    if (QAction* action = qobject_cast<QAction*>(sender())) {
        std::string fileName(utilqt::fromQString(action->data().toString()));

        if (!filesystem::fileExists(fileName)) {
            LogError("Failed to load the requested file: " << fileName);
            return;
        }
        if (askToSaveWorkspaceChanges()) openWorkspace(action->data().toString());
    }
}

void InviwoMainWindow::saveWorkspace(QString workspaceFileName) {
    std::string fileName{workspaceFileName.toStdString()};
    fileName = filesystem::cleanupPath(fileName);

    try {
        app_->getWorkspaceManager()->save(fileName, [&](ExceptionContext ec) {
            try {
                throw;
            } catch (const IgnoreException& e) {
                util::log(e.getContext(),
                          "Incomplete network save " + fileName + " due to " + e.getMessage(),
                          LogLevel::Error);
            }
        });
        getNetworkEditor()->setModified(false);
        updateWindowTitle();
    } catch (const Exception& e) {
        util::log(e.getContext(),
                  "Unable to save network " + fileName + " due to " + e.getMessage(),
                  LogLevel::Error);
    }
}

void InviwoMainWindow::saveWorkspace() {
    if (currentWorkspaceFileName_.contains("untitled.inv"))
        saveWorkspaceAs();
    else {
        saveWorkspace(currentWorkspaceFileName_);
    }
}

void InviwoMainWindow::saveWorkspaceAs() {
    InviwoFileDialog saveFileDialog(this, "Save Workspace ...", "workspace");
    saveFileDialog.setFileMode(FileMode::AnyFile);
    saveFileDialog.setAcceptMode(AcceptMode::Save);
    saveFileDialog.setConfirmOverwrite(true);

    saveFileDialog.addSidebarPath(PathType::Workspaces);
    saveFileDialog.addSidebarPath(workspaceFileDir_);

    saveFileDialog.addExtension("inv", "Inviwo File");

    if (saveFileDialog.exec()) {
        QString path = saveFileDialog.selectedFiles().at(0);
        if (!path.endsWith(".inv")) path.append(".inv");

        saveWorkspace(path);
        setCurrentWorkspace(path);
        addToRecentWorkspaces(path);
    }
    saveWindowState();
}

void InviwoMainWindow::saveWorkspaceAsCopy() {
    InviwoFileDialog saveFileDialog(this, "Save Workspace ...", "workspace");
    saveFileDialog.setFileMode(FileMode::AnyFile);
    saveFileDialog.setAcceptMode(AcceptMode::Save);
    saveFileDialog.setConfirmOverwrite(true);

    saveFileDialog.addSidebarPath(PathType::Workspaces);
    saveFileDialog.addSidebarPath(workspaceFileDir_);

    saveFileDialog.addExtension("inv", "Inviwo File");

    if (saveFileDialog.exec()) {
        QString path = saveFileDialog.selectedFiles().at(0);

        if (!path.endsWith(".inv")) path.append(".inv");

        saveWorkspace(path);
        addToRecentWorkspaces(path);
    }
    saveWindowState();
}

void InviwoMainWindow::onModifiedStatusChanged(const bool& /*newStatus*/) { updateWindowTitle(); }

void InviwoMainWindow::showAboutBox() {
    auto caps = InviwoApplication::getPtr()->getModuleByType<InviwoCore>()->getCapabilities();
    auto syscap = getTypeFromVector<SystemCapabilities>(caps);

    const int buildYear = (syscap ? syscap->getBuildTimeYear() : 0);

    std::stringstream aboutText;
    aboutText << "<html><head>\n"
              << "<style>\n"
              << "table { margin-top:0px;margin-bottom:0px; }\n"
              << "table > tr > td { "
              << "padding-left:0px; padding-right:0px;padding-top:0px; \n"
              << "padding-bottom:0px;"
              << "}\n"
              << "</style>\n"
              << "<head/>\n"
              << "<body>\n"

              << "<b>Inviwo v" << IVW_VERSION << "</b><br>\n"
              << "Interactive Visualization Workshop<br>\n"
              << "&copy; 2012-" << buildYear << " The Inviwo Foundation<br>\n"
              << "<a href='http://www.inviwo.org/' style='color: #AAAAAA;'>"
              << "http://www.inviwo.org/</a>\n"
              << "<p>Inviwo is a rapid prototyping environment for interactive "
              << "visualizations.<br>It is licensed under the Simplified BSD license.</p>\n"

              << "<p><b>Core Team:</b><br>\n"
              << "Peter Steneteg, Erik Sund&eacute;n, Daniel J&ouml;nsson, Martin Falk, "
              << "Rickard Englund, Sathish Kottravel, Timo Ropinski</p>\n"

              << "<p><b>Former Developers:</b><br>\n"
              << "Alexander Johansson, Andreas Valter, Johan Nor&eacute;n, Emanuel Winblad, "
              << "Hans-Christian Helltegen, Viktor Axelsson</p>\n";
    if (syscap) {
        aboutText << "<p><b>Build Date: </b>\n" << syscap->getBuildDateString() << "</p>\n";
        aboutText << "<p><b>Repos:</b>\n"
                  << "<table border='0' cellspacing='0' cellpadding='0' style='margin: 0px;'>\n";
        for (size_t i = 0; i < syscap->getGitNumberOfHashes(); ++i) {
            auto item = syscap->getGitHash(i);
            aboutText << "<tr><td style='padding-right:8px;'>" << item.first
                      << "</td><td style='padding-right:8px;'>" << item.second << "</td></tr>\n";
        }
        aboutText << "</table></p>\n";
    }
    const auto& mfos = InviwoApplication::getPtr()->getModuleFactoryObjects();
    auto names = util::transform(
        mfos, [](const std::unique_ptr<InviwoModuleFactoryObject>& mfo) { return mfo->name_; });
    std::sort(names.begin(), names.end());
    aboutText << "<p><b>Modules:</b><br>\n" << joinString(names, ", ") << "</p>\n";
    aboutText << "</body></html>";

    aboutText << "<p><b>Qt:</b> Version " << QT_VERSION_STR << ".<br/>";

    auto str = aboutText.str();

    // Use custom dialog since in QMessageBox::about you can not select text
    auto about =
        new QMessageBox(QMessageBox::NoIcon, QString::fromStdString("Inviwo v" + IVW_VERSION),
                        QString::fromStdString(aboutText.str()), QMessageBox::Ok, this);
    auto icon = windowIcon();
    about->setIconPixmap(icon.pixmap(256));
    about->setTextInteractionFlags(Qt::TextBrowserInteraction);
    about->exec();
}

void InviwoMainWindow::visibilityModeChangedInSettings() {
    if (appUsageModeProp_) {
        auto network = getInviwoApplication()->getProcessorNetwork();
        switch (appUsageModeProp_->getSelectedValue()) {
            case UsageMode::Development: {
                for (auto& p : network->getProcessors()) {
                    auto md =
                        p->getMetaData<ProcessorMetaData>(ProcessorMetaData::CLASS_IDENTIFIER);
                    if (md->isSelected()) {
                        propertyListWidget_->addProcessorProperties(p);
                    } else {
                        propertyListWidget_->removeProcessorProperties(p);
                    }
                }

                if (visibilityModeAction_->isChecked()) {
                    visibilityModeAction_->setChecked(false);
                }
                networkEditorView_->hideNetwork(false);
                break;
            }
            case UsageMode::Application: {
                if (!visibilityModeAction_->isChecked()) {
                    visibilityModeAction_->setChecked(true);
                }
                networkEditorView_->hideNetwork(true);

                for (auto& p : network->getProcessors()) {
                    propertyListWidget_->addProcessorProperties(p);
                }
                break;
            }
        }

        updateWindowTitle();
    }
}

NetworkEditor* InviwoMainWindow::getNetworkEditor() const { return networkEditor_.get(); }

void InviwoMainWindow::exitInviwo(bool saveIfModified) {
    if (!saveIfModified) getNetworkEditor()->setModified(false);
    QMainWindow::close();
    app_->closeInviwoApplication();
}

void InviwoMainWindow::saveWindowState() {
    QSettings settings("Inviwo", "Inviwo");
    settings.beginGroup("mainwindow");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.setValue("maximized", isMaximized());
    // settings.setValue("recentFileList", recentFileList_);

    // save sticky flags for main dock widgets
    settings.beginGroup("dialogs");
    settings.setValue("settingswidgetSticky", settingsWidget_->isSticky());
    settings.setValue("processorwidgetSticky", processorTreeWidget_->isSticky());
    settings.setValue("propertywidgetSticky", propertyListWidget_->isSticky());
    settings.setValue("consolewidgetSticky", consoleWidget_->isSticky());
    settings.setValue("resourcemanagerwidgetSticky", resourceManagerWidget_->isSticky());
    settings.setValue("helpwidgetSticky", helpWidget_->isSticky());
    settings.endGroup();  // dialogs

    settings.endGroup();  // mainwindow
}
void InviwoMainWindow::loadWindowState() {
    // load settings and restore window state
    QSettings settings("Inviwo", "Inviwo");
    settings.beginGroup("mainwindow");
    restoreGeometry(settings.value("geometry", saveGeometry()).toByteArray());
    restoreState(settings.value("state", saveState()).toByteArray());
    maximized_ = settings.value("maximized", false).toBool();

    // restore sticky flags for main dock widgets
    settings.beginGroup("dialogs");
    settingsWidget_->setSticky(settings.value("settingswidgetSticky", true).toBool());
    processorTreeWidget_->setSticky(settings.value("processorwidgetSticky", true).toBool());
    propertyListWidget_->setSticky(settings.value("propertywidgetSticky", true).toBool());
    consoleWidget_->setSticky(settings.value("consolewidgetSticky", true).toBool());
    resourceManagerWidget_->setSticky(settings.value("resourcemanagerwidgetSticky", true).toBool());
    helpWidget_->setSticky(settings.value("helpwidgetSticky", true).toBool());
}

void InviwoMainWindow::closeEvent(QCloseEvent* event) {
    if (!askToSaveWorkspaceChanges()) {
        event->ignore();
        return;
    }

    app_->getWorkspaceManager()->clear();

    saveWindowState();

    QSettings settings("Inviwo", "Inviwo");
    settings.beginGroup("mainwindow");
    if (!currentWorkspaceFileName_.contains("untitled.inv")) {
        settings.setValue("workspaceOnLastSuccessfulExit", currentWorkspaceFileName_);
    } else {
        settings.setValue("workspaceOnLastSuccessfulExit", "");
    }
    settings.endGroup();

    // pass a close event to all children to let the same state etc.
    for (auto& child : children()) {
        QCloseEvent closeEvent;
        QApplication::sendEvent(child, &closeEvent);
    }

    QMainWindow::closeEvent(event);
}

bool InviwoMainWindow::askToSaveWorkspaceChanges() {
    bool continueOperation = true;

    if (getNetworkEditor()->isModified() && !app_->getProcessorNetwork()->isEmpty()) {
        QMessageBox msgBox(this);
        msgBox.setText("Workspace Modified");
        msgBox.setInformativeText("Do you want to save your changes?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int answer = msgBox.exec();

        switch (answer) {
            case QMessageBox::Yes:
                saveWorkspace();
                break;

            case QMessageBox::No:
                break;

            case QMessageBox::Cancel:
                continueOperation = false;
                break;
        }
    }

    return continueOperation;
}

SettingsWidget* InviwoMainWindow::getSettingsWidget() const { return settingsWidget_; }

ProcessorTreeWidget* InviwoMainWindow::getProcessorTreeWidget() const {
    return processorTreeWidget_;
}

PropertyListWidget* InviwoMainWindow::getPropertyListWidget() const { return propertyListWidget_; }

ConsoleWidget* InviwoMainWindow::getConsoleWidget() const { return consoleWidget_.get(); }

ResourceManagerWidget* InviwoMainWindow::getResourceManagerWidget() const {
    return resourceManagerWidget_;
}

HelpWidget* InviwoMainWindow::getHelpWidget() const { return helpWidget_; }

InviwoApplication* InviwoMainWindow::getInviwoApplication() const { return app_; }

const std::unordered_map<std::string, QAction*>& InviwoMainWindow::getActions() const {
    return actions_;
}

}  // namespace inviwo

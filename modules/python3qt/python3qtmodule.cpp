/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2014-2015 Inviwo Foundation
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

#include <modules/python3/pythonincluder.h>
#include <modules/python3qt/python3qtmodule.h>
#include <modules/python3qt/pythonqtmethods/pythonqtmethods.h>
#include <modules/python3qt/pythoneditorwidget.h>
#include <modules/python3/pyinviwo.h>
#include <modules/python3qt/pythonmenu.h>
#include <modules/python3/pythoninterface/pymodule.h>

namespace inviwo {

Python3QtModule::Python3QtModule(InviwoApplication* app)
    : InviwoModule(app, "Python3Qt")
    , inviwoPyQtModule_(util::make_unique<PyModule>("inviwoqt"))
    , menu_(util::make_unique<PythonMenu>(app))
    , pythonScriptArg_("p", "pythonScript", "Specify a python script to run at startup", false, "",
                       "Path to the file containing the script") {
    inviwoPyQtModule_->addMethod(new PyGetPathCurrentWorkspace());
    inviwoPyQtModule_->addMethod(new PyLoadNetworkMethod());
    inviwoPyQtModule_->addMethod(new PySaveNetworkMethod());
    inviwoPyQtModule_->addMethod(new PyQuitInviwoMethod());
    inviwoPyQtModule_->addMethod(new PyPromptMethod());
    inviwoPyQtModule_->addMethod(new PyShowPropertyWidgetMethod());
    PyInviwo::getPtr()->registerPyModule(inviwoPyQtModule_.get());

    app->getCommandLineParser().add(&pythonScriptArg_, [this]() {
        menu_->getEditor()->loadFile(pythonScriptArg_.getValue(), false);
        menu_->getEditor()->run();
    }, 100);
}

Python3QtModule::~Python3QtModule() = default;

} // namespace

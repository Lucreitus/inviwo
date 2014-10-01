/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 * Version 0.6b
 *
 * Copyright (c) 2012-2014 Inviwo Foundation
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
 * Main file authors: Peter Steneteg
 *
 *********************************************************************************/

#include <modules/python3/pythonincluder.h>

#include "pyinviwo.h"

#include <modules/python3/pythonscript.h>
#include <modules/python3/pythonexecutionoutputobservable.h>

#include <modules/python3/pythoninterface/pymodule.h>

#include <modules/python3/pyinviwoobserver.h>


#include "defaultinterface/pyproperties.h"
#include "defaultinterface/pycamera.h"
#include "defaultinterface/pycanvas.h"
#include "defaultinterface/pylist.h"
#include "defaultinterface/pyutil.h"
#include "defaultinterface/pyvolume.h"

namespace inviwo {
    static PyObject* py_stdout(PyObject* /*self*/, PyObject* args);
    class PyStdOutCatcher : public PyMethod {
    public:
        virtual std::string getName() const { return "ivwPrint"; }
        virtual std::string getDesc() const {
            return " Only for internal use. Redirect std output to python editor widget.";
        }
        virtual PyCFunction getFunc() { return py_stdout; }
    };

    static PyObject* py_stdout(PyObject* /*self*/, PyObject* args) {
        char* msg;
        int len;
        int isStderr;

        if (!PyArg_ParseTuple(args, "s#i", &msg, &len, &isStderr)) {
            LogWarnCustom("inviwo.Python.py_print", "failed to parse log message");
        }
        else {
            if (len != 0) {
                if (!(len == 1 && (msg[0] == '\n' || msg[0] == '\r' || msg[0] == '\0')))
                    PythonExecutionOutputObservable::getPtr()->pythonExecutionOutputEvent(
                    msg, (isStderr == 0) ? sysstdout : sysstderr);
            }
        }

        Py_RETURN_NONE;
    }




    PyInviwo::PyInviwo() : isInit_(false), inviwoPyModule_(0), inviwoInternalPyModule_(0), mainDict_(0), modulesDict_(0){
        init(this);

        initPythonCInterface();
    }

    PyInviwo::~PyInviwo() {
        delete inviwoPyModule_;
        delete inviwoInternalPyModule_;
    }


    void PyInviwo::registerPyModule(PyModule* pyModule){
        
        if (Py_IsInitialized()) {
            struct PyModuleDef moduleDef = { PyModuleDef_HEAD_INIT, pyModule->getModuleName(), NULL, -1, pyModule->getPyMethodDefs() };

            PyObject* obj = PyModule_Create(&moduleDef);

            if (!obj) {
                LogWarn("Failed to init python module '" << pyModule->getModuleName() << "' ");
            }
            PyDict_SetItemString(modulesDict_, pyModule->getModuleName(), obj);
            
            pyModule->setPyObject(obj);
            registeredModules_.push_back(pyModule);

            for (ObserverSet::reverse_iterator it = observers_->rbegin(); it != observers_->rend();
                ++it) {
                static_cast<PyInviwoObserver*>(*it)->onModuleRegistered(pyModule);
            }
        }
        else {
            LogError("Python environment not initialized");
        }
    }



    void PyInviwo::addModulePath(const std::string& path){
        LogError("NOT YET IMPLEMENTED");
    }

    void PyInviwo::initPythonCInterface() {
        if (isInit_) return;

        isInit_ = true;
        LogInfo("Python version: " + toString(Py_GetVersion()));
        wchar_t programName[] = L"PyInviwo";
        Py_SetProgramName(programName);
#ifdef WIN32
        Py_NoSiteFlag = 1;
#endif
        Py_InitializeEx(false);

        if (!Py_IsInitialized()) {
            LogError("Python is not Initialized");
            return;
        }

        PyEval_InitThreads();
        mainDict_ = PyDict_New();
        modulesDict_ = PyImport_GetModuleDict();
        if (PyDict_GetItemString(mainDict_, "__builtins__") == NULL)
        {
            PyObject* pMod = PyImport_ImportModule("builtins");
            if (NULL != pMod)
                PyDict_SetItemString(mainDict_, "__builtins__", pMod);
        }
        addModulePath(InviwoApplication::getPtr()->getBasePath() + "modules/python/scripts");

        initDefaultInterfaces();

        initOutputRedirector();
    }

    void PyInviwo::initDefaultInterfaces(){
        inviwoInternalPyModule_ = new PyModule("inviwo_internal");
        inviwoInternalPyModule_->addMethod(new PyStdOutCatcher());
        registerPyModule(inviwoInternalPyModule_);


        inviwoPyModule_ = new PyModule("inviwo");
        inviwoPyModule_->addMethod(new PySetPropertyValueMethod());
        inviwoPyModule_->addMethod(new PySetPropertyMaxValueMethod());
        inviwoPyModule_->addMethod(new PySetPropertyMinValueMethod());
        inviwoPyModule_->addMethod(new PyGetPropertyValueMethod());
        inviwoPyModule_->addMethod(new PyGetPropertyMaxValueMethod());
        inviwoPyModule_->addMethod(new PyGetPropertyMinValueMethod());
        inviwoPyModule_->addMethod(new PyClickButtonMethod());
        inviwoPyModule_->addMethod(new PySetCameraFocusMethod());
        inviwoPyModule_->addMethod(new PySetCameraUpMethod());
        inviwoPyModule_->addMethod(new PySetCameraPosMethod());
        inviwoPyModule_->addMethod(new PyListPropertiesMethod());
        inviwoPyModule_->addMethod(new PyListProcessorsMethod());
        inviwoPyModule_->addMethod(new PyCanvasCountMethod());
        inviwoPyModule_->addMethod(new PyResizeCanvasMethod());
        inviwoPyModule_->addMethod(new PySnapshotMethod());
        inviwoPyModule_->addMethod(new PySnapshotCanvasMethod());
        inviwoPyModule_->addMethod(new PyGetBasePathMethod());
        inviwoPyModule_->addMethod(new PyGetWorkspaceSavePathMethod());
        inviwoPyModule_->addMethod(new PyGetVolumePathMethod());
        inviwoPyModule_->addMethod(new PyGetDataPathMethod());
        inviwoPyModule_->addMethod(new PyGetImagePathMethod());
        inviwoPyModule_->addMethod(new PyGetModulePathMethod());
        inviwoPyModule_->addMethod(new PyGetTransferFunctionPath());
        inviwoPyModule_->addMethod(new PySetVoxelMethod());
        inviwoPyModule_->addMethod(new PyGetVolumeDimension());
        inviwoPyModule_->addMethod(new PyGetMemoryUsage());
        inviwoPyModule_->addMethod(new PyClearResourceManage());
        inviwoPyModule_->addMethod(new PyEnableEvaluation());
        inviwoPyModule_->addMethod(new PyDisableEvaluation());
        inviwoPyModule_->addMethod(new PySaveTransferFunction());
        inviwoPyModule_->addMethod(new PyLoadTransferFunction());
        inviwoPyModule_->addMethod(new PyClearTransferfunction());
        inviwoPyModule_->addMethod(new PyAddTransferFunction());


        registerPyModule(inviwoPyModule_);
    }
    
    void PyInviwo::initOutputRedirector()
    {
        PythonScript outputCatcher = PythonScript();
        std::string directorFileName =
            InviwoApplication::getPtr()->getPath(InviwoApplication::PATH_MODULES , "python3/scripts/outputredirector.py");
        std::ifstream file(directorFileName.c_str());
        std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        outputCatcher.setSource(text);

        if (!outputCatcher.run(false)) {
            LogWarn("Python init script failed to run");
        }
    }

} // namespace

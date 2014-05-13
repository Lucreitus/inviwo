/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 * Version 0.6b
 *
 * Copyright (c) 2013-2014 Inviwo Foundation
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
 * Main file author: Rickard Englund
 *
 *********************************************************************************/

#ifdef _MSC_VER
#pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#endif

#include <modules/unittests/unittestsmodule.h>


#include <modules/opengl/inviwoopengl.h>
#include <modules/glfw/canvasglfw.h>

#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/network/processornetwork.h>
#include <inviwo/core/network/processornetworkevaluator.h>
#include <inviwo/core/processors/canvasprocessor.h>
#include <inviwo/core/util/filesystem.h>
#include <modules/base/processors/imageexport.h>
#include <moduleregistration.h>


using namespace inviwo;

int main(int argc , char** argv) {
    int ret = -1;
    inviwo::ConsoleLogger* logger = new inviwo::ConsoleLogger();
    {
        // scope for ivw app
        
        inviwo::LogCentral::instance()->registerLogger(logger);

        // Search for directory containing data folder to find application basepath.
        // Working directory will be used if data folder is not found in parent directories.
        std::string basePath = inviwo::filesystem::findBasePath();
        InviwoApplication app(argc, argv, "unittest " + IVW_VERSION, basePath);
        
        if(!glfwInit()){
            LogErrorCustom("Inviwo Unit Tests Application", "GLFW could not be initialized.");
            return 0;
        }



        ProcessorNetwork processorNetwork;

        // Create process network evaluator and associate the canvas
        ProcessorNetworkEvaluator processorNetworkEvaluator(&processorNetwork);

        //Set processor network to application
        app.setProcessorNetwork(&processorNetwork);

        //Initialize all modules
        app.initialize(&inviwo::registerAllModules);

        //Continue initialization of default context
        CanvasGLFW* sharedCanvas = static_cast<CanvasGLFW*>(processorNetworkEvaluator.getDefaultRenderContext());
        sharedCanvas->initializeSquare();
        sharedCanvas->initialize();
        sharedCanvas->activate();
        
        
        ret = inviwo::UnitTestsModule::runAllTests();


        glfwWaitEvents();
    }
    delete logger;
    return ret;
}
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

#include <modules/base/basemodule.h>
#include <inviwo/core/io/serialization/versionconverter.h>
#include <modules/base/processors/convexhull2dprocessor.h>
#include <modules/base/processors/cubeproxygeometryprocessor.h>
#include <modules/base/processors/diffuselightsourceprocessor.h>
#include <modules/base/processors/directionallightsourceprocessor.h>
#include <modules/base/processors/distancetransformram.h>
#include <modules/base/processors/meshsource.h>
#include <modules/base/processors/heightfieldmapper.h>
#include <modules/base/processors/imageexport.h>
#include <modules/base/processors/imagesnapshot.h>
#include <modules/base/processors/imagesource.h>
#include <modules/base/processors/imagesourceseries.h>
#include <modules/base/processors/imagecontourprocessor.h>
#include <modules/base/processors/layerdistancetransformram.h>
#include <modules/base/processors/meshclipping.h>
#include <modules/base/processors/meshcreator.h>
#include <modules/base/processors/noiseprocessor.h>
#include <modules/base/processors/pixeltobufferprocessor.h>
#include <modules/base/processors/pointlightsourceprocessor.h>
#include <modules/base/processors/ordinalpropertyanimator.h>
#include <modules/base/processors/spotlightsourceprocessor.h>
#include <modules/base/processors/surfaceextraction.h>
#include <modules/base/processors/volumesequenceelementselectorprocessor.h>
#include <modules/base/processors/imagesequenceelementselectorprocessor.h>
#include <modules/base/processors/meshsequenceelementselectorprocessor.h>
#include <modules/base/processors/volumesource.h>
#include <modules/base/processors/volumeexport.h>
#include <modules/base/processors/volumebasistransformer.h>
#include <modules/base/processors/volumeslice.h>
#include <modules/base/processors/volumesubsample.h>
#include <modules/base/processors/volumesubset.h>
#include <modules/base/processors/volumesequencesource.h>
#include <modules/base/processors/volumetospatialsampler.h>
#include <modules/base/processors/orientationindicator.h>
#include <modules/base/processors/singlevoxel.h>
#include <modules/base/processors/stereocamerasyncer.h>
#include <modules/base/processors/worldtransform.h>
#include <modules/base/processors/volumeboundingbox.h>

#include <modules/base/properties/basisproperty.h>
#include <modules/base/properties/gaussianproperty.h>
#include <modules/base/properties/volumeinformationproperty.h>
#include <modules/base/properties/sequencetimerproperty.h>

#include <inviwo/core/processors/processor.h>
#include <inviwo/core/ports/meshport.h>
#include <inviwo/core/ports/volumeport.h>
#include <inviwo/core/ports/imageport.h>
#include <inviwo/core/ports/datainport.h>
#include <modules/base/processors/pixelvalue.h>
#include <modules/base/processors/volumesequencetospatial4dsampler.h>
#include <modules/base/processors/volumegradientcpuprocessor.h>
#include <modules/base/processors/volumecurlcpuprocessor.h>
#include <modules/base/processors/volumelaplacianprocessor.h>
#include <modules/base/processors/volumedivergencecpuprocessor.h>
#include <modules/base/processors/meshexport.h>

#include <modules/base/io/stlwriter.h>
#include <modules/base/io/binarystlwriter.h>
#include <modules/base/io/wavefrontwriter.h>
#include <modules/base/processors/randommeshgenerator.h>
#include <modules/base/processors/randomspheregenerator.h>
#include <modules/base/processors/inputselector.h>


#include <modules/base/processors/noisevolumeprocessor.h>


namespace inviwo {

using BasisTransformMesh = BasisTransform<Mesh>;
using BasisTransformVolume = BasisTransform<Volume>;

using WorldTransformMesh = WorldTransform<Mesh>;
using WorldTransformVolume = WorldTransform<Volume>;


BaseModule::BaseModule(InviwoApplication* app) : InviwoModule(app, "Base") {
    registerProcessor<ConvexHull2DProcessor>();
    registerProcessor<CubeProxyGeometry>();
    registerProcessor<DiffuseLightSourceProcessor>();
    registerProcessor<DirectionalLightSourceProcessor>();
    registerProcessor<DistanceTransformRAM>();
    registerProcessor<MeshSource>();
    registerProcessor<HeightFieldMapper>();
    registerProcessor<ImageExport>();
    registerProcessor<ImageSnapshot>();
    registerProcessor<ImageSource>();
    registerProcessor<ImageSourceSeries>();
    registerProcessor<LayerDistanceTransformRAM>();
    registerProcessor<MeshClipping>();
    registerProcessor<MeshCreator>();
    registerProcessor<NoiseProcessor>();
    registerProcessor<PixelToBufferProcessor>();
    registerProcessor<PointLightSourceProcessor>();
    registerProcessor<OrdinalPropertyAnimator>();
    registerProcessor<SpotLightSourceProcessor>();
    registerProcessor<SurfaceExtraction>();
    registerProcessor<VolumeSource>();
    registerProcessor<VolumeExport>();
    registerProcessor<BasisTransformMesh>();
    registerProcessor<BasisTransformVolume>();
    registerProcessor<WorldTransformMesh>();
    registerProcessor<WorldTransformVolume>();
    registerProcessor<VolumeSlice>();
    registerProcessor<VolumeSubsample>();
    registerProcessor<VolumeSubset>();
    registerProcessor<ImageContourProcessor>();
    registerProcessor<VolumeSequenceSource>();
    registerProcessor<VolumeSequenceElementSelectorProcessor>();
    registerProcessor<ImageSequenceElementSelectorProcessor>();
    registerProcessor<MeshSequenceElementSelectorProcessor>();
    
    registerProcessor<VolumeBoundingBox>();
    registerProcessor<SingleVoxel>();
    registerProcessor<StereoCameraSyncer>();

    registerProcessor<VolumeToSpatialSampler>();
    registerProcessor<OrientationIndicator>();
    registerProcessor<PixelValue>();
    registerProcessor<VolumeSequenceToSpatial4DSampler>();
    registerProcessor<VolumeGradientCPUProcessor>();
    registerProcessor<VolumeCurlCPUProcessor>();
    registerProcessor<VolumeDivergenceCPUProcessor>();
    registerProcessor<VolumeLaplacianProcessor>();
    registerProcessor<MeshExport>();
    registerProcessor<RandomMeshGenerator>();
    registerProcessor<RandomSphereGenerator>();
    registerProcessor<NoiseVolumeProcessor>();
    // input selectors
    registerProcessor<InputSelector<MultiDataInport<Volume>, VolumeOutport>>();
    registerProcessor<InputSelector<MultiDataInport<Mesh>, MeshOutport>>();
    registerProcessor<InputSelector<ImageMultiInport, ImageOutport>>();

    registerProperty<SequenceTimerProperty>();
    registerProperty<BasisProperty>();
    registerProperty<VolumeInformationProperty>();

    registerProperty<Gaussian1DProperty>();
    registerProperty<Gaussian2DProperty>();

    registerPort<DataInport<LightSource>>("LightSourceInport");
    registerPort<DataOutport<LightSource>>("LightSourceOutport");
    registerPort<BufferInport>("BufferInport");
    registerPort<BufferOutport>("BufferOutport");
    
    registerDataWriter(util::make_unique<StlWriter>());
    registerDataWriter(util::make_unique<BinarySTLWriter>());
    registerDataWriter(util::make_unique<WaveFrontWriter>());
    
    util::for_each_type<OrdinalPropertyAnimator::Types>{}(RegHelper{}, *this);
}

int BaseModule::getVersion() const {
    return 2;
}

std::unique_ptr<VersionConverter> BaseModule::getConverter(int version) const {
    return util::make_unique<Converter>(version);
}

BaseModule::Converter::Converter(int version) : version_(version) {}

bool BaseModule::Converter::convert(TxElement* root) {
    std::vector<xml::IdentifierReplacement> repl = {
        // CubeProxyGeometry
        {{xml::Kind::processor("org.inviwo.CubeProxyGeometry"),
          xml::Kind::inport("org.inviwo.VolumeInport")},
         "volume.inport",
         "volume"},
        {{xml::Kind::processor("org.inviwo.CubeProxyGeometry"),
          xml::Kind::outport("org.inviwo.MeshOutport")},
         "geometry.outport",
         "proxyGeometry"},
        // ImageSource
        {{xml::Kind::processor("org.inviwo.ImageSource"),
          xml::Kind::outport("org.inviwo.ImageOutport")},
         "image.outport",
         "image"},
        // MeshClipping
        {{xml::Kind::processor("org.inviwo.MeshClipping"),
          xml::Kind::inport("org.inviwo.MeshInport")},
         "geometry.input",
         "inputMesh"},
        {{xml::Kind::processor("org.inviwo.MeshClipping"),
          xml::Kind::outport("org.inviwo.MeshOutport")},
         "geometry.output",
         "clippedMesh"},

        // VolumeSlice
        {{xml::Kind::processor("org.inviwo.VolumeSlice"),
          xml::Kind::inport("org.inviwo.VolumeInport")},
         "volume.inport",
         "inputVolume"},
        {{xml::Kind::processor("org.inviwo.VolumeSlice"),
          xml::Kind::outport("org.inviwo.ImageOutport")},
         "image.outport",
         "outputImage"},

        // VolumeSubsample
        {{xml::Kind::processor("org.inviwo.VolumeSubsample"),
          xml::Kind::inport("org.inviwo.VolumeInport")},
         "volume.inport",
         "inputVolume"},
        {{xml::Kind::processor("org.inviwo.VolumeSubsample"),
          xml::Kind::outport("org.inviwo.VolumeOutport")},
         "volume.outport",
         "outputVolume"},

        // DistanceTransformRAM
        {{xml::Kind::processor("org.inviwo.DistanceTransformRAM"),
          xml::Kind::inport("org.inviwo.VolumeInport")},
         "volume.inport",
         "inputVolume"},
        {{xml::Kind::processor("org.inviwo.DistanceTransformRAM"),
          xml::Kind::outport("org.inviwo.VolumeOutport")},
         "volume.outport",
         "outputVolume"},

        // VolumeSubset
        {{xml::Kind::processor("org.inviwo.VolumeSubset"),
          xml::Kind::inport("org.inviwo.VolumeInport")},
         "volume.inport",
         "inputVolume"},
        {{xml::Kind::processor("org.inviwo.VolumeSubset"),
          xml::Kind::outport("org.inviwo.VolumeOutport")},
         "volume.outport",
         "outputVolume"}};

    for (const auto& id : { "org.inviwo.FloatProperty", "org.inviwo.FloatVec2Property",
                           "org.inviwo.FloatVec3Property", "org.inviwo.FloatVec4Property",
                           "org.inviwo.DoubleProperty", "org.inviwo.DoubleVec2Property",
                           "org.inviwo.DoubleVec3Property", "org.inviwo.DoubleVec4Property",
                           "org.inviwo.IntProperty", "org.inviwo.IntVec2Property",
                           "org.inviwo.IntVec3Property", "org.inviwo.IntVec4Property" }) {
        xml::IdentifierReplacement ir1 = {
            {xml::Kind::processor("org.inviwo.OrdinalPropertyAnimator"), xml::Kind::property(id)},
            id,
            dotSeperatedToPascalCase(id) };
        repl.push_back(ir1);
        xml::IdentifierReplacement ir2 = {
            {xml::Kind::processor("org.inviwo.OrdinalPropertyAnimator"), xml::Kind::property(id)},
            std::string(id) + "-Delta",
            dotSeperatedToPascalCase(id) + "-Delta" };
        repl.push_back(ir2);
    }

    bool res = false;
    switch (version_) {
        case 0: {
            res |= xml::changeIdentifiers(root, repl);
        }case 1: {
            res |= xml::changeAttribute(
                root, { { xml::Kind::processor("org.inviwo.GeometeryGenerator") } },
                "type",
                "org.inviwo.GeometeryGenerator",
                "org.inviwo.RandomMeshGenerator");
        }
            return res;

        default:
            return false;  // No changes
    }
    return true;
}

} // namespace

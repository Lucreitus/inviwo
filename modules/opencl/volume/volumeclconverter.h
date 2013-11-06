#ifndef IVW_VOLUMECLCONVERTER_H
#define IVW_VOLUMECLCONVERTER_H

#include <inviwo/core/datastructures/representationconverter.h>
#include <inviwo/core/datastructures/volume/volumeram.h>
#include <inviwo/core/datastructures/volume/volumeramconverter.h>
#include <inviwo/core/datastructures/volume/volumedisk.h>
#include <modules/opencl/volume/volumecl.h>

namespace inviwo {

    class IVW_MODULE_OPENCL_API VolumeRAM2CLConverter : public RepresentationConverterType<VolumeCL> {

    public:
        VolumeRAM2CLConverter();
        virtual ~VolumeRAM2CLConverter() {};

        inline bool canConvertFrom(const DataRepresentation* source) const {
            return dynamic_cast<const VolumeRAM*>(source) != NULL;
        }
        DataRepresentation* createFrom(const DataRepresentation* source);
        void update(const DataRepresentation* source, DataRepresentation* destination);
    };

    class IVW_MODULE_OPENCL_API VolumeDisk2CLConverter : public RepresentationConverterPackage<VolumeCL> {
    
    public:
        VolumeDisk2CLConverter() : RepresentationConverterPackage<VolumeCL>(){
            addConverter(new VolumeDisk2RAMConverter());
            addConverter(new VolumeRAM2CLConverter());
        };
        virtual ~VolumeDisk2CLConverter() {};
    };

    class IVW_MODULE_OPENCL_API VolumeCL2RAMConverter : public RepresentationConverterType<VolumeRAM> {

    public:
        VolumeCL2RAMConverter();
        virtual ~VolumeCL2RAMConverter(){};

        inline bool canConvertFrom(const DataRepresentation* source) const {
            return dynamic_cast<const VolumeCL*>(source) != NULL;
        }
        DataRepresentation* createFrom(const DataRepresentation* source);
        void update(const DataRepresentation* source, DataRepresentation* destination);
    };



} // namespace

#endif // IVW_VOLUMECLCONVERTER_H
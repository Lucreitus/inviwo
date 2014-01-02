/**********************************************************************
 * Copyright (C) 2012-2013 Scientific Visualization Group - Link�ping University
 * All Rights Reserved.
 * 
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * No part of this software may be reproduced or transmitted in any
 * form or by any means including photocopying or recording without
 * written permission of the copyright owner.
 *
 * Primary author : Erik Sund�n
 *
 **********************************************************************/

#include <inviwo/core/datastructures/image/layer.h>
#include <inviwo/core/datastructures/image/layerram.h>

namespace inviwo {

Layer::Layer(uvec2 dimensions, const DataFormatBase* format) : Data(format), StructuredGridMetaData<2>(dimensions) {}

Layer::Layer(LayerRepresentation* in) : Data(in->getDataFormat()), StructuredGridMetaData<2>(in->getDimensions()) 
{ 
    clearRepresentations();
    addRepresentation(in);
}

Layer::Layer(const Layer& rhs) : Data(rhs.dataFormatBase_), StructuredGridMetaData<2>(rhs.getDimension()) {}

Layer* Layer::clone() const {
    return new Layer(*this);
}

Layer::~Layer() {
    // Representations are deleted by Data destructor.
}

void Layer::resize(uvec2 dimensions) {
    setDimension(dimensions);
    if(lastValidRepresentation_) {
        // Resize last valid representation and remove the other ones
        static_cast<LayerRepresentation*>(lastValidRepresentation_)->resize(dimensions);
        std::vector<DataRepresentation*>::iterator it = std::find(representations_.begin(), representations_.end(), lastValidRepresentation_);
        // First delete the representations before erasing them from the vector
        for (size_t i=0; i<representations_.size(); i++) {
            if(representations_[i] != lastValidRepresentation_) {
                delete representations_[i]; representations_[i] = NULL;
            }
        }
        // Erasing representations: start from the back 
        if(it != --representations_.end()) {
            std::vector<DataRepresentation*>::iterator eraseFrom = it + 1;
            representations_.erase(eraseFrom, representations_.end()); 
        }
        // and then erase the ones infron of the valid representation
        if(representations_.begin() != it) {
            representations_.erase(representations_.begin(), it);
        }
         
    }
    setAllRepresentationsAsInvalid();
}


uvec2 Layer::getDimension() const {
	return StructuredGridMetaData<2>::getDimension();
}
void  Layer::setDimension(const uvec2& dim){
	StructuredGridMetaData<2>::setDimension(dim);
}

void Layer::resizeLayerRepresentations(Layer* targetLayer, uvec2 targetDim) {
    //TODO: check if getClassName() is necessary.
    //TODO: And also need to be tested on multiple representations_ such as LayerRAM, LayerDisk etc.,
    //TODO: optimize the code
    targetLayer->resize(targetDim);
    LayerRepresentation* layerRepresentation = 0;
    LayerRepresentation* targetRepresentation = 0;
    std::vector<DataRepresentation*> &targetRepresentations = targetLayer->representations_;

    if (targetRepresentations.size()) {
        for (int i=0; i<static_cast<int>(representations_.size()); i++) {
            layerRepresentation = dynamic_cast<LayerRepresentation*>(representations_[i]) ;        
            ivwAssert(layerRepresentation!=0, "Only layer representations should be used here.");
            if (isRepresentationValid(i)) {
                int numberOfTargets = static_cast<int>(targetRepresentations.size());
                for (int j=0; j<numberOfTargets; j++) {
                    targetRepresentation = dynamic_cast<LayerRepresentation*>(targetRepresentations[j]) ;
                    ivwAssert(targetRepresentation!=0, "Only layer representations should be used here.");
                    if (layerRepresentation->getClassName()==targetRepresentation->getClassName()) {
                        if (layerRepresentation->copyAndResizeLayer(targetRepresentation)) {
                            targetLayer->setRepresentationAsValid(j);
                            targetLayer->lastValidRepresentation_ = targetRepresentations[j];
                        }
                    }
                }
            }
        }
    }
    else {
        ivwAssert(lastValidRepresentation_!=0, "Last valid representation is expected.");
        targetLayer->setAllRepresentationsAsInvalid();
        targetLayer->createDefaultRepresentation();
        LayerRepresentation* lastValidRepresentation = dynamic_cast<LayerRepresentation*>(lastValidRepresentation_);
        LayerRepresentation* cloneOfLastValidRepresentation = dynamic_cast<LayerRepresentation*>(lastValidRepresentation->clone());        
        targetLayer->addRepresentation(cloneOfLastValidRepresentation);        

       targetLayer->resize(targetDim);
       if (lastValidRepresentation->copyAndResizeLayer(cloneOfLastValidRepresentation)) {
            targetLayer->setRepresentationAsValid(static_cast<int>(targetLayer->representations_.size())-1);
            targetLayer->lastValidRepresentation_ = cloneOfLastValidRepresentation;
       }
    }
}

DataRepresentation* Layer::createDefaultRepresentation() {
	return createLayerRAM((uvec2)getDimension(), getDataFormat());
}

} // namespace

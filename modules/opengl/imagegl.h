#ifndef IVW_IMAGEGL_H
#define IVW_IMAGEGL_H

#include "inviwo/core/inviwo.h"
#include "inviwo/core/datastructures/imagerepresentation.h"
#include "glwrap/framebufferobject.h"
#include "glwrap/texture2d.h"

namespace inviwo {

    class ImageGL : public ImageRepresentation {

    public:
        ImageGL();
        ImageGL(ivec2 dimensions);
        virtual ~ImageGL();

        void initialize();
        void deinitialize();

        void activateBuffer();
        void deactivateBuffer();

        void bindColorTexture(GLenum texUnit);
        void bindDepthTexture(GLenum texUnit);
        void bindTextures(GLenum colorTexUnit, GLenum depthTexUnit);
        virtual void resize(ivec2 dimensions);
        virtual ivec2 size() { return dimensions_;}

    private:
        ivec2 dimensions_;

        Texture2D* colorTexture_;
        Texture2D* depthTexture_;
        FrameBufferObject* frameBufferObject_;
    };

} // namespace

#endif // IVW_IMAGEGL_H

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
 * Main file authors: Erik Sund�n, Daniel J�nsson
 *
 *********************************************************************************/

#ifndef IVW_BUFFER_OBJECT_H
#define IVW_BUFFER_OBJECT_H

#include <modules/opengl/openglmoduledefine.h>
#include <modules/opengl/inviwoopengl.h>
#include <modules/opengl/buffer/bufferobjectobserver.h>
#include <inviwo/core/datastructures/geometry/attributes.h>
#include <inviwo/core/util/referencecounter.h>

namespace inviwo {

class IVW_MODULE_OPENGL_API BufferObject: public Observable<BufferObjectObserver>, public ReferenceCounter  {

public:
    BufferObject(size_t size, const DataFormatBase* format, BufferType type, BufferUsage usage, GLenum target = GL_ARRAY_BUFFER);
    virtual ~BufferObject();
    BufferObject(const BufferObject& rhs);

    virtual void initialize();
    virtual void deinitialize();
    virtual BufferObject* clone() const;

    const Buffer* getAttribute() const;
    GLenum getFormatType() const;
    GLuint getId() const;

    GLFormats::GLFormat getGLFormat() const { return glFormat_; }
    BufferType getBufferType() const { return type_; }

    virtual void enable() const;
    virtual void disable() const;

    void bind() const;

    void initialize(const void* data, GLsizeiptr sizeInBytes);
    void upload(const void* data, GLsizeiptr sizeInBytes);

    void download(void* data) const;

protected:
    void enableArray() const;
    void disableArray() const;
    void specifyLocation() const;

    void colorPointer() const;
    void normalPointer() const;
    void texCoordPointer() const;
    void vertexPointer() const;

    void emptyFunc() const;

private:
    GLuint id_;
    GLenum usageGL_;
    GLenum state_;
    GLenum target_;
    GLFormats::GLFormat glFormat_;
    BufferType type_;
    GLsizeiptr sizeInBytes_;
    void (BufferObject::*locationPointerFunc_)() const;

};


} // namespace

#endif // IVW_BUFFER_OBJECT_H
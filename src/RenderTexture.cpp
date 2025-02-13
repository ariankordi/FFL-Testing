#include <RenderTexture.h>
#include <gpu/rio_RenderState.h>

#include <gpu/rio_VertexArray.h>
#include <gpu/rio_TextureSampler.h>
#include <gpu/rio_Shader.h>

static const rio::TextureFormat cColorFormat = rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM;
static const rio::TextureFormat cDepthFormat = rio::DEPTH_TEXTURE_FORMAT_R32_FLOAT;

RenderTexture::RenderTexture(u32 width, u32 height)
    : mRenderBuffer(width, height)
    // default color format
    , mColorFormat(cColorFormat)
    , mRenderTextureColor(mColorFormat, width, height, 1)
    , mRenderTextureDepth(cDepthFormat, width, height, 1)
{
    linkTargets_();
}

RenderTexture::RenderTexture(u32 width, u32 height, rio::TextureFormat colorFormat)
    : mRenderBuffer(width, height)
    // default color format
    , mColorFormat(colorFormat)
    , mRenderTextureColor(mColorFormat, width, height, 1)
    , mRenderTextureDepth(cDepthFormat, width, height, 1)
{
    linkTargets_();
}

RenderTexture::~RenderTexture()
{
    mColorTarget.invalidateGPUCache();
    mDepthTarget.invalidateGPUCache();
}

void RenderTexture::linkTargets_()
{
    mColorTarget.linkTexture2D(mRenderTextureColor);
    mDepthTarget.linkTexture2D(mRenderTextureDepth);

    mRenderBuffer.setRenderTargetColor(&mColorTarget);
    mRenderBuffer.setRenderTargetDepth(&mDepthTarget);
}

void RenderTexture::bindDepthColor()
{
    mRenderBuffer.setRenderTargetColor(&mColorTarget);
    mRenderBuffer.setRenderTargetDepth(&mDepthTarget);
    mRenderBuffer.bind();
}

void RenderTexture::bindDepthOnly()
{
    mRenderBuffer.setRenderTargetColor(nullptr);
    mRenderBuffer.setRenderTargetDepth(&mDepthTarget);
    mRenderBuffer.bind();
}

/*
void RenderTexture::setWriteColorOnly()
{
    mRenderBuffer.setRenderTargetColo]r(&mColorTarget);
    mRenderBuffer.setRenderTargetDepth(nullptr);
}
*/

#include <gpu/rio_VertexBuffer.h>
#include <gpu/rio_Drawer.h>
#include <gpu/win/rio_Texture2DUtilWin.h>

// for socket
#if RIO_IS_WIN && defined(_WIN32)
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#elif RIO_IS_WIN
    #define closesocket close
    #include <sys/socket.h>
#endif // RIO_IS_WIN && defined(_WIN32)

void copyAndSendRenderBufferToSocket(rio::Texture2D* texture, int socket, int ssaaFactor)
{
    // does operations on the renderbuffer assuming it is already bound
#ifdef TRY_SCALING
    const u32 width = texture->getWidth() / ssaaFactor;
    const u32 height = texture->getHeight() / ssaaFactor;
#else
    const u32 width = texture->getWidth();
    const u32 height = texture->getHeight();
#endif
    // Read the rendered data into a buffer and save it to a file
    const rio::TextureFormat textureFormat = texture->getTextureFormat();
    u32 bufferSize = rio::Texture2DUtil::calcImageSize(textureFormat, texture->getWidth(), texture->getHeight());





#ifdef TRY_SCALING

    rio::RenderBuffer renderBufferDownsample;
    renderBufferDownsample.setSize(width, height);
    RIO_GL_CALL(glViewport(0, 0, width, height));
    //rio::Window::instance()->getNativeWindow()->getColorBufferTextureFormat();

    rio::Texture2D renderTextureDownsampleColor(rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM, renderBufferDownsample.getSize().x, renderBufferDownsample.getSize().y, 1);
    rio::RenderTargetColor renderTargetDownsampleColor;
    renderTargetDownsampleColor.linkTexture2D(renderTextureDownsampleColor);
    renderBufferDownsample.setRenderTargetColor(&renderTargetDownsampleColor);
    renderBufferDownsample.clear(rio::RenderBuffer::CLEAR_FLAG_COLOR, { 0.0f, 0.0f, 0.0f, 0.0f });

    renderBufferDownsample.bind();
    rio::RenderState render_state;
    render_state.setBlendEnable(true);

    // premultiplied alpha blending
    render_state.setBlendEquation(rio::Graphics::BLEND_FUNC_ADD);
    //render_state.setBlendFactor(rio::Graphics::BLEND_MODE_ONE, rio::Graphics::BLEND_MODE_DST_ALPHA);

    render_state.setBlendFactorSrcRGB(rio::Graphics::BLEND_MODE_ONE);
    render_state.setBlendFactorDstRGB(rio::Graphics::BLEND_MODE_ONE_MINUS_SRC_ALPHA);

    render_state.setBlendEquationAlpha(rio::Graphics::BLEND_FUNC_ADD);
    render_state.setBlendFactorSrcAlpha(rio::Graphics::BLEND_MODE_ONE_MINUS_DST_ALPHA);
    render_state.setBlendFactorDstAlpha(rio::Graphics::BLEND_MODE_ONE);

    render_state.applyBlendAndFastZ();

    // Load and compile the downsampling shader
    rio::Shader downsampleShader;
    downsampleShader.load("TransparentAdjuster");
    downsampleShader.bind();

    // Bind the high-resolution texture
    rio::TextureSampler2D highResSampler;
    highResSampler.linkTexture2D(texture);
    highResSampler.tryBindFS(downsampleShader.getFragmentSamplerLocation("s_Tex"), 0);

    const float oneDivisionWidth = 1.0f / (width * ssaaFactor);
    const float oneDivisionHeight = 1.0f / (height * ssaaFactor);

    // Set shader uniforms if needed
    downsampleShader.setUniform(oneDivisionWidth, oneDivisionHeight, u32(-1), downsampleShader.getFragmentUniformLocation("u_OneDivisionResolution"));

    // Render a full-screen quad to apply the downsampling shader.


    // Struct for the quad triangle strip data.
    struct QuadVertex {
        float position[2]; // R32G32 for position
        float texcoord[2]; // R32G32 for texCoord
    };
    static const s32 cQuadStride = sizeof(QuadVertex); // Stride

    static const s32 cQuadVtxNum = 4;
    // Quad vertex data in a struct format
    static const QuadVertex quadTriStripVertices[cQuadVtxNum] = {
#ifndef RIO_NO_CLIP_CONTROL
        // Position        // TexCoord
        {{-1.0f, -1.0f}, {0.0f, 1.0f}}, // Top-left
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}}, // Top-right
        {{-1.0f,  1.0f}, {0.0f, 0.0f}}, // Bottom-left
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}}, // Bottom-right
#else
        // Position        // TexCoord
        {{-1.0f,  1.0f}, {0.0f, 1.0f}}, // Top-left
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}, // Top-right
        {{-1.0f, -1.0f}, {0.0f, 0.0f}}, // Bottom-left
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}}, // Bottom-right
#endif
    };

    rio::VertexBuffer vertexBuffer; // VBO ig

    // Set the vertex buffer instance's stride (must be done before setting the instance's data)
    vertexBuffer.setStride(cQuadStride);
    // Set the vertex buffer instance's data and flush/invalidate its cache (now, as it won't be modified later)
    vertexBuffer.setDataInvalidate(quadTriStripVertices, sizeof(quadTriStripVertices));

    // Set the vertex attribute stream's layout
    rio::VertexStream posStream;
    rio::VertexStream texStream;
    posStream.setLayout(0, rio::VertexStream::FORMAT_32_32_FLOAT, offsetof(QuadVertex, position));
    texStream.setLayout(1, rio::VertexStream::FORMAT_32_32_FLOAT, offsetof(QuadVertex, texcoord));

    rio::VertexArray vertexArray; // VAO
    vertexArray.addAttribute(posStream, vertexBuffer);
    vertexArray.addAttribute(texStream, vertexBuffer);
    vertexArray.process();

    // shader was already bound
    vertexArray.bind();

    rio::Drawer::DrawArrays(rio::Drawer::TRIANGLE_STRIP, cQuadVtxNum);

    // VertexArray (glDeleteVertexArrays) and
    // VertexBuffer (glDeleteBuffers) destructors
    // will be called at the end of this function.

    downsampleShader.unload();

    renderBufferDownsample.bind();


    //int bufferSize = renderRequest->resolution * renderRequest->resolution * 4;
/*
    // Create a regular framebuffer to resolve the MSAA buffer to
    GLuint resolveFBO, resolveColorRBO;
    glGenFramebuffers(1, &resolveFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    // Create and attach a regular color renderbuffer
    glGenRenderbuffers(1, &resolveColorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, resolveColorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, renderRequest->resolution, renderRequest->resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resolveColorRBO);


    // Blit (resolve) the multisampled framebuffer to the regular framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
    glBlitFramebuffer(0, 0, renderRequest->resolution, renderRequest->resolution,
                    0, 0, renderRequest->resolution, renderRequest->resolution,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);

    RIO_GL_CALL(glReadPixels(0, 0, renderRequest->resolution, renderRequest->resolution, GL_RGBA, GL_UNSIGNED_BYTE, readBuffer));
*/

#endif

    //renderBufferDownsample.read(0, readBuffer, renderBufferDownsample.getSize().x, renderBufferDownsample.getSize().y, renderTextureDownsampleColor->getNativeTexture().surface.nativeFormat);

    rio::NativeTextureFormat nativeFormat = texture->getNativeTexture().surface.nativeFormat;

#if RIO_IS_WIN
    // map a pixel buffer object (DMA?)
    GLuint pbo;
    RIO_GL_CALL(glGenBuffers(1, &pbo));
    RIO_GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo));
    RIO_GL_CALL(glBufferData(GL_PIXEL_PACK_BUFFER, bufferSize, nullptr, GL_STREAM_READ));

    // Read pixels into PBO (asynchronously)
    RIO_GL_CALL(glReadPixels(0, 0, width, height, nativeFormat.format, nativeFormat.type, nullptr));

    // Map the PBO to read the data
    void* readBuffer;
    // opengl 2 and below compatible function
    //RIO_GL_CALL(readBuffer = glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
    // this should be compatible with OpenGL 3.3 and OpenGL ES 3.0
    RIO_GL_CALL(readBuffer = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, bufferSize, GL_MAP_READ_BIT));

    // don't dereference a null pointer
    if (readBuffer)
    {
        // Process the data in readBuffer
        RIO_LOG("Rendered data read successfully from the buffer.\n");
        send(socket, reinterpret_cast<char*>(readBuffer), bufferSize, 0);
        RIO_GL_CALL(glUnmapBuffer(GL_PIXEL_PACK_BUFFER));
    }
    else
        RIO_ASSERT(!readBuffer && "why is readBuffer nullptr...???");

    // Unbind the PBO
    RIO_GL_CALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
    RIO_GL_CALL(glDeleteBuffers(1, &pbo));
// NOTE: renderBuffer.read() DOES NOT WORK ON CAFE AS OF WRITING, NOR IS THIS MEANT TO RUN ON WII U AT ALL LOL
#else
    u8* readBuffer = new u8[bufferSize];
    //rio::MemUtil::set(readBuffer, 0xFF, bufferSize);
    // NOTE: UNTESTED
    renderBuffer.read(0, readBuffer, renderBuffer.getSize().x, renderBuffer.getSize().y, nativeFormat);
    send(socket, reinterpret_cast<char*>(&header), TGA_HEADER_SIZE, 0); // send tga header
    send(socket, reinterpret_cast<char*>(readBuffer), bufferSize, 0);
    delete[] readBuffer;
#endif

    RIO_LOG("Wrote %d bytes out to socket.\n", bufferSize);
}

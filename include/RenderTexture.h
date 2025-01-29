#pragma once

#include <gpu/rio_Texture.h>
#include <gpu/rio_RenderBuffer.h>
#include <gpu/rio_RenderTarget.h>

void copyAndSendRenderBufferToSocket(rio::Texture2D* texture, int socket, int ssaaFactor);

class RenderTexture
{
public:
    RenderTexture(u32 width, u32 height);
    RenderTexture(u32 width, u32 height, rio::TextureFormat colorFormat);

    ~RenderTexture();

    rio::TextureFormat getColorFormat() const
    {
        return mRenderTextureColor.getTextureFormat();
    };
    rio::Texture2D* getColorTexture()
    {
        return &mRenderTextureColor;
    };

    void bind()
    {
        mRenderBuffer.bind();
    }
    void clear(u32 clear_flag, const rio::Color4f& color = rio::Color4f::cBlack, f32 depth = 1.0f, u8 stencil = 0)
    {
        mRenderBuffer.clear(0, clear_flag, color, depth, stencil);
    }

    void bindDepthColor();
    void bindDepthOnly();
    void sendToSocket();
private:
    void linkTargets_();

    rio::RenderBuffer      mRenderBuffer;
    rio::TextureFormat     mColorFormat;

    rio::Texture2D         mRenderTextureColor;
    rio::Texture2D         mRenderTextureDepth;

    rio::RenderTargetColor mColorTarget;
    rio::RenderTargetDepth mDepthTarget;
};

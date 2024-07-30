#pragma once

#include <gpu/rio_Shader.h>
#include <nn/ffl.h>

class IShader
{
public:
    virtual ~IShader() = default;

    virtual void initialize() = 0;
    virtual void bind(bool light_enable, FFLiCharInfo* charInfo) = 0;
    virtual void bindBodyShader(FFLiCharInfo* pCharInfo) = 0;
    virtual void setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const = 0;
    virtual void setViewUniformBody(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const = 0;
    virtual void applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const = 0;
    virtual void applyAlphaTestEnable() const = 0;
    virtual void applyAlphaTestDisable() const = 0;
    // static method which means it's not here i guess
    // actually this is only called inside for now so shouldn't it be private
    //virtual void setCulling(FFLCullMode mode) = 0;
};

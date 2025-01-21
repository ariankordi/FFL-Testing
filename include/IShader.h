#pragma once

#include <gpu/rio_Shader.h>
#include <nn/ffl.h>
#include <nn/ffl/FFLiCharModel.h>

// Custom material parameters to accomodate body and pants
// used in all shader classes at the moment.
#define CUSTOM_MATERIAL_PARAM_BODY  FFL_MODULATE_TYPE_SHAPE_MAX
#define CUSTOM_MATERIAL_PARAM_PANTS FFL_MODULATE_TYPE_SHAPE_MAX + 1

#define CUSTOM_MATERIAL_PARAM_SIZE  CUSTOM_MATERIAL_PARAM_PANTS + 1

#define SHADER_MAX_BONE_COUNT 65

#include <PantsColor.h> // make available to shaders

class IShader
{
public:
    virtual ~IShader() = default;

    virtual void initialize() = 0;
    virtual void bind(bool light_enable, FFLiCharInfo* charInfo) = 0;
    virtual void setModulate(const FFLModulateParam& modulateParam) = 0;
    virtual void setModulatePantsMaterial(PantsColor pantsColor) = 0;
    virtual void setViewUniform(const rio::BaseMtx34f& model_mtx, const rio::BaseMtx34f& view_mtx, const rio::BaseMtx44f& proj_mtx) const = 0;
    virtual void resetUniformsToDefault() = 0;
    virtual void setLightDirection(rio::Vector3f direction) = 0;
    virtual void setBoneMatrix(rio::Matrix34f* mtx, s32 boneCount) = 0;
    virtual void applyAlphaTest(bool enable, rio::Graphics::CompareFunc func, f32 ref) const = 0;
    virtual void applyAlphaTestEnable() const = 0;
    virtual void applyAlphaTestDisable() const = 0;
    // static method which means it's not here i guess
    // actually this is only called inside for now so shouldn't it be private
    //virtual void setCulling(FFLCullMode mode) = 0;
};

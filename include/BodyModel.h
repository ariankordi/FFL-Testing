#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>
#include <gfx/mdl/rio_Model.h>

#include <IShader.h>
#include <Types.h>
#include <BodyTypes.h>

class Model;

class BodyModel
{
public:
    BodyModel(rio::mdl::Model* pBodyModel, BodyType type);
    ~BodyModel();

    void initialize(Model* pModel, PantsColor pantsColor);

    void setPantsColor(PantsColor pantsColor)
    {
        mPantsColor = pantsColor;
    };
    const rio::Vector3f getBodyScale() const
    {
        return mBodyScale;
    }
    rio::Vector3f getHeadRotation();
    rio::Vector3f getHeadRelativeTranslation();
    rio::Vector3f getHeadTranslation();
    rio::Matrix34f getHeadModelMatrix();

    static rio::Vector3f calcBodyScale(f32 build, f32 height);
    void draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx,
    rio::BaseMtx44f& proj_mtx);
private:
    const rio::mdl::Model* mpBodyModel;
    Model*                 mpModel;

    rio::Vector3f          mScale;
    rio::Vector3f          mBodyScale;
    IShader*               mpShader;
    FFLColor               mBodyColor; // aka favorite color
    BodyType               mBodyType;
    PantsColor             mPantsColor;
};

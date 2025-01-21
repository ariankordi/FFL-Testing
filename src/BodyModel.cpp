#include <BodyModel.h>
#include <BodyTypes.h>
#include <Types.h>
#include <Model.h>

BodyModel::BodyModel(rio::mdl::Model* pBodyModel, BodyType type)
    : mpModel(nullptr)
    , mScale{ 1.0f, 1.0f, 1.0f }
    , mBodyScale{ 1.0f, 1.0f, 1.0f }
    , mpShader(nullptr)
    , mBodyColor{ 0.0f, 0.0f, 0.0f, 1.0f }
    , mBodyType{ type }
    , mPantsColor(PANTS_COLOR_GRAY)
{
    // ig you already know the gender by now
    mpBodyModel = pBodyModel;

    const f32 s = cBodyTypeScaleFactors[type];
    mScale = { s, s, s };
}

BodyModel::~BodyModel() { }

void BodyModel::initialize(Model* pModel, PantsColor pantsColor)
{
    mpModel = pModel;

    FFLiCharInfo* pCharInfo = &reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel())->charInfo;
    mBodyScale = calcBodyScale(static_cast<f32>(pCharInfo->build), static_cast<f32>(pCharInfo->height));

    mBodyColor = FFLGetFavoriteColor(pCharInfo->favoriteColor);

    mPantsColor = pantsColor;
}

rio::Vector3f BodyModel::getHeadRotation()
{
    return cBodyTypeHeadRotation[mBodyType];
}

rio::Vector3f BodyModel::getHeadRelativeTranslation()
{
    return { 0.0f, cBodyTypeHeadTranslation[mBodyType], 0.0f };
}

rio::Vector3f BodyModel::getHeadTranslation()
{
    rio::Vector3f translate;
    translate.setMul(getHeadRelativeTranslation(), mBodyScale);
    translate.setMul(translate, mScale);
    return translate;
}

rio::Matrix34f BodyModel::getHeadModelMatrix()
{
    rio::Matrix34f bodyHeadMatrix;
    // apply head rotation, and translation
    bodyHeadMatrix.makeRT(getHeadRotation(), getHeadTranslation());
    return bodyHeadMatrix;
}

// draws mii body based on charinfo's build/height
// shader sets favorite and pants color
void BodyModel::draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx)
{
    //const BodyType bodyType = BODY_TYPE_WIIU_MIIBODYMIDDLE;

    const bool lightEnable = mpModel->getLightEnable();
    FFLiCharInfo* pCharInfo = mpModel->getCharInfo();
    //const FFLGender gender = pCharInfo->gender;

    // Select body model.
    //RIO_ASSERT(gender < FFL_GENDER_MAX);
    //const rio::mdl::Model* model = mpBodyModels[bodyType][gender]; // Based on gender.

    const rio::mdl::Mesh* meshes = mpBodyModel->meshes(); // Body and pants mesh.

    // Render each mesh in order
    for (u32 i = 0; i < mpBodyModel->numMeshes(); i++)
    {
        const rio::mdl::Mesh& mesh = meshes[i];

        // Bind shader and set body material.
        IShader* pShader = mpModel->getShader();
        pShader->bind(lightEnable, pCharInfo);

        bool isPantsModel = ((i % 2) == 1); // is it the second mesh?

        if (isPantsModel
            // if pants color is same as body then the
            // same modulate/material AS the body is used
            && mPantsColor != PANTS_COLOR_SAME_AS_BODY
        )
        {
            if (mPantsColor == PANTS_COLOR_NO_DRAW_PANTS)
                continue; // Break out of the loop
            // we would be able to use the same setModulate method
            // if only the switch shader just let you use arbitrary
            // colors but no it NEEDS the index of the pants colorhHHHHHHHHHHHHHHg
            pShader->setModulatePantsMaterial(mPantsColor);
        }
        else
        {
            const FFLColor modulateColor = FFLGetFavoriteColor(pCharInfo->favoriteColor);
            const FFLModulateParam modulateParam = {
                FFL_MODULATE_MODE_CONSTANT, // no texture
                CUSTOM_MATERIAL_PARAM_BODY, // decides which material is bound
                &modulateColor, // constant color (R)
                nullptr, // no color G
                nullptr, // no color B
                nullptr  // no texture
            };

            pShader->setModulate(modulateParam);
        }

        // make new matrix for body
        rio::Matrix34f modelMtxBody = rio::Matrix34f::ident;//model_mtx;

        // apply scale factors before anything else
        modelMtxBody.applyScaleLocal(mBodyScale);
        // apply original model matrix (rotation)
        modelMtxBody.setMul(model_mtx, modelMtxBody);

        pShader->setViewUniform(modelMtxBody, view_mtx, proj_mtx);

        rio::RenderState render_state;
        render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
        render_state.applyCullingAndPolygonModeAndPolygonOffset();
        mesh.draw();
    }
}


// scale vec3 for body
rio::Vector3f BodyModel::calcBodyScale(f32 build, f32 height)
{
    rio::Vector3f bodyScale;
    // referenced in GetBodyScale (anonymous function) in nn::mii::detail::VariableIconBodyImpl::CalculateWorldMatrix
    // also in ffl_app.rpx: FUN_020ec380 (FFLUtility), FUN_020737b8 (mii maker US)
#ifndef USE_HEIGHT_LIMIT_SCALE_FACTORS
    // ScaleApply?
                    // 0.47 / 128.0 = 0.003671875
    bodyScale.x = (build * (height * 0.003671875f + 0.4f)) / 128.0f +
                    // 0.23 / 128.0 = 0.001796875
                    height * 0.001796875f + 0.4f;
                    // 0.77 / 128.0 = 0.006015625
    bodyScale.y = (height * 0.006015625f) + 0.5f;

    /* the following set is found in ffl_app.rpx (FFLUtility)
     * Q:/sugar/program/ffl_application/src/mii/body/Scale.cpp
     * when an input is set to 1 (enum ::mii::body::ScaleMode?)
     * it may be for limiting scale so that the pants don't show
     * this may be what is used in wii u mii maker bottom screen icons but otherwise the above factors seem more relevant
    */
#else
    // ScaleLimit?

    // NOTE: even in wii u mii maker this still shows a few
    // pixels of the pants, but here without proper body scaling
    // this won't actually let you get away w/o pants
    f32 heightFactor = height / 128.0f;
    bodyScale.y = heightFactor * 0.55 + 0.6;
    bodyScale.x = heightFactor * 0.3 + 0.6;
    bodyScale.x = ((heightFactor * 0.6 + 0.8) - bodyScale.x) *
                        (build / 128.0f) + bodyScale.x;
#endif

    // z is always set to x for either set
    bodyScale.z = bodyScale.x;

    return bodyScale;
}

#include <BodyModel.h>
#include <BodyTypes.h>
#include <Types.h>
#include <Model.h>

#include <misc/rio_MemUtil.h>

//BodyModel::BodyModel(rio::mdl::Model* pBodyModel, BodyType type)
BodyModel::BodyModel(BodyModelItem* pItem)
    : mpModel(nullptr)
    , mpBodyModel(pItem)
    //, mScale{ 1.0f, 1.0f, 1.0f }
    , mBodyScale{ 1.0f, 1.0f, 1.0f }
    , mpShader(nullptr)
    , mBodyColor{ 0.0f, 0.0f, 0.0f, 1.0f }
    , mBodyType{ pItem->mType }
    , mPantsColor(PANTS_COLOR_GRAY)
    , mUseSkeleton(false)
    , mSkeletonMatrix{ }
{
    const f32 s = mpBodyModel->mScale;
    mScale = { s, s, s };
}

BodyModel::~BodyModel() { }

namespace
{
    // Note that these are the same bones
    // between the Switch MiiBodyHigh model
    // and the Wii U MiiBodyMiddle model.
    enum VriableIconBodyBoneKind
    {
        VriableIconBodyBoneKind_AllRoot   = 0,
        VriableIconBodyBoneKind_Body      = 1,
        VriableIconBodyBoneKind_SklRoot   = 2,
        VriableIconBodyBoneKind_Chest     = 3,
        VriableIconBodyBoneKind_ArmL1     = 4,
        VriableIconBodyBoneKind_ArmL2     = 5,
        VriableIconBodyBoneKind_WristL    = 6,
        VriableIconBodyBoneKind_ElbowL    = 7,
        VriableIconBodyBoneKind_ShoulderL = 8,
        VriableIconBodyBoneKind_ArmR1     = 9,
        VriableIconBodyBoneKind_ArmR2     = 10,
        VriableIconBodyBoneKind_WristR    = 11,
        VriableIconBodyBoneKind_ElbowR    = 12,
        VriableIconBodyBoneKind_ShoulderR = 13,
        VriableIconBodyBoneKind_Head      = 14,
        VriableIconBodyBoneKind_Chest2    = 15,
        VriableIconBodyBoneKind_Hip       = 16,
        VriableIconBodyBoneKind_FootL1    = 17,
        VriableIconBodyBoneKind_FootL2    = 18,
        VriableIconBodyBoneKind_AnkleL    = 19,
        VriableIconBodyBoneKind_KneeL     = 20,
        VriableIconBodyBoneKind_FootR1    = 21,
        VriableIconBodyBoneKind_FootR2    = 22,
        VriableIconBodyBoneKind_AnkleR    = 23,
        VriableIconBodyBoneKind_KneeR     = 24,
        VriableIconBodyBoneKind_End       = 25
    };

    rio::Vector3f GetMatrixTranslation(rio::Matrix34f& mtx)
    {
        return { mtx.m[0][3], mtx.m[1][3], mtx.m[2][3] };
    }
    void SetMatrixTranslation(rio::Matrix34f& mtx, rio::Vector3f t)
    {
        mtx.m[0][3] = t.x; mtx.m[1][3] = t.y; mtx.m[2][3] = t.z;
    }
}

void BodyModel::initialize(Model* pModel, PantsColor pantsColor)
{
    mpModel = pModel;

    FFLiCharInfo* pCharInfo = &reinterpret_cast<FFLiCharModel*>(mpModel->getCharModel())->charInfo;
    mBodyScale = calcBodyScale(static_cast<f32>(pCharInfo->build), static_cast<f32>(pCharInfo->height));

    mBodyColor = FFLGetFavoriteColor(pCharInfo->favoriteColor);

    mPantsColor = pantsColor;

    if (mpBodyModel->useSkeleton())
    {
        mUseSkeleton = true;
        initializeSkeleton_();
    }
    else
    {
        // mHeadModelMatrix is set by initializeSkeleton_()
        // Without a skeleton, it is just translation
        mHeadModelMatrix.makeT(getHeadTranslation());
    }
}

rio::Vector3f BodyModel::getHeadTranslation()
{
    rio::Vector3f translate;

    if (mUseSkeleton)
    {
        // Skeleton path: use head bone matrix.
        const s32 bone = mpBodyModel->mHeadBoneID;
        rio::Matrix34f mat = mSkeletonMatrix[bone];

        // Extract translation and only scale that.
        translate = GetMatrixTranslation(mat);
        translate.setMul(translate, mScale); // scale translation
    }
    else
    {
        // Non-skeleton path: use relative translation from
        // mpBodyModel/BodyModelItem specified in CSV.
        translate = { 0.0f, mpBodyModel->mHeadYTranslate, 0.0f };
        // Scale by the same scale factors the model used.
        translate.setMul(translate, mBodyScale);
        translate.setMul(translate, mScale);
    }

    return translate;
}

rio::Matrix34f BodyModel::getHeadModelMatrix()
{
    return mHeadModelMatrix;
/*
    rio::Matrix34f mat;

    if (mUseSkeleton)
    {
        // Skeleton path: use head bone matrix.
        // The transformed head bone has to be used
        // rather than the original pre-transformation.
        const s32 bone = mpBodyModel->mHeadBoneID;
        mat = mSkeletonMatrix[bone];

        // Set scaled translation.
        rio::Vector3f translate = getHeadTranslation();
        SetMatrixTranslation(mat, translate);
    }
    else
    {
        // apply head translation
        mat.makeT(getHeadTranslation());
    }

    return mat;
*/
}



namespace
{

/*
enum BodyBoneScaleType
{
    BODY_BONE_SCALE_TYPE_NONE,
    BODY_BONE_SCALE_TYPE_XYZ,
    BODY_BONE_SCALE_TYPE_YXZ,
    BODY_BONE_SCALE_TYPE_XXX,
    BODY_BONE_SCALE_TYPE_TRANSLATION,
};
*/

// void nn::mii::detail::`anonymous namespace'::UpdateScale(class nn::util::Vector3f *, enum nn::mii::detail::VriableIconBodyBoneKind, struct nn::util::Float3 const &)
static void UpdateScale(rio::Vector3f &scaleOut, VriableIconBodyBoneKind bone, const rio::Vector3f &bodyScale)
{
    // miibodylow "3ds":
    //bone = static_cast<VriableIconBodyBoneKind>(bone + 1);
    switch (bone)
    {
    case VriableIconBodyBoneKind_AllRoot:   [[fallthrough]];
    case VriableIconBodyBoneKind_Body:      [[fallthrough]];
    case VriableIconBodyBoneKind_SklRoot:
        // Do not update scale.mSkeletonMatrix
        break;
    case VriableIconBodyBoneKind_Chest:     [[fallthrough]];
    // Includes neck, not the actual head:
    case VriableIconBodyBoneKind_Head:      [[fallthrough]];
    case VriableIconBodyBoneKind_Chest2:    [[fallthrough]];
    case VriableIconBodyBoneKind_Hip:       [[fallthrough]];
    case VriableIconBodyBoneKind_FootL1:    [[fallthrough]];
    case VriableIconBodyBoneKind_FootL2:    [[fallthrough]];
    case VriableIconBodyBoneKind_FootR1:    [[fallthrough]];
    case VriableIconBodyBoneKind_FootR2:
        // Chest, Hip, Foot: XYZ
        // Includes entire body except
        // for entire arms/hands and shoes.
        scaleOut.x = bodyScale.x;
        scaleOut.y = bodyScale.y;
        scaleOut.z = bodyScale.z;
        break;
    case VriableIconBodyBoneKind_ArmL1:     [[fallthrough]];
    case VriableIconBodyBoneKind_ArmL2:     [[fallthrough]];
    case VriableIconBodyBoneKind_ElbowL:    [[fallthrough]];
    case VriableIconBodyBoneKind_ArmR1:     [[fallthrough]];
    case VriableIconBodyBoneKind_ArmR2:     [[fallthrough]];
    case VriableIconBodyBoneKind_ElbowR:
        // Arm, Elbow: YXZ
        // Includes: Entire arms.
        scaleOut.x = bodyScale.y;
        scaleOut.y = bodyScale.x;
        scaleOut.z = bodyScale.z;
        break;
    case VriableIconBodyBoneKind_WristL:    [[fallthrough]];
    case VriableIconBodyBoneKind_ShoulderL: [[fallthrough]];
    case VriableIconBodyBoneKind_WristR:    [[fallthrough]];
    case VriableIconBodyBoneKind_ShoulderR: [[fallthrough]];
    case VriableIconBodyBoneKind_AnkleL:    [[fallthrough]];
    case VriableIconBodyBoneKind_KneeL:     [[fallthrough]];
    case VriableIconBodyBoneKind_AnkleR:    [[fallthrough]];
    case VriableIconBodyBoneKind_KneeR:
        // Wrist, Shoulder, Ankle, Knee: XXX (one dimension)
        // Includes:
        // * Shoulders
        // * Hand spheres
        // * Knees
        // * Shoes
        scaleOut.x = bodyScale.x;
        scaleOut.y = bodyScale.x;
        scaleOut.z = bodyScale.x;
        break;
    // NOTE: For the Head bone, real UpdateBodyScale function
    // uses XYZ scale but clamps Y to 1.0. However, this only
    // actually applies to the area of the body NEAR the
    // head, or the neck, and NOT for the head itself
    default:
        RIO_ASSERT(false && "UpdateScale: Unexpected bone ID passed in.");
    }
    return;
}


// NOTE: Only shares name with this function but
// not matching it whatsoever, the real StoreHeadWorldMatrix
// actually extracts ONLY translation from the head bone
// and applies that to identity matrix, dropping rotation
static void StoreHeadWorldMatrix(rio::Matrix34f* pOut, rio::Matrix34f& matrix, rio::Vector3f scale)
{
    *pOut = matrix;
    rio::Vector3f translate = GetMatrixTranslation(*pOut);
    translate.setMul(translate, scale);
    SetMatrixTranslation(*pOut, translate);
}

}
void BodyModel::calculateWorldMatrix_(rio::Matrix34f* localMatrices, const s32* parentBoneIDs, const s32 matrixCount, const rio::Vector3f bodyScale)
{
    // Meant to model the two loops done in: void nn::mii::detail::VariableIconBodyImpl::CalculateWorldMatrix(VariableIconBodyImpl *this,VariableIconBodyWorldMatrix *pOut,Gender gender,int build,int height);
    for (int bone = 0; bone < matrixCount; bone++)
    {
        const s32 parent = parentBoneIDs[bone];
        // Skip this bone if there is no parent:
        if (parent < 0)
            continue; // No scaling, rotation, etc.
        // mtx = localMatrices[bone]
        rio::Matrix34f& mtx = localMatrices[bone]; // mirror

        // localScale = scale difference in this bone.
        rio::Vector3f localScale = { 1.0f, 1.0f, 1.0f }; // Initialize
        // Get scale vector for PARENT BONE INDEX
        UpdateScale(localScale, (VriableIconBodyBoneKind)parent, bodyScale);

        // Update translation:

        // Get translation/W-axis from matrix.
        rio::Vector3f w = GetMatrixTranslation(mtx);

        // If this bone is skl_root (2), update translation.
        // Usually performed in: void nn::mii::detail::`anonymous namespace'::UpdateRotateTranslate(struct nn::util::general::MatrixRowMajor4x3fType *, enum nn::mii::detail::VriableIconBodyBoneKind, struct nn::util::Float3 const &)
        if (bone == VriableIconBodyBoneKind_SklRoot)
        {
            // Multiply translation by YYX axes:
            w.x *= bodyScale.y; // X by bodyScale.y
            w.y *= bodyScale.y; // Y by bodyScale.y
            w.z *= bodyScale.x; // Z by bodyScale.x

            // Add to Y translation from bodyScale and
            // scale factor of body model relative to world vvv
            w.y += ((bodyScale.x - bodyScale.y) * 1.0f);
                              // cBodyScaleFactor ^^^^ (orig. = 7.0f)
        }

        w.setMul(w, localScale); // Multiply: w.xyz * localScale.xyz
        // ^^ Equiv: w.x *= localScale.x; w.y *= localScale.y; w.z *= localScale.z;

        // Set translation on matrix: (maybe applyScaleWorld?)
        SetMatrixTranslation(mtx, w);

        // Multiply matrices:
        mtx.setMul(localMatrices[parent], mtx); // Multiply parent and local bone
    }

    for (int bone = 0; bone < matrixCount; bone++)
    {
        if (bone == mpBodyModel->mHeadBoneID)
            // If this is the head bone, capture pre-scale.
            StoreHeadWorldMatrix(&mHeadModelMatrix, localMatrices[bone], mScale);

        // localScale = scale difference in this bone.
        rio::Vector3f localScale = { 1.0f, 1.0f, 1.0f }; // Initialize
        // Get scale vector for this bone, not parent
        UpdateScale(localScale, (VriableIconBodyBoneKind)bone, bodyScale);

        // Usually performed in: void nn::mii::detail::`anonymous namespace'::MatrixScaleBase(struct nn::util::general::MatrixRowMajor4x3fType *, struct nn::util::general::MatrixRowMajor4x3fType const &, struct nn::util::general::Vector3fType const &)

        // Apply local scale on matrix. (Scale/Rotate/NOT translate)
        localMatrices[bone].applyScaleLocal(localScale);
    }
}


void BodyModel::initializeSkeleton_()
{
    s32 parentBoneIDs[sizeof(mSkeletonMatrix) / sizeof(rio::Matrix34f)];
    for (s32 i = 0; i < mpBodyModel->mBoneCount; i++)
    {
        mSkeletonMatrix[i] = mpBodyModel->mBones[i].localMatrix;
        parentBoneIDs[i] = mpBodyModel->mBones[i].parentID;
        //if (parentBoneIDs[i] < 0) continue;
        //mSkeletonMatrix[i].setMul(mSkeletonMatrix[parentBoneIDs[i]], mSkeletonMatrix[i]);
    }
    calculateWorldMatrix_(mSkeletonMatrix, parentBoneIDs, mpBodyModel->mBoneCount, mBodyScale);
}

rio::mdl::Model* BodyModel::getBodyModel_()
{
    FFLiCharInfo* pCharInfo = mpModel->getCharInfo();
    FFLGender genderTmp = FFLGender(pCharInfo->gender);

    // Clamp the value of gender.
    const FFLGender gender = FFLGender(genderTmp % FFL_GENDER_MAX);

    // Select body model based on gender.
    rio::mdl::Model* model = mpBodyModel->mpModels[gender];

    RIO_ASSERT(model); // make sure it is not null
    return model;
}


// draws mii body based on charinfo's build/height
// shader sets favorite and pants color
void BodyModel::draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx)
{
    //const BodyType bodyType = BODY_TYPE_WIIU_MIIBODYMIDDLE;

    const bool lightEnable = mpModel->getLightEnable();
    FFLiCharInfo* pCharInfo = mpModel->getCharInfo();
    //const FFLGender gender = pCharInfo->gender;

    const rio::mdl::Model* pModel = getBodyModel_();
    const rio::mdl::Mesh* meshes = pModel->meshes(); // Body and pants mesh.

    // Set blending options.
    rio::RenderState render_state;
    render_state.setBlendEnable(false); // Opaque blending.
    render_state.applyBlendAndFastZ();

    // Render each mesh in order
    for (u32 i = 0; i < pModel->numMeshes(); i++)
    {
        const rio::mdl::Mesh& mesh = meshes[i];

        // Bind shader and set body material.
        IShader* pShader = mpModel->getShader();
        pShader->bind(lightEnable, pCharInfo);

        if (mUseSkeleton)
            pShader->setBoneMatrix(mSkeletonMatrix, mpBodyModel->mBoneCount);

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
        if (mUseSkeleton)
            modelMtxBody.applyScaleLocal(mScale);
        else
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


// calculate vector in which body scaling is based off of
rio::Vector3f BodyModel::calcBodyScale(f32 build, f32 height)
{
    rio::Vector3f bodyScale;
    // calculated in this function: void __cdecl nn::mii::detail::`anonymous namespace'::GetBodyScale(struct nn::util::Float3 *, int, int)
    // in libnn_mii/draw/src/detail/mii_VariableIconBodyImpl.cpp
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

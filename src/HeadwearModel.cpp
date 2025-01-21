#include <HeadwearModel.h>
#include <Types.h>
#include <TGAHeader.h>
#include <Model.h>

#include <cstdio>
#include <gfx/mdl/res/rio_ModelCacher.h>
#include <filedevice/rio_FileDeviceMgr.h>

#include <nn/ffl/FFLiResourceLoaderBuffer.h>
#include <nn/ffl/FFLiResourceLoader.h>
#include <nn/ffl/FFLiManager.h>
#include <nn/ffl/FFLiTexture.h>
#include <nn/ffl/FFLiShape.h>

#if RIO_IS_WIN
#include <gpu/win/rio_Texture2DUtilWin.h>
#endif

HeadwearModel::HeadwearModel(HeadwearItem* pItem)
    : mpModel(nullptr)
    , mpHeadModel(nullptr)
    , mpHeadwearItem(nullptr)
    //, mHeadwearType(modelType)
    //, mScale{ 1.0f, 1.0f, 1.0f }
    , mpTexture(nullptr)
    , mpShader(nullptr)
    , mAccessoryColor{ 0.0f, 0.0f, 0.0f, 1.0f }
    , mAccessoryColorIndex(FFL_FAVORITE_COLOR_BLACK)
    , mPartsTransform()
{
    rio::MemUtil::set(&mResourceModel, 0, sizeof(mResourceModel));

    RIO_ASSERT(pItem);
    mpHeadwearItem = pItem;

    char modelPath[256];

    // fill modelPath with formatted filename
    // fill cHeadwearFileNameFormat with id, name
    snprintf(modelPath, sizeof(modelPath), cHeadwearFileNameFormat, mpHeadwearItem->id);//, mpHeadwearItem->name.c_str());

    const rio::mdl::res::Model* resModel = rio::mdl::res::ModelCacher::instance()->loadModel(modelPath, modelPath);
    if (resModel == nullptr)
        RIO_LOG("WARNING: resModel for headwear model path \"%s\" (id %d) == nullptr\n", modelPath, mpHeadwearItem->id);

    mpModel = new rio::mdl::Model(resModel);

    if (!mpHeadwearItem->texturePath.empty())
        loadTexture_();
}

HeadwearModel::~HeadwearModel()
{
    if (mpModel != nullptr)
        delete mpModel;

    if (mpTexture != nullptr)
        FFLiDeleteTexture(&mpTexture, false);
    if (mResourceModel.pShapeData != nullptr)
        FFLiDeleteShape(&mResourceModel.pShapeData, &mResourceModel.drawParam);
}

#define MODEL_FLAGS_CLEAR 0xf

// Modifies CharInfo and model create flag based on
// HeadwearType, meant for model creation to call into.
void HeadwearModel::modifyCharInfoAndFlag(FFLiCharInfo* pCharInfo, u32* pModelFlag)
{
    switch (mpHeadwearItem->type)
    {
        case HEADWEAR_TYPE_HAT:
            // clear existing flags
            *pModelFlag &= ~MODEL_FLAGS_CLEAR;
            *pModelFlag |= FFL_MODEL_FLAG_HAT;
            break;
        case HEADWEAR_TYPE_SKINHEAD:
            // Change hair type to bald (30/empty).
            pCharInfo->parts.hairType = FFL_HAIR_TYPE_EMPTY;
            break;
        //case HEADWEAR_TYPE_HEADGEAR: // aka helmets
            //*pModelFlag &= ~MODEL_FLAGS_CLEAR;
            //*pModelFlag |= FFL_MODEL_TYPE_FACE_ONLY;
            //break;
        //case HEADWEAR_TYPE_FRONT:
        //case HEADWEAR_TYPE_SIDE:
        //case HEADWEAR_TYPE_TOP:
        case HEADWEAR_TYPE_NORMAL:
            break; // no action needed
        default:
            RIO_ASSERT(false); // unexpected default
            break;
    }
}

// matrix of the headgear to adjust it to the head
rio::Matrix34f HeadwearModel::getHeadwearMatrix_(bool flip)
{
    rio::Matrix34f mtx = rio::Matrix34f::ident;

    switch (mpHeadwearItem->type)
    {
        //case HEADWEAR_TYPE_HEADGEAR: // hatTranslate
        case HEADWEAR_TYPE_SKINHEAD: // hatTranslate
            [[fallthrough]];
        case HEADWEAR_TYPE_NORMAL:   // hatTranslate
            [[fallthrough]];
        case HEADWEAR_TYPE_HAT:      // hatTranslate
        {
            // apply translation to mtx
            rio::Matrix34f translate;
            translate.makeT({ mPartsTransform.hatTranslate.x, mPartsTransform.hatTranslate.y, mPartsTransform.hatTranslate.z });

            mtx.setMul(mtx, translate);
            break;
        }
        //case HEADWEAR_TYPE_FRONT: // headFrontRotate/Translate
        //case HEADWEAR_TYPE_SIDE:  // headSideRotate/Translate
        /*
        {
            // get rotate and translate vectors
            rio::BaseVec3f rotate = { mPartsTransform.headSideRotate.x, mPartsTransform.headSideRotate.y, mPartsTransform.headSideRotate.z
            };
            rio::BaseVec3f translate = { mPartsTransform.hatTranslate.x, mPartsTransform.hatTranslate.y, mPartsTransform.hatTranslate.z };
            rio::Matrix34f rotateTranslate;
            // note for second set (right): need to
            // flip x axis on translate, y z axes on rotate
            if (flip)
            {
                // flip YZ rotation
                rotate.y *= -1.0f;
                rotate.z *= -1.0f;
                // flip X translation
                translate.x *= -1.0f;
            }

            rotateTranslate.makeRT(rotate, translate);
            mtx.setMul(mtx, rotateTranslate);
            break;
        }
        */
        //case HEADWEAR_TYPE_TOP:   // headTopRotate/Translate
        default:
            RIO_ASSERT(false); // unexpected default
            break;
    }

    return mtx;
}

void HeadwearModel::loadTexture_()
{
    //RIO_LOG("headwear item has texture path: %s\n", mpHeadwearItem->texturePath.c_str());
    rio::FileDevice::LoadArg arg;
    arg.path = cHeadwearDir + mpHeadwearItem->texturePath;

    // not the native file device so it will be in fs/content/
    const u8* data = rio::FileDeviceMgr::instance()->tryLoad(arg);

    if (data == nullptr)
    {
        RIO_LOG("NativeFileDevice failed to load when trying to load headwear texture: %s\n", arg.path.c_str());
        return;
    }

    const TGAHeader* header = reinterpret_cast<const TGAHeader*>(data);
    const u32 width = header->width;
    const u32 height = header->height;

    rio::TextureFormat format;
    switch (header->bitsPerPixel)
    {
    case 8:
        format = rio::TEXTURE_FORMAT_R8_UNORM;
        break;
    case 32:
        format = rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM;
        break;
    default:
        RIO_LOG("unknown tga bpp %d for tga file %s\n", header->bitsPerPixel, arg.path.c_str());
        format = rio::TEXTURE_FORMAT_R8_G8_B8_A8_UNORM;
    }

    const u32 imageSize = rio::Texture2DUtil::calcImageSize(format, width, height);

    RIO_ASSERT(arg.read_size >=
            (imageSize + sizeof(TGAHeader)));

    mpTexture = new rio::Texture2D(format, width, height, 1); // numMips = 1
    const rio::NativeTexture2DHandle handle = mpTexture->getNativeTextureHandle();

    // skip past tga header...
    const u8* dataPastTGAHeader = data + sizeof(TGAHeader);

    rio::NativeTextureFormat nativeFormat;
    rio::TextureFormatUtil::getNativeTextureFormat(nativeFormat, format);
    // calls glBindTexture, glTexImage2D
    rio::Texture2DUtil::uploadTexture(handle, format, nativeFormat,
                                      width, height, imageSize,
                                      dataPastTGAHeader);

    delete[] data;
}

void HeadwearModel::initialize(Model* pHeadModel, FFLFavoriteColor accessoryColor)
{
    mpHeadModel = pHeadModel;

    mPartsTransform = mpHeadModel->getPartsTransform();

    //FFLiCharInfo* pCharInfo = &reinterpret_cast<FFLiCharModel*>(mpHeadModel->getCharModel())->charInfo;
    mAccessoryColorIndex = accessoryColor;
    mAccessoryColor = FFLGetFavoriteColor(mAccessoryColorIndex);

    mHeadwearMtx = getHeadwearMatrix_();
    // for right side if using side headwear type
    //if (mpHeadwearItem->type == HEADWEAR_TYPE_SIDE)
    //    mHeadwearMtxFlip = getHeadwearMatrix_(true);
/*
    // texture from FFL resource ----------------
    const FFLiResourceManager& manager = FFLiManager::GetInstance()->GetResourceManager();

    FFLiCharModel* pCharModel = reinterpret_cast<FFLiCharModel*>(mpHeadModel->getCharModel());
    static const FFLResourceType resourceType = pCharModel->charModelDesc.resourceType;
    FFLiResourceLoaderBuffer resLoaderBuffer(&manager, resourceType);

    FFLiResourceLoader resLoader(const_cast<FFLiResourceManager*>(&manager), &resLoaderBuffer, resourceType);

    [[maybe_unused]] FFLResult result = FFLiLoadTextureWithAllocate(&mpTexture, FFLI_TEXTURE_PARTS_TYPE_CAP, 34, &resLoader);
    RIO_ASSERT(result == FFL_RESULT_OK);
    // shape from FFL resource ------------------
    result = FFLiLoadShape(&mResourceModel.pShapeData, &mResourceModel.drawParam, &mResourceModel.boundingBox, pCharModel, FFLI_SHAPE_PARTS_TYPE_HAT_NORMAL, 34, &resLoader);
    RIO_ASSERT(result == FFL_RESULT_OK);

    // draw shape -------------------------------
    //FFLiManager::GetInstance()->GetShaderCallback().CallDraw(&mResourceModel.drawParam);
    // ------------------------------------------
*/
}

void HeadwearModel::prepareDraw_(rio::Matrix34f& localMtx, rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx, bool lightEnable, FFLiCharInfo* pCharInfo, IShader* pShader)
{
    pShader->bind(lightEnable, pCharInfo);
    FFLModulateMode modulateMode = FFL_MODULATE_MODE_CONSTANT; // no texture
    if (mpTexture != nullptr) // if it exists use alpha opa
        modulateMode = FFL_MODULATE_MODE_ALPHA_OPA;

    const FFLModulateParam modulateParam = {
        modulateMode,
        FFL_MODULATE_TYPE_SHAPE_CAP, // use cap material
        &mAccessoryColor, // constant color (R)
        nullptr,  // no color G
        nullptr,  // no color B
        mpTexture // bind texture if exists
    };

    // hack: need to set favorite color
    const FFLFavoriteColor originalFavoriteColor = pCharInfo->favoriteColor;
    pCharInfo->favoriteColor = mAccessoryColorIndex;

    pShader->setModulate(modulateParam);
    if (mResourceModel.pShapeData != nullptr)
    {
        // Copy modulateParam to mResourceModel.
        rio::MemUtil::copy(&mResourceModel.drawParam.modulateParam, &modulateParam, sizeof(FFLModulateParam));
    }

    pCharInfo->favoriteColor = originalFavoriteColor;

    // make new matrix for headwear
    rio::Matrix34f modelMtxHat = rio::Matrix34f::ident;
    // apply original model matrix (rotation)
    modelMtxHat.setMul(model_mtx, modelMtxHat);
    // apply rotate/translate for headwear
    modelMtxHat.setMul(modelMtxHat, localMtx);

    pShader->setViewUniform(modelMtxHat, view_mtx, proj_mtx);

    rio::RenderState render_state;
    // this does not account for ANYTHING but back cull
    render_state.setCullingMode(rio::Graphics::CULLING_MODE_BACK);
    render_state.applyCullingAndPolygonModeAndPolygonOffset();
}

void HeadwearModel::draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx)
{
    //RIO_LOG("hat translate: %f, %f, %f\n", mPartsTransform.hatTranslate.x, mPartsTransform.hatTranslate.y, mPartsTransform.hatTranslate.z);

    const bool lightEnable = mpHeadModel->getLightEnable();
    FFLiCharInfo* pCharInfo = mpHeadModel->getCharInfo();
    IShader* pShader = mpHeadModel->getShader();

    // Draw from mpModel.

    const rio::mdl::Mesh* meshes = mpModel->meshes();
    for (u32 i = 0; i < mpModel->numMeshes(); i++)
    {
        rio::Matrix34f& localMtx = mHeadwearMtx;
        prepareDraw_(localMtx, model_mtx, view_mtx, proj_mtx, lightEnable, pCharInfo, pShader);

        meshes[i].draw();
    }

/*
    // Draw from mResourceModel.
    rio::Matrix34f localMtx = mHeadwearMtx;
    localMtx.applyScaleLocal({ 1.1f, 1.1f, 1.1f });
    prepareDraw_(localMtx, model_mtx, view_mtx, proj_mtx, lightEnable, pCharInfo, pShader);
    FFLiManager::GetInstance()->GetShaderCallback().CallDraw(&mResourceModel.drawParam);
*/
}

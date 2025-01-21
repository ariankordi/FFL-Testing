#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>
#include <gfx/mdl/rio_Model.h>

#include <IShader.h>
#include <Types.h>
#include <string>

class Model;

// for HeadwearModel, determines what modifications
// are made when creating the head model, should be done
enum HeadwearType
{
    HEADWEAR_TYPE_NONE   = -1, // Indicates stub/dummy
    // == Using hatTranslate: (all for now)
    HEADWEAR_TYPE_NORMAL, // On top of normal head model
    HEADWEAR_TYPE_HAT,    // Uses FFL_MODEL_TYPE_HAT
    // (Language used by Miitomo:)
    HEADWEAR_TYPE_SKINHEAD, // Hair replaced w/ FFL_HAIR_TYPE_EMPTY

    // Potentially to be added:
    // (order: front, side, top, hat, headgear)
    // * HEADGEAR (helmet, bear...) using FFL_MODEL_TYPE_FACE_ONLY
    // * ApplyHairMaskWhenSkinhead = cap hair?
    // == Using headFrontRotate/Translate
    // * FRONT (headband...)
    // == Using headSideRotate/Translate
    // * SIDE (cat ears...) on both sides
    //   - should there be single side? (default=left)
    // == Using headTopRotate/Translate
    // * TOP (crown...)

    HEADWEAR_TYPE_MAX
};

// Strings for the above types to be used in the CSV table.
inline const char* cHeadwearTypeStrings[HEADWEAR_TYPE_MAX] = {
    "normal",
    //"front",
    //"side",
    //"top",
    "hat",
    //"headgear",
    "skinhead",
};

// Format for headwear filenames.
inline const char* cHeadwearFileNameFormat = "headwear/hat_%d";
inline const char* cHeadwearDir = "models/headwear/";

// Structure for each entry in the headwear table.
struct HeadwearItem
{
    s32          id;
    std::string  name;        // File and server name
    HeadwearType type;        // Parsed from string
    std::string  texturePath; // Albedo texture
    //std::string  normalTexturePath; // Bump map texture
    //std::string  maskTexturePath;   // Skin  / hair mask
                                    // Green / blue colors
};

class HeadwearModel
{
public:
    //HeadwearModel(const char* modelPath, HeadwearType modelType);
    HeadwearModel(HeadwearItem* pItem);
    ~HeadwearModel();

    void initialize(Model* pHeadModel, FFLFavoriteColor accessoryColor);
    void modifyCharInfoAndFlag(FFLiCharInfo* pCharInfo, u32* pModelFlag);

    rio::Matrix34f getModelMatrix();

    void draw(rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx,
    rio::BaseMtx44f& proj_mtx);
private:
    void loadTexture_();
    rio::Matrix34f getHeadwearMatrix_(bool flip = false);
    void prepareDraw_(rio::Matrix34f& localMtx, rio::Matrix34f& model_mtx, rio::BaseMtx34f& view_mtx, rio::BaseMtx44f& proj_mtx, bool lightEnable, FFLiCharInfo* pCharInfo, IShader* pShader);

    const rio::mdl::Model* mpModel;
    Model*                 mpHeadModel;
    HeadwearItem*          mpHeadwearItem;

    rio::Vector3f          mScale;
    rio::Matrix34f         mHeadwearMtx;
    //rio::Matrix34f         mHeadwearMtxFlip;
    rio::Texture2D*        mpTexture;
    IShader*               mpShader;
    FFLColor               mAccessoryColor;
    FFLFavoriteColor       mAccessoryColorIndex;
    FFLPartsTransform      mPartsTransform;
    struct {
        void*          pShapeData;
        FFLBoundingBox boundingBox; // unused by us
        FFLDrawParam   drawParam;
    } mResourceModel;
};

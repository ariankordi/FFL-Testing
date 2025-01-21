#pragma once

#include <math/rio_MathTypes.h>

#include <nn/ffl.h>
#include <gfx/mdl/rio_Model.h>

#include <IShader.h>
#include <Types.h>
#include <BodyTypes.h>

#include <string>
#include <sstream>
//#include <fstream>
#include <vector>
#include <filedevice/rio_FileDeviceMgr.h>
#include <gfx/mdl/res/rio_ModelCacher.h>

class Model;

struct BodyBone
{
    s32            poseID;
    s32            parentID;
    rio::Matrix34f localMatrix;
    //BoneScaleType  scaleType;
};

class BodyModelItem
{
public:
    BodyType              mType;
    std::string           mName;
    f32                   mScale;
    f32                   mHeadYTranslate;
    s32                   mHeadBoneID;
    s32                   mBoneCount;

    rio::mdl::Model*      mpModels[FFL_GENDER_MAX];
    std::vector<BodyBone> mBones;

    ~BodyModelItem()
    {
        for (u32 gender = 0; gender < FFL_GENDER_MAX; gender++)
            delete mpModels[gender];
    }

    bool useSkeleton() { return mBoneCount > 0; }

    static bool populateArrayFromCSV(BodyModelItem list[BODY_TYPE_MAX], const char* filePath)
    {
        RIO_LOG("loading body models: ");
        rio::FileDevice::LoadArg arg;
        arg.path = filePath;

        // in fs/content
        char* data = reinterpret_cast<char*>(
            rio::FileDeviceMgr::instance()->tryLoad(arg));

        if (data == nullptr)
        {
            RIO_LOG("\nFailed to load %s. Cannot load any body models, will probably crash now.\n", arg.path.c_str());
            return false; // body models unavailable
        }

        data[arg.read_size - 1] = '\0'; // null-terminate

        std::istringstream fileStream(data);
        std::string line;

        // get and ignore first line (header)
        std::getline(fileStream, line);
        // for each type, get line
        for (s32 type = 0; type < BODY_TYPE_MAX; type++)
        {
            std::getline(fileStream, line);
            RIO_ASSERT(!line.empty());

            BodyModelItem& item = list[type];
            std::stringstream ss(line);
            std::string field;

            if (!std::getline(ss, field, ','))
                continue;
            else
            {
                item.mType = static_cast<BodyType>(std::stoi(field));
                RIO_ASSERT(item.mType == type);
            }

            std::getline(ss, field, ',');
                item.mName = field;

            std::getline(ss, field, ',');
                item.mScale = std::stof(field);

            std::getline(ss, field, ',');
                item.mHeadYTranslate = std::stof(field);

            std::getline(ss, field, ',');
                item.mHeadBoneID = std::stoi(field);

            std::getline(ss, field, ',');
                item.mBoneCount = std::stoi(field);

            for (u32 gender = 0; gender < FFL_GENDER_MAX; gender++)
                item.mpModels[gender] = nullptr;

            item.loadModel_();
            if (item.mBoneCount > 0)
            {
                item.mBones.resize(item.mBoneCount);
                if (!item.loadSkeleton_())
                {
                    RIO_LOG("Failed to load skeleton for model %s, failing.\n", item.mName.c_str());
                    return false;
                }
            }
        }

        // print bold/blue:
        RIO_LOG("\033[1m(all loaded successfully)\033[0m\n");

        rio::MemUtil::free(data);
        return true; // loading successful
    }

private:
    void loadModel_()
    {
        for (u32 gender = 0; gender < FFL_GENDER_MAX; gender++)
        {
            const char* bodyTypeString = mName.c_str();
            const char* genderString = cBodyGenderStrings[gender];

            char bodyPathC[128];

            if (useSkeleton())
                snprintf(bodyPathC, sizeof(bodyPathC), cBodyFileNameFormat, bodyTypeString, genderString);
            else
                snprintf(bodyPathC, sizeof(bodyPathC), cStaticBodyFileNameFormat, bodyTypeString, genderString);

            RIO_LOG("%s, ", bodyPathC);
            const rio::mdl::res::Model* resModel = rio::mdl::res::ModelCacher::instance()->loadModel(bodyPathC, bodyPathC);

            RIO_ASSERT(resModel);
            if (resModel == nullptr)
            {
                RIO_LOG("Body model not found: %s\n", bodyPathC);
                return;
            }

            mpModels[gender] = new rio::mdl::Model(resModel);
        }
    }
    bool loadSkeleton_()
    {
        rio::FileDevice::LoadArg arg;
        const char* bodyTypeString = mName.c_str();

        char pathC[256];
        snprintf(pathC, sizeof(pathC), cBodySkeletonFileNameFormat, bodyTypeString);
        arg.path = pathC;

        // in fs/content
        char* data = reinterpret_cast<char*>(
            rio::FileDeviceMgr::instance()->tryLoad(arg));

        if (data == nullptr)
        {
            RIO_LOG("Body skeleton csv could not be opened: %s\n", pathC);
            return false;
        }
        data[arg.read_size - 1] = '\0';

        std::istringstream fileStream(data);
        //std::ifstream fileStream(path);
        std::string line;

        // get and ignore first line (header)
        std::getline(fileStream, line);
        // get line for each bone
        for (s32 i = 0; i < mBoneCount; i++)
        {
            std::getline(fileStream, line);
            RIO_ASSERT(!line.empty());

            BodyBone& bone = mBones[i];
            std::stringstream ss(line);
            std::string field;

            // boneId,parentBoneId,m00,m01,...,m33

            if (!std::getline(ss, field, ','))
                continue;
            else
            {
                [[maybe_unused]] s32 boneID = std::stoi(field);
                RIO_ASSERT(boneID == i);
            }

            std::getline(ss, field, ',');
                bone.parentID = std::stoi(field);

            // Set each field in the matrix.
            static const s32 floatCount = static_cast<s32>(
                sizeof(rio::Matrix34f) / sizeof(f32)
            );
            // Parse from CSV into rio::Matrix34f format.
            for (s32 j = 0; j < floatCount; j++)
            {
                std::getline(ss, field, ',');
                bone.localMatrix.a[j] = std::stof(field);
            }

        }

        rio::MemUtil::free(data);
        return true;
    }
};

class BodyModel
{
public:
    BodyModel(BodyModelItem* pItem);
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
    rio::mdl::Model* getBodyModel_();
    void initializeSkeleton_();

    Model*                 mpModel;
    BodyModelItem*         mpBodyModel;

    rio::Vector3f          mScale;
    rio::Vector3f          mBodyScale;
    IShader*               mpShader;
    FFLColor               mBodyColor; // aka favorite color
    BodyType               mBodyType;
    PantsColor             mPantsColor;

    bool                   mUseSkeleton;
    rio::Matrix34f         mSkeletonMatrix[65];
};

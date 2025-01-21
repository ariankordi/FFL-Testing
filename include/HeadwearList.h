#include <vector>
#include <sstream>

#include <HeadwearModel.h>
#include <string>
#include <filedevice/rio_FileDeviceMgr.h>

class HeadwearList
{
private:
    std::vector<HeadwearItem> mHeadwearItems;

    HeadwearType stringToHeadwearType(const char* typeString)
    {
        if (strlen(typeString) <= 0)
            // If the type field is blank, it's negative.
            return HEADWEAR_TYPE_NONE;

        for (int i = 0; i < HEADWEAR_TYPE_MAX; ++i)
            if (strcmp(typeString, cHeadwearTypeStrings[i]) == 0)
                return static_cast<HeadwearType>(i);

        // no match found, should not happen
        if (strlen(typeString) > 0)
            RIO_LOG("unknown headwear type: %s\n", typeString);
        return HEADWEAR_TYPE_NORMAL;
    }

public:
    bool loadFromCSV(const char* filePath)
    {
        rio::FileDevice::LoadArg arg;
        arg.path = filePath;

        // in fs/content
        char* data = reinterpret_cast<char*>(
            rio::FileDeviceMgr::instance()->tryLoad(arg));

        if (data == nullptr)
        {
            RIO_LOG("Could not load %s, no headwear available.\n", arg.path.c_str());
            return false; // headwear unavailable
        }

        data[arg.read_size - 1] = '\0'; // null-terminate

        std::istringstream fileStream(data);
        std::string line;

        // get and ignore first line (header)
        std::getline(fileStream, line);
        // iterate through each line
        while (std::getline(fileStream, line))
        {
            HeadwearItem item;
            std::stringstream ss(line);
            std::string field;

            if (!std::getline(ss, field, ','))
                continue;
            else
                item.id = std::stoi(field);

            std::getline(ss, field, ',');
                item.name = field;

            std::getline(ss, field, ',');
                item.type = stringToHeadwearType(field.c_str());

            std::getline(ss, field, ',');
                item.texturePath = field;

            mHeadwearItems.push_back(item);
        }

        rio::MemUtil::free(data);
        return true; // loading successful
    }

    HeadwearItem* getByID(int id)
    {
        for (HeadwearItem& item : mHeadwearItems)
            if (item.id == id
            && item.type > HEADWEAR_TYPE_NONE)
                return &item;

        return nullptr;
    }
};

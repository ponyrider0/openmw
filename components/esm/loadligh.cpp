#include "loadligh.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Light::sRecordId = REC_LIGH;

    void Light::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'M','O','D','L'>::value:
                    mModel = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'L','H','D','T'>::value:
                    esm.getHT(mData, 24);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                    mSound = esm.getHString();
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing LHDT subrecord");
    }
    void Light::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("ITEX", mIcon);
        esm.writeHNT("LHDT", mData, 24);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("SNAM", mSound);
    }
	bool Light::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// MODL == Model Filename
		if (mModel.size() > 4)
		{
//			tempStr = esm.generateEDIDTES4(mModel, 1);
//			tempStr.replace(tempStr.size()-4, 4, ".nif");
//			tempPath << "lights\\morro\\" << tempStr;
			tempPath << esm.substituteLightModel(mId, 0);
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("MODL");
			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(0.0);
			esm.endSubRecordTES4("MODB");
			// MODT
		}

		// SCRI (script formID) mScript
		tempFormID = esm.crossRefStringID(mScript);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		if (mName.size() > 0)
		{
			esm.startSubRecordTES4("FULL");
			esm.writeHCString(mName);
			esm.endSubRecordTES4("FULL");
		}

		// ICON, mIcon
		if (mIcon.size() > 4)
		{
//			tempStr = esm.generateEDIDTES4(mIcon, 1);
//			tempStr.replace(tempStr.size()-4, 4, ".dds");
			tempPath.str(""); tempPath.clear();
//			tempPath << "lights\\morro\\" << tempStr;
			tempPath << esm.substituteLightModel(mId, 1);
			esm.startSubRecordTES4("ICON");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("ICON");
		}

		// DATA, {time, radius, color, flags, falloff, fov, value, weight}
		esm.startSubRecordTES4("DATA");
		esm.writeT<int32_t>(mData.mTime);
		esm.writeT<uint32_t>(mData.mRadius);
		esm.writeT<uint32_t>(mData.mColor);
		esm.writeT<uint32_t>(mData.mFlags);
		esm.writeT<float>(1); // falloff exponent
		esm.writeT<float>(90); // FOV
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// FNAM, fade value
		esm.startSubRecordTES4("FNAM");
		esm.writeT<float>(0.33);
		esm.endSubRecordTES4("FNAM");

		// SNAM (sound formID)
		tempFormID = esm.crossRefStringID(mSound);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SNAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SNAM");
		}

		return true;
	}

    void Light::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mTime = 0;
        mData.mRadius = 0;
        mData.mColor = 0;
        mData.mFlags = 0;
        mSound.clear();
        mScript.clear();
        mModel.clear();
        mIcon.clear();
        mName.clear();
    }
}

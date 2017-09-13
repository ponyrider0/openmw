#include "loadappa.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Apparatus::sRecordId = REC_APPA;

    void Apparatus::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'A','A','D','T'>::value:
                    esm.getHT(mData);
                    hasData = true;
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'I','T','E','X'>::value:
                    mIcon = esm.getHString();
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
            esm.fail("Missing AADT subrecord");
    }

    void Apparatus::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNCString("FNAM", mName);
        esm.writeHNT("AADT", mData, 16);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNCString("ITEX", mIcon);
    }
	bool Apparatus::exportTESx(ESMWriter &esm, int export_format) const
	{
		std::string tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		tempPath << "clutter\\magesguild\\morro\\" << tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MODL");

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(30.0);
		esm.endSubRecordTES4("MODB");

		// MODT
		// ICON, mIcon
		tempStr = esm.generateEDIDTES4(mIcon, 1);
		tempStr.replace(tempStr.size()-4, 4, ".dds");
		tempPath.str(""); tempPath.clear();
		tempPath << "clutter\\magesguild\\morro\\" << tempStr;
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint8_t>(mData.mType);
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.writeT<float>(mData.mQuality);
		esm.endSubRecordTES4("DATA");

		// SCRI (script formID)
		std::string strScript = esm.generateEDIDTES4(mScript);
		if (Misc::StringUtils::lowerCase(strScript).find("sc", strScript.size()-2) == strScript.npos)
		{
			strScript += "Script";
		}
		uint32_t tempFormID = esm.crossRefStringID(strScript, false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		return true;
	}

    void Apparatus::blank()
    {
        mData.mType = 0;
        mData.mQuality = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mName.clear();
    }
}

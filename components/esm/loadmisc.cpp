#include "loadmisc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Miscellaneous::sRecordId = REC_MISC;

    void Miscellaneous::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'M','C','D','T'>::value:
                    esm.getHT(mData, 12);
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
            esm.fail("Missing MCDT subrecord");
    }

    void Miscellaneous::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("MCDT", mData, 12);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
    }

	bool Miscellaneous::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, 0, "MISC");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		if (mName.size() > 0)
		{
			esm.startSubRecordTES4("FULL");
			esm.writeHCString(mName);
			esm.endSubRecordTES4("FULL");
		}

		// MODL == Model Filename
		if (mModel.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mModel, 1);
			tempStr.replace(tempStr.size()-4, 4, ".nif");
			tempPath << "clutter\\morro\\" << tempStr;
			esm.QueueModelForExport(mModel, tempPath.str(), 3);
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("MODL");
			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(5.0);
			esm.endSubRecordTES4("MODB");
			// MODT
		}

		// ICON, mIcon
		if (mIcon.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mIcon, 1);
			tempStr.replace(tempStr.size()-4, 4, ".dds");
			tempPath.str(""); tempPath.clear();
			tempPath << "clutter\\morro\\" << tempStr;
			esm.mDDSToExportList.push_back(std::make_pair(mIcon, std::make_pair(tempPath.str(), 1)));
//			esm.mDDSToExportList[mIcon] = std::make_pair(tempPath.str(), 1);
			esm.startSubRecordTES4("ICON");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("ICON");
		}

		// SCRI (script formID) mScript
		std::string strScript = esm.generateEDIDTES4(mScript, 3);
		tempFormID = esm.crossRefStringID(strScript, "SCPT", false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// DATA
		esm.startSubRecordTES4("DATA");
		esm.writeT<uint32_t>(mData.mValue);
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		return true;
	}

    void Miscellaneous::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mIsKey = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}

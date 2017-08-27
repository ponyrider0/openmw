#include "loadalch.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Potion::sRecordId = REC_ALCH;

    void Potion::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mEffects.mList.clear();

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
                case ESM::FourCC<'T','E','X','T'>::value: // not ITEX here for some reason
                    mIcon = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'A','L','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    mEffects.add(esm);
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
            esm.fail("Missing ALDT subrecord");
    }
    void Potion::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("TEXT", mIcon);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("ALDT", mData, 12);
        mEffects.save(esm);
    }
	bool Potion::exportTESx(ESMWriter &esm, int export_format) const
	{
		std::string *tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, false);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(*tempStr);
		esm.endSubRecordTES4("EDID");
		delete tempStr;

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, true);
		tempStr->replace(tempStr->size()-4, 4, ".nif");
		tempPath << "clutter\\potions\\morro\\" << *tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MODL");
		delete tempStr;

		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(1.0);
		esm.endSubRecordTES4("MODB");

		// MODT
		// ICON, mIcon
		tempStr = esm.generateEDIDTES4(mIcon, true);
		tempStr->replace(tempStr->size()-4, 4, ".dds");
		tempPath.str(""); tempPath.clear();
		tempPath << "clutter\\potions\\morro\\" << *tempStr;
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");
		delete tempStr;

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// SCRI --mScript (formID)
		uint32_t tempFormID = esm.crossRefStringID(mScript);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// ENIT {long, byte[1], byte[3]} -- flags
		// EFID string[4] (magic effect ID)
		// EFIT {string[4], long, long, long, long, long}
		// SCIT {formID, long, string[4], byte[1], byte[3]}

		return true;
	}

    void Potion::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mAutoCalc = 0;
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
        mEffects.mList.clear();
    }
}

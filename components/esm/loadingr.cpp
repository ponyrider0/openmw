#include "loadingr.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Ingredient::sRecordId = REC_INGR;

    void Ingredient::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'I','R','D','T'>::value:
                    esm.getHT(mData, 56);
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
            esm.fail("Missing IRDT subrecord");

        // horrible hack to fix broken data in records
        for (int i=0; i<4; ++i)
        {
            if (mData.mEffectID[i] != 85 &&
                mData.mEffectID[i] != 22 &&
                mData.mEffectID[i] != 17 &&
                mData.mEffectID[i] != 79 &&
                mData.mEffectID[i] != 74)
            {
                mData.mAttributes[i] = -1;
            }

            // is this relevant in cycle from 0 to 4?
            if (mData.mEffectID[i] != 89 &&
                mData.mEffectID[i] != 26 &&
                mData.mEffectID[i] != 21 &&
                mData.mEffectID[i] != 83 &&
                mData.mEffectID[i] != 78)
            {
                mData.mSkills[i] = -1;
            }
        }
    }

    void Ingredient::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("IRDT", mData, 56);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);
    }

	bool Ingredient::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		tempStr = esm.generateEDIDTES4(mId, 0, "INGR");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		tempPath << "clutter\\morro\\" << tempStr;
		esm.QueueModelForExport(mModel, tempPath.str());
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MODL");
		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(10.0);
		esm.endSubRecordTES4("MODB");

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

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// ENIT
		esm.startSubRecordTES4("ENIT");
		esm.writeT<int32_t>(mData.mValue);
		int flags=0;
		flags |= 0x01; // no auto-calc
		flags |= 0x02; // food item
		esm.writeT<int8_t>(flags);
		esm.writeT<uint16_t>(0); // unused
		esm.writeT<int8_t>(0); // unused
		esm.endSubRecordTES4("ENIT");

		// wbEffects
//		for (auto magicEffect = mEffects.mList.begin(); magicEffect != mEffects.mList.end(); magicEffect++)
		for (int i=0; i < 4; i++)
		{
			if ( (mData.mEffectID[i] != 0) && (mData.mEffectID[i] != -1) )
			{
				std::string effSIG = esm.intToMagEffIDTES4(mData.mEffectID[i]);
				// EFID
				esm.startSubRecordTES4("EFID");
				// write char[4] magic effect
				esm.writeName(effSIG);
				esm.endSubRecordTES4("EFID");

				// EFIT
				esm.startSubRecordTES4("EFIT");
				// write char[4] magic effect
				esm.writeName(effSIG);
				// magnitude (uint32)
				esm.writeT<uint32_t>(35);
				// area (uint32)
				esm.writeT<uint32_t>(0);
				// duration (uint32)
				esm.writeT<uint32_t>(0);
				// type (uint32) -- enum{self, touch, target}
				esm.writeT<uint32_t>(0);
				// actor value (int32)
				int32_t actorVal=-1;
				if (mData.mSkills[i] != -1)
				{
					actorVal = esm.skillToActorValTES4(mData.mSkills[i]);
				}
				else if (mData.mAttributes[i] != -1)
				{
					actorVal = esm.attributeToActorValTES4(mData.mAttributes[i]);
				}
				esm.writeT<int32_t>(actorVal);
				esm.endSubRecordTES4("EFIT");

				// SCIT
				bool isScriptEffect = (effSIG == "SEFF");
				if (isScriptEffect)
				{
					esm.startSubRecordTES4("SCIT");
					// formID (SCPT)
					tempFormID = 0x1470351; // mwSpellCastScript
					esm.writeT<uint32_t>(tempFormID);
					// magic school (uint32)
					esm.writeT<uint32_t>(0);
					// visual effect name (char[4]) (uint32)
					esm.writeT<uint32_t>(0);
					// flags (uint8) [Hostile]
					uint8_t flags = 0;
					esm.writeT<uint8_t>(flags);
					// unused x3 (uint8)
					esm.writeT<uint8_t>(0);
					esm.writeT<uint8_t>(0);
					esm.writeT<uint8_t>(0);
					esm.endSubRecordTES4("SCIT");
				}
			}
		}

		return true;
	}

    void Ingredient::blank()
    {
        mData.mWeight = 0;
        mData.mValue = 0;
        for (int i=0; i<4; ++i)
        {
            mData.mEffectID[i] = 0;
            mData.mSkills[i] = 0;
            mData.mAttributes[i] = 0;
        }

        mName.clear();
        mModel.clear();
        mIcon.clear();
        mScript.clear();
    }
}

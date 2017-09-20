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
		tempPath << "clutter\\potions\\morro\\" << tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("MODL");
		// MODB == Bound Radius
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(16.0);
		esm.endSubRecordTES4("MODB");

		// ICON, mIcon
		tempStr = esm.generateEDIDTES4(mIcon, 1);
		tempStr.replace(tempStr.size()-4, 4, ".dds");
		tempPath.str(""); tempPath.clear();
		tempPath << "clutter\\potions\\morro\\" << tempStr;
		esm.startSubRecordTES4("ICON");
		esm.writeHCString(tempPath.str());
		esm.endSubRecordTES4("ICON");

		// SCRI --mScript (formID)
		std::string strScript = esm.generateEDIDTES4(mScript);
		if (strScript.size() > 2 && (Misc::StringUtils::lowerCase(strScript).find("sc", strScript.size() - 2) == strScript.npos) &&
			(Misc::StringUtils::lowerCase(strScript).find("script", strScript.size() - 6) == strScript.npos)) 
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

		// DATA, float (item weight)
		esm.startSubRecordTES4("DATA");
		esm.writeT<float>(mData.mWeight);
		esm.endSubRecordTES4("DATA");

		// ENIT {long, byte[1], byte[3]} -- flags
		esm.startSubRecordTES4("ENIT");
		// value
		esm.writeT<int32_t>(mData.mValue);
		// flags [ no autocalc, food item ]
		int flags=0;
		if (mData.mAutoCalc == 0)
			flags |= 0x01; // no-autocalc on
		esm.writeT<uint8_t>(flags);
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.endSubRecordTES4("ENIT");

		// wbEffects...
		for (auto magicEffect = mEffects.mList.begin(); magicEffect != mEffects.mList.end(); magicEffect++)
		{
			// EFID
			esm.startSubRecordTES4("EFID");
			// write char[4] magic effect
			magicEffect->exportTES4EFID(esm);
			esm.endSubRecordTES4("EFID");
			// EFIT
			esm.startSubRecordTES4("EFIT");
			magicEffect->exportTES4EFIT(esm);
			esm.endSubRecordTES4("EFIT");
			// SCIT
			std::string effSIG = esm.intToMagEffIDTES4(magicEffect->mEffectID);
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

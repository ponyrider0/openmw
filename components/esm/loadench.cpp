#include "loadench.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Enchantment::sRecordId = REC_ENCH;

    void Enchantment::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'E','N','D','T'>::value:
                    esm.getHT(mData, 16);
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
            esm.fail("Missing ENDT subrecord");
    }

    void Enchantment::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNT("ENDT", mData, 16);
        mEffects.save(esm);
    }

	void Enchantment::exportTESx(ESMWriter &esm, int export_type) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempStream;

		// export EDID
		tempStr = esm.generateEDIDTES4(mId, 0, "ENCH");
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// OBME
		// FULL

		// ENIT {type, amount, cost, flags, unused}
		uint32_t enchType=0;
		esm.startSubRecordTES4("ENIT");
		if (mData.mType == ESM::Enchantment::Type::CastOnce)
			enchType = 0;
		if (mData.mType == ESM::Enchantment::Type::WhenStrikes)
			enchType = 2;
		if (mData.mType == ESM::Enchantment::Type::WhenUsed)
			enchType = 3;
		if (mData.mType == ESM::Enchantment::Type::ConstantEffect)
			enchType = 3;
		esm.writeT<uint32_t>(enchType); // enchantment type: 0=scroll, 1=staff, 2=weap, 3=apparel
		esm.writeT<uint32_t>(mData.mCharge);
		esm.writeT<uint32_t>(mData.mCost);
		esm.writeT<uint8_t>(mData.mAutocalc ? 0 : 1);
		esm.writeT<uint16_t>(0); // unused
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

	}

    void Enchantment::blank()
    {
        mData.mType = 0;
        mData.mCost = 0;
        mData.mCharge = 0;
        mData.mAutocalc = 0;

        mEffects.mList.clear();
    }
}

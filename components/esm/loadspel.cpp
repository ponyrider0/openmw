#include "loadspel.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Spell::sRecordId = REC_SPEL;

    void Spell::load(ESMReader &esm, bool &isDeleted)
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
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'S','P','D','T'>::value:
                    esm.getHT(mData, 12);
                    hasData = true;
                    break;
                case ESM::FourCC<'E','N','A','M'>::value:
                    ENAMstruct s;
                    esm.getHT(s, 24);
                    mEffects.mList.push_back(s);
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
            esm.fail("Missing SPDT subrecord");
    }

    void Spell::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("SPDT", mData, 12);
        mEffects.save(esm);
    }

	void Spell::exportTESx(ESMWriter &esm, int export_type) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		// EDID
		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// SPIT { type, cost, level, flags, unused}
		esm.startSubRecordTES4("SPIT");
		int type=0;
		switch (mData.mType)
		{
		case Spell::SpellType::ST_Spell:
			type = 0; // spell
			break;
		case Spell::SpellType::ST_Blight:
		case Spell::SpellType::ST_Curse:
		case Spell::SpellType::ST_Disease:
			type = 1; // disease
			break;
		case Spell::SpellType::ST_Power:
			type = 2; // power or 3 (lesser power)
			break;
		case Spell::SpellType::ST_Ability:
			type = 4; // ability
			break;
		default:
			type = 5; // poison
			break;
		}
		esm.writeT<uint32_t>(type);
		esm.writeT<uint32_t>(mData.mCost);
		esm.writeT<uint32_t>(0); // level == novice, apprentice, journeyman, expert, master
		uint8_t flags=0;
		if ((mData.mFlags & Spell::Flags::F_Autocalc) == 0)
			flags |= 0x01; // Manual spell cost
		if (mData.mFlags & Spell::Flags::F_PCStart)
			flags |= 0x04; // Player Start Spell
		if (mData.mFlags & Spell::Flags::F_Always) // always succeeds
			flags |= 0x02 | 0x08; // Immune to Silence1 and 2
		esm.writeT<uint8_t>(flags);
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.endSubRecordTES4("SPIT");

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

	void Spell::blank()
    {
        mData.mType = 0;
        mData.mCost = 0;
        mData.mFlags = 0;

        mName.clear();
        mEffects.mList.clear();
    }
}

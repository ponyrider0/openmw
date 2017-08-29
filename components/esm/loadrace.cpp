#include "loadrace.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Race::sRecordId = REC_RACE;

    int Race::MaleFemale::getValue (bool male) const
    {
        return male ? mMale : mFemale;
    }

    int Race::MaleFemaleF::getValue (bool male) const
    {
        return static_cast<int>(male ? mMale : mFemale);
    }

    void Race::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPowers.mList.clear();

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
                case ESM::FourCC<'R','A','D','T'>::value:
                    esm.getHT(mData, 140);
                    hasData = true;
                    break;
                case ESM::FourCC<'D','E','S','C'>::value:
                    mDescription = esm.getHString();
                    break;
                case ESM::FourCC<'N','P','C','S'>::value:
                    mPowers.add(esm);
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing RADT subrecord");
    }

    void Race::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("RADT", mData, 140);
        mPowers.save(esm);
        esm.writeHNOString("DESC", mDescription);
    }

	void Race::exportTESx(ESMWriter &esm, int export_type) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;

		// EDID
		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// FULL name
		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		esm.startSubRecordTES4("DESC");
		esm.writeHCString(mDescription);
		esm.endSubRecordTES4("DESC");

		// SPLOs array
		for (auto spellItem = mPowers.mList.begin(); spellItem != mPowers.mList.end(); spellItem++)
		{
			tempFormID = esm.crossRefStringID(*spellItem);
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("SPLO");
				esm.writeT<uint32_t>(tempFormID); // spell or lvlspel formID
				esm.endSubRecordTES4("SPLO");
			}
		}

		// XNAMs, relations array

		// DATA {skill boosts, sex height, weight, flags}
		esm.startSubRecordTES4("DATA");
		// skill boosts { skill ActorValue (signed byte), boost (signed byte) }
		for (int i=0; i < 7; i++)
		{
			int actorVal = esm.skillToActorValTES4(mData.mBonus[i].mSkill);
			esm.writeT<int8_t>(actorVal);
			esm.writeT<int8_t>(mData.mBonus[i].mBonus);
		}
		esm.writeT<uint16_t>(8); // unused
		esm.writeT<float>(mData.mHeight.mMale);
		esm.writeT<float>(mData.mHeight.mFemale);
		esm.writeT<float>(mData.mWeight.mMale);
		esm.writeT<float>(mData.mWeight.mFemale);
		uint32_t flags=0;
		if (mData.mFlags & Flags::Playable)
			flags |= 0x01;
		esm.writeT<uint32_t>(flags);
		esm.endSubRecordTES4("DATA");

		// VNAM
		// DNAM
		// CNAM
		// PNAM

		// ATTR base attributes, male then female
		esm.startSubRecordTES4("ATTR");
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Strength].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Intelligence].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Willpower].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Agility].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Speed].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Endurance].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Personality].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Luck].mMale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Strength].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Intelligence].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Willpower].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Agility].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Speed].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Endurance].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Personality].mFemale);
		esm.writeT<int8_t>(mData.mAttributeValues[Attribute::Luck].mFemale);
		esm.endSubRecordTES4("ATTR");

		// Face Data
		// NAM1, body data marker
		// Male body data
		// Female body data
		// HNAM hairs
		// ENAM eyes
		// FaceGen Data
		// SNAM unknown

	}

    void Race::blank()
    {
        mName.clear();
        mDescription.clear();

        mPowers.mList.clear();

        for (int i=0; i<7; ++i)
        {
            mData.mBonus[i].mSkill = -1;
            mData.mBonus[i].mBonus = 0;
        }

        for (int i=0; i<8; ++i)
            mData.mAttributeValues[i].mMale = mData.mAttributeValues[i].mFemale = 1;

        mData.mHeight.mMale = mData.mHeight.mFemale = 1;
        mData.mWeight.mMale = mData.mWeight.mFemale = 1;

        mData.mFlags = 0;
    }
}

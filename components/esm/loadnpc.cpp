#include "loadnpc.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int NPC::sRecordId = REC_NPC_;

    void NPC::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPersistent = (esm.getRecordFlags() & 0x0400) != 0;

        mSpells.mList.clear();
        mInventory.mList.clear();
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mHasAI = false;

        bool hasName = false;
        bool hasNpdt = false;
        bool hasFlags = false;
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
                case ESM::FourCC<'R','N','A','M'>::value:
                    mRace = esm.getHString();
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    mClass = esm.getHString();
                    break;
                case ESM::FourCC<'A','N','A','M'>::value:
                    mFaction = esm.getHString();
                    break;
                case ESM::FourCC<'B','N','A','M'>::value:
                    mHead = esm.getHString();
                    break;
                case ESM::FourCC<'K','N','A','M'>::value:
                    mHair = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'N','P','D','T'>::value:
                    hasNpdt = true;
                    esm.getSubHeader();
                    if (esm.getSubSize() == 52)
                    {
                        mNpdtType = NPC_DEFAULT;
                        esm.getExact(&mNpdt52, 52);
                    }
                    else if (esm.getSubSize() == 12)
                    {
                        mNpdtType = NPC_WITH_AUTOCALCULATED_STATS;
                        esm.getExact(&mNpdt12, 12);
                    }
                    else
                        esm.fail("NPC_NPDT must be 12 or 52 bytes long");
                    break;
                case ESM::FourCC<'F','L','A','G'>::value:
                    hasFlags = true;
                    esm.getHT(mFlags);
                    break;
                case ESM::FourCC<'N','P','C','S'>::value:
                    mSpells.add(esm);
                    break;
                case ESM::FourCC<'N','P','C','O'>::value:
                    mInventory.add(esm);
                    break;
                case ESM::FourCC<'A','I','D','T'>::value:
                    esm.getHExact(&mAiData, sizeof(mAiData));
                    mHasAI= true;
                    break;
                case ESM::FourCC<'D','O','D','T'>::value:
                case ESM::FourCC<'D','N','A','M'>::value:
                    mTransport.add(esm);
                    break;
                case AI_Wander:
                case AI_Activate:
                case AI_Escort:
                case AI_Follow:
                case AI_Travel:
                case AI_CNDT:
                    mAiPackage.add(esm);
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
        if (!hasNpdt && !isDeleted)
            esm.fail("Missing NPDT subrecord");
        if (!hasFlags && !isDeleted)
            esm.fail("Missing FLAG subrecord");
    }
    void NPC::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNCString("RNAM", mRace);
        esm.writeHNCString("CNAM", mClass);
        esm.writeHNCString("ANAM", mFaction);
        esm.writeHNCString("BNAM", mHead);
        esm.writeHNCString("KNAM", mHair);
        esm.writeHNOCString("SCRI", mScript);

        if (mNpdtType == NPC_DEFAULT)
            esm.writeHNT("NPDT", mNpdt52, 52);
        else if (mNpdtType == NPC_WITH_AUTOCALCULATED_STATS)
            esm.writeHNT("NPDT", mNpdt12, 12);

        esm.writeHNT("FLAG", mFlags);

        mInventory.save(esm);
        mSpells.save(esm);
        if (mAiData.mHello != 0
            || mAiData.mFight != 0
            || mAiData.mFlee != 0
            || mAiData.mAlarm != 0
            || mAiData.mServices != 0)
        {
            esm.writeHNT("AIDT", mAiData, sizeof(mAiData));
        }

        mTransport.save(esm);

        mAiPackage.save(esm);
    }
	bool NPC::exportTESx(ESMWriter &esm, int export_format) const
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

		// MODL == Model Filename
		if (mModel.size() > 4)
		{
			tempStr = esm.generateEDIDTES4(mModel, true);
			tempStr.replace(tempStr.size()-4, 4, ".nif");
			tempPath << "clutter\\morro\\" << tempStr;
			esm.startSubRecordTES4("MODL");
			esm.writeHCString(tempPath.str());
			esm.endSubRecordTES4("MODL");
			// MODB == Bound Radius
			esm.startSubRecordTES4("MODB");
			esm.writeT<float>(0.0);
			esm.endSubRecordTES4("MODB");
			// MODT
		}

		// ACBS group
		esm.startSubRecordTES4("ACBS");
		uint32_t flags=0;
		if (mFlags & Flags::Female)
			flags |= 0x01;
		if (mFlags & Flags::Essential)
			flags |= 0x02;
		if (mFlags & Flags::Respawn)
			flags |= 0x08;
		if (mFlags & Flags::Autocalc)
			flags |= 0x10;
		esm.writeT<uint32_t>(flags); //flags
		esm.writeT<uint16_t>(mNpdt52.mMana); // base spell pts
		esm.writeT<uint16_t>(mNpdt52.mFatigue); // fatigue
		esm.writeT<uint16_t>(mNpdt52.mGold); // barter gold
		esm.writeT<int16_t>(mNpdt52.mLevel); // level / offset level
		esm.writeT<uint16_t>(0); // calc min
		esm.writeT<uint16_t>(0); // calc max
		esm.endSubRecordTES4("ACBS");

		// SNAM, faction struct { formid, rank(byte), flags(ubyte[3]) }
		tempFormID = esm.crossRefStringID(mFaction);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SNAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.writeT<int8_t>(mNpdt52.mRank);
			esm.writeT<uint8_t>(0); // unused
			esm.writeT<uint8_t>(0); // unused
			esm.writeT<uint8_t>(0); // unused
			esm.endSubRecordTES4("SNAM");
		}

		// INAM, death item formID [LVLI]
		// RNAM, race formID
		tempFormID = esm.crossRefStringID(mRace);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("RACE");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("RACE");
		}

		// CNTO: {formID, uint32}
		for (auto inventoryItem = mInventory.mList.begin(); inventoryItem != mInventory.mList.end(); inventoryItem++)
		{
			tempFormID = esm.crossRefStringID(inventoryItem->mItem.toString());
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("CNTO");
				esm.writeT<uint32_t>(tempFormID);
				esm.writeT<int32_t>(inventoryItem->mCount);
				esm.endSubRecordTES4("CNTO");
			}
		}

		// SPLO
		for (auto spellItem = mSpells.mList.begin(); spellItem != mSpells.mList.end(); spellItem++)
		{
			tempFormID = esm.crossRefStringID(*spellItem);
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("SPLO");
				esm.writeT<uint32_t>(tempFormID); // spell or lvlspel formID
				esm.endSubRecordTES4("SPLO");
			}
		}

		// SCRI
		tempFormID = esm.crossRefStringID(mScript);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// AI Data
		esm.startSubRecordTES4("AIDT");
		esm.writeT<unsigned char>(mAiData.mFight); // aggression
		esm.writeT<unsigned char>(100-mAiData.mFlee); // confidence
		esm.writeT<unsigned char>(50); // energy
		esm.writeT<unsigned char>(50); // responsibility
		flags = 0;
		// NPC services...
		esm.writeT<uint32_t>(flags); // flags (buy/sell/services)
		esm.writeT<unsigned char>(0); // trainer Skill
		esm.writeT<unsigned char>(0); // trainer level
		esm.writeT<uint16_t>(0); // unused
		esm.endSubRecordTES4("AIDT");

		// PKID, AIpackage formID
		// KFFZ, animations

		// CNAM, class formID
		uint32_t classFormID = esm.crossRefStringID(mClass);
		if (classFormID != 0)
		{
			esm.startSubRecordTES4("CNAM");
			esm.writeT<uint32_t>(classFormID);
			esm.endSubRecordTES4("CNAM");
		}

		// DATA ***switch Npdt52 vs Npdt12***
		esm.startSubRecordTES4("DATA"); // stats
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Armorer]); // armorer
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Athletics]); // athletics
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::LongBlade]); // blade
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Block]); // block
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::BluntWeapon]); // blunt
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::HandToHand]); // HtoH
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::HeavyArmor]); // heavy
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Alchemy]); // alchemy
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Alteration]); // alteration
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Conjuration]); // conjuration
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Destruction]); // destruction
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Illusion]); // illusion
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Mysticism]); // mysticism
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Restoration]); // restoration
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Acrobatics]); // acrobatics
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::LightArmor]); // light
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Marksman]); // marksman
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Mercantile]); // mercantile
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Security]); // security
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Sneak]); // sneak
		esm.writeT<unsigned char>(mNpdt52.mSkills[ESM::Skill::Speechcraft]); // speechcraft
		esm.writeT<uint16_t>(mNpdt52.mHealth); // health
		esm.writeT<unsigned char>(0); // Unused
		esm.writeT<unsigned char>(0); // Unused
		esm.writeT<unsigned char>(mNpdt52.mStrength); // strength
		esm.writeT<unsigned char>(mNpdt52.mIntelligence); // int
		esm.writeT<unsigned char>(mNpdt52.mWillpower); // will
		esm.writeT<unsigned char>(mNpdt52.mAgility); // agi
		esm.writeT<unsigned char>(mNpdt52.mSpeed); // speed
		esm.writeT<unsigned char>(mNpdt52.mEndurance); // end
		esm.writeT<unsigned char>(mNpdt52.mPersonality); // per
		esm.writeT<unsigned char>(mNpdt52.mLuck); // luck
		esm.endSubRecordTES4("DATA");

		// HNAM (hair formid)
		// LNAM (float, hair lenght)
		// ENAM (eye formid)
		// HCLR, hair color 32bit
		// CSTY, combat style formid
		// FaceGen...
		// FNAM bytearray?? unknown

		return true;
	}

    bool NPC::isMale() const {
        return (mFlags & Female) == 0;
    }

    void NPC::setIsMale(bool value) {
        mFlags |= Female;
        if (value) {
            mFlags ^= Female;
        }
    }

    void NPC::blank()
    {
        mNpdtType = NPC_DEFAULT;
        mNpdt52.mLevel = 0;
        mNpdt52.mStrength = mNpdt52.mIntelligence = mNpdt52.mWillpower = mNpdt52.mAgility =
            mNpdt52.mSpeed = mNpdt52.mEndurance = mNpdt52.mPersonality = mNpdt52.mLuck = 0;
        for (int i=0; i< Skill::Length; ++i) mNpdt52.mSkills[i] = 0;
        mNpdt52.mReputation = 0;
        mNpdt52.mHealth = mNpdt52.mMana = mNpdt52.mFatigue = 0;
        mNpdt52.mDisposition = 0;
        mNpdt52.mFactionID = 0;
        mNpdt52.mRank = 0;
        mNpdt52.mUnknown = 0;
        mNpdt52.mGold = 0;
        mNpdt12.mLevel = 0;
        mNpdt12.mDisposition = 0;
        mNpdt12.mReputation = 0;
        mNpdt12.mRank = 0;
        mNpdt12.mUnknown1 = 0;
        mNpdt12.mUnknown2 = 0;
        mNpdt12.mUnknown3 = 0;
        mNpdt12.mGold = 0;
        mFlags = 0;
        mInventory.mList.clear();
        mSpells.mList.clear();
        mAiData.blank();
        mHasAI = false;
        mTransport.mList.clear();
        mAiPackage.mList.clear();
        mName.clear();
        mModel.clear();
        mRace.clear();
        mClass.clear();
        mFaction.clear();
        mScript.clear();
        mHair.clear();
        mHead.clear();
    }

    int NPC::getFactionRank() const
    {
        if (mFaction.empty())
            return -1;
        else if (mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS)
            return mNpdt12.mRank;
        else // NPC_DEFAULT
            return mNpdt52.mRank;
    }

    const std::vector<Transport::Dest>& NPC::getTransport() const
    {
        return mTransport.mList;
    }
}

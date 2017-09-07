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
		int tempVal;
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempPath;
		bool autocalc = (mFlags & Flags::Autocalc);
		bool beast_race = false;

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
/*
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
*/

		tempStr = Misc::StringUtils::lowerCase(mRace);
		if ( (tempStr.find("argonian") != tempStr.npos) || (tempStr.find("khajiit") != tempStr.npos) )
		{
			beast_race = true;
			tempStr = "characters\\_male\\skeletonbeast.nif";
		}
		else
			tempStr = "characters\\_male\\skeleton.nif";
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("MODL");
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(72);
		esm.endSubRecordTES4("MODB");

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
		tempVal = (autocalc) ? 50 : mNpdt52.mMana;
		esm.writeT<uint16_t>(tempVal); // base spell pts
		tempVal = (autocalc) ? 50 : mNpdt52.mFatigue;
		esm.writeT<uint16_t>(tempVal); // fatigue
		tempVal = (autocalc) ? mNpdt12.mGold : mNpdt52.mGold;
		esm.writeT<uint16_t>(tempVal); // barter gold
		tempVal = (autocalc) ? mNpdt12.mLevel : mNpdt52.mLevel;
		esm.writeT<int16_t>(tempVal); // level / offset level
		esm.writeT<uint16_t>(1); // calc min
		esm.writeT<uint16_t>(0); // calc max
		esm.endSubRecordTES4("ACBS");

		// SNAM, faction struct { formid, rank(byte), flags(ubyte[3]) }
		tempFormID = esm.crossRefStringID(mFaction);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SNAM");
			esm.writeT<uint32_t>(tempFormID);
			tempVal = (autocalc) ? mNpdt12.mRank : mNpdt52.mRank;
			esm.writeT<int8_t>(tempVal);
			esm.writeT<uint8_t>(0); // unused
			esm.writeT<uint8_t>(0); // unused
			esm.writeT<uint8_t>(0); // unused
			esm.endSubRecordTES4("SNAM");
		}

		// INAM, death item formID [LVLI]

		// RNAM, race formID
		tempFormID = esm.substituteRaceID(mRace);
		if (tempFormID == 0)
			tempFormID = esm.crossRefStringID(mRace);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("RNAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("RNAM");
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
		if ( Misc::StringUtils::lowerCase(mScript).find("script") == std::string::npos )
			tempStr = mScript + "Script";
		else
			tempStr = mScript;
		tempFormID = esm.crossRefStringID(tempStr);
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
		esm.startSubRecordTES4("PKID");
		esm.writeT<uint32_t>(0x02E56E); // Oblivion ID: aaaDefaultStayAtCurrentLocation
		esm.endSubRecordTES4("PKID");
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
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Armorer];
		esm.writeT<unsigned char>(tempVal); // armorer
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Athletics];
		esm.writeT<unsigned char>(tempVal); // athletics
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::LongBlade];
		esm.writeT<unsigned char>(tempVal); // blade
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Block];
		esm.writeT<unsigned char>(tempVal); // block
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::BluntWeapon];
		esm.writeT<unsigned char>(tempVal); // blunt
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::HandToHand];
		esm.writeT<unsigned char>(tempVal); // HtoH
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::HeavyArmor];
		esm.writeT<unsigned char>(tempVal); // heavy
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Alchemy];
		esm.writeT<unsigned char>(tempVal); // alchemy
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Alteration];
		esm.writeT<unsigned char>(tempVal); // alteration
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Conjuration];
		esm.writeT<unsigned char>(tempVal); // conjuration
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Destruction];
		esm.writeT<unsigned char>(tempVal); // destruction
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Illusion];
		esm.writeT<unsigned char>(tempVal); // illusion
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Mysticism];
		esm.writeT<unsigned char>(tempVal); // mysticism
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Restoration];
		esm.writeT<unsigned char>(tempVal); // restoration
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Acrobatics];
		esm.writeT<unsigned char>(tempVal); // acrobatics
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::LightArmor];
		esm.writeT<unsigned char>(tempVal); // light
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Marksman];
		esm.writeT<unsigned char>(tempVal); // marksman
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Mercantile];
		esm.writeT<unsigned char>(tempVal); // mercantile
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Security];
		esm.writeT<unsigned char>(tempVal); // security
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Sneak];
		esm.writeT<unsigned char>(tempVal); // sneak
		tempVal = (autocalc) ? 5 : mNpdt52.mSkills[ESM::Skill::Speechcraft];
		esm.writeT<unsigned char>(tempVal); // speechcraft
		tempVal = (autocalc) ? 50 : mNpdt52.mHealth;
		esm.writeT<uint16_t>(tempVal); // health
		esm.writeT<unsigned char>(0); // Unused
		esm.writeT<unsigned char>(0); // Unused
		tempVal = (autocalc) ? 50 : mNpdt52.mStrength;
		esm.writeT<unsigned char>(tempVal); // strength
		tempVal = (autocalc) ? 50 : mNpdt52.mIntelligence;
		esm.writeT<unsigned char>(tempVal); // int
		tempVal = (autocalc) ? 50 : mNpdt52.mWillpower;
		esm.writeT<unsigned char>(tempVal); // will
		tempVal = (autocalc) ? 50 : mNpdt52.mAgility;
		esm.writeT<unsigned char>(tempVal); // agi
		tempVal = (autocalc) ? 50 : mNpdt52.mSpeed;
		esm.writeT<unsigned char>(tempVal); // speed
		tempVal = (autocalc) ? 50 : mNpdt52.mEndurance;
		esm.writeT<unsigned char>(tempVal); // end
		tempVal = (autocalc) ? 30 : mNpdt52.mPersonality;
		esm.writeT<unsigned char>(tempVal); // per
		tempVal = (autocalc) ? 50 : mNpdt52.mLuck;
		esm.writeT<unsigned char>(tempVal); // luck
		esm.endSubRecordTES4("DATA");

		// HNAM (hair formid)
		tempFormID = (beast_race) ? 0 : 0x1DA82; // elf ponytail
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("HNAM");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("HNAM");
		}

		// LNAM (float, hair lenght)
		esm.startSubRecordTES4("LNAM");
		esm.writeT<float>(0);
		esm.endSubRecordTES4("LNAM");

/*
		// ENAM (eye formid)
		tempFormID = 0;
		esm.startSubRecordTES4("ENAM");
		esm.writeT<uint32_t>(tempFormID);
		esm.endSubRecordTES4("ENAM");
*/

		// HCLR, hair color 24bit
		esm.startSubRecordTES4("HCLR");
		esm.writeT<uint8_t>(0x4B); // 75
		esm.writeT<uint8_t>(0x32); // 50
		esm.writeT<uint8_t>(0x19); // 25
		esm.writeT<uint8_t>(0x00); //unused
		esm.endSubRecordTES4("HCLR");

/*
		// CSTY, combat style formid ????ZNAM???
		tempFormID = 0;
		esm.startSubRecordTES4("CSTY");
		esm.writeT<uint32_t>(tempFormID);
		esm.endSubRecordTES4("CSTY");
*/

		// FaceGen...
		esm.startSubRecordTES4("FGGS");
		for (int i=0; i < sizeof(FGGS_default); i++)
			esm.writeT<uint8_t>(FGGS_default[i]);
		esm.endSubRecordTES4("FGGS");
		esm.startSubRecordTES4("FGGA");
		for (int i=0; i < sizeof(FGGA_default); i++)
			esm.writeT<uint8_t>(FGGA_default[i]);
		esm.endSubRecordTES4("FGGA");
		esm.startSubRecordTES4("FGTS");
		for (int i=0; i < sizeof(FGTS_default); i++)
			esm.writeT<uint8_t>(FGTS_default[i]);
		esm.endSubRecordTES4("FGTS");

		// FNAM bytearray?? unknown
		esm.startSubRecordTES4("FNAM");
		esm.writeT<uint8_t>(0x23);
		esm.writeT<uint8_t>(0x17);
		esm.endSubRecordTES4("FNAM");

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

#include "loadcrea.hpp"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM {

    unsigned int Creature::sRecordId = REC_CREA;

    void Creature::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mPersistent = (esm.getRecordFlags() & 0x0400) != 0;

        mAiPackage.mList.clear();
        mInventory.mList.clear();
        mSpells.mList.clear();
        mTransport.mList.clear();

        mScale = 1.f;
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
                case ESM::FourCC<'C','N','A','M'>::value:
                    mOriginal = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                    mName = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','R','I'>::value:
                    mScript = esm.getHString();
                    break;
                case ESM::FourCC<'N','P','D','T'>::value:
                    esm.getHT(mData, 96);
                    hasNpdt = true;
                    break;
                case ESM::FourCC<'F','L','A','G'>::value:
                    esm.getHT(mFlags);
                    hasFlags = true;
                    break;
                case ESM::FourCC<'X','S','C','L'>::value:
                    esm.getHT(mScale);
                    break;
                case ESM::FourCC<'N','P','C','O'>::value:
                    mInventory.add(esm);
                    break;
                case ESM::FourCC<'N','P','C','S'>::value:
                    mSpells.add(esm);
                    break;
                case ESM::FourCC<'A','I','D','T'>::value:
                    esm.getHExact(&mAiData, sizeof(mAiData));
                    mHasAI = true;
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
                case ESM::FourCC<'I','N','D','X'>::value:
                    // seems to occur only in .ESS files, unsure of purpose
                    int index;
                    esm.getHT(index);
                    std::cerr << "Creature::load: Unhandled INDX " << index << std::endl;
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

    void Creature::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("CNAM", mOriginal);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNT("NPDT", mData, 96);
        esm.writeHNT("FLAG", mFlags);
        if (mScale != 1.0) {
            esm.writeHNT("XSCL", mScale);
        }

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
	bool Creature::exportTESx(ESMWriter &esm, int export_format) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempStream;
		int tempVal;
		float tempFloat;

		// export EDID
		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		// FULL, fullname
		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

/*
		// MODL == Model Filename
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		tempStream << "morro\\" << tempStr;
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempStream.str());
		esm.endSubRecordTES4("MODL");
		// MODB?
		// MODT?
*/
		tempStr = "creatures\\minotaur\\skeleton.nif";
		esm.startSubRecordTES4("MODL");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("MODL");
		esm.startSubRecordTES4("MODB");
		esm.writeT<float>(140);
		esm.endSubRecordTES4("MODB");

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

/*
		// array - NIFZ
		tempStr = esm.generateEDIDTES4(mModel, 1);
		tempStr.replace(tempStr.size()-4, 4, ".nif");
		esm.startSubRecordTES4("NIFZ");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("NIFZ");
		// NIFT
*/
		esm.startSubRecordTES4("NIFZ");
		esm.writeHCString("eyelids.nif");
		esm.writeHCString("goz.nif");
		esm.writeHCString("hair01.nif");
		esm.writeHCString("hair02.nif");
		esm.writeHCString("head.nif");
		esm.writeHCString("hornsb.nif");
		esm.writeHCString("minotaur.nif");
		esm.writeHCString("minotaurbodyhair.nif");
		esm.writeT<uint8_t>(0); // terminating null character for string list
		esm.endSubRecordTES4("NIFZ");
		// NIFT
		esm.startSubRecordTES4("NIFT");
		esm.writeT<uint32_t>(0);
		esm.endSubRecordTES4("NIFT");

		// ACBS group
		uint32_t flags=0;
		flags |= 0x200; // No Low Level Processing (needed for LVLC)
		if (mFlags & ESM::Creature::Flags::Bipedal)
			flags |= 0x01;
		if (mFlags & ESM::Creature::Flags::Essential)
			flags |= 0x02;
		if (mFlags & ESM::Creature::Flags::Weapon)
			flags |= 0x04;
		if (mFlags & ESM::Creature::Flags::Respawn)
			flags |= 0x08;
		if (mFlags & ESM::Creature::Flags::Swims)
			flags |= 0x10;
		if (mFlags & ESM::Creature::Flags::Flies)
			flags |= 0x20;
		if (mFlags & ESM::Creature::Flags::Walks)
			flags |= 0x40;
		if (mFlags & ESM::Creature::Flags::Skeleton)
			flags |= 0x800;
		if (mFlags & ESM::Creature::Flags::Metal)
			flags |= 0x800;
		esm.startSubRecordTES4("ACBS");
		esm.writeT<uint32_t>(flags); //flags
		esm.writeT<uint16_t>(mData.mMana); // spell pts
		esm.writeT<uint16_t>(mData.mFatigue); // fatigue
		esm.writeT<uint16_t>(mData.mGold); // barter gold
		esm.writeT<int16_t>(mData.mLevel); // level / offset level
		esm.writeT<uint16_t>(0); // calc min
		esm.writeT<uint16_t>(0); // calc max
		esm.endSubRecordTES4("ACBS");

		// Factions ...SNAM array
		tempFormID = 0x13; // CreatureFaction
		esm.startSubRecordTES4("SNAM");
		esm.writeT<uint32_t>(tempFormID);
		tempVal = 0; // Faction Rank
		esm.writeT<uint8_t>(tempVal); 
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint8_t>(0); // unused
		esm.endSubRecordTES4("SNAM");

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

		// INAM, death items (LVLI)

		// SCRI
		std::string strScript = esm.generateEDIDTES4(mScript);
		if (Misc::StringUtils::lowerCase(strScript).find("sc", strScript.size()-2) == strScript.npos)
		{
			strScript += "Script";
		}
		tempFormID = esm.crossRefStringID(strScript, false);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("SCRI");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("SCRI");
		}

		// AI Data
		uint8_t aggression=0;
		if (mAiData.mFight <= 0)
			aggression = 5;
		else if (mAiData.mFight <= 10)
			aggression = 6;
		else if (mAiData.mFight <= 30)
			aggression = 7;
		else if (mAiData.mFight <= 70)
			aggression = 10;
		else if (mAiData.mFight <= 80)
			aggression = 30;
		else if (mAiData.mFight > 80)
			aggression = 80;
		esm.startSubRecordTES4("AIDT");
		esm.writeT<unsigned char>(aggression); // aggression
		esm.writeT<unsigned char>(100-mAiData.mFlee); // confidence
		esm.writeT<unsigned char>(70); // energy
		esm.writeT<unsigned char>(0); // responsibility
		flags = 0;
		esm.writeT<uint32_t>(flags); // flags (buy/sell/services)
		esm.writeT<unsigned char>(0); // trainer Skill
		esm.writeT<unsigned char>(0); // trainer level
		esm.writeT<uint16_t>(0); // unused
		esm.endSubRecordTES4("AIDT");

		// PKID, ai packages (formID)
		tempStr = esm.generateEDIDTES4(mId);
		std::string pkgEDID;
		int pkgcount = 0;
		uint32_t pkgFormID;
		for (auto it_aipackage = mAiPackage.mList.begin(); it_aipackage != mAiPackage.mList.end(); it_aipackage++)
		{
			// aiPkg is full definition and not just a stringID...
			// ... so must generate a full ESM4 AIPackage record from each aiPkg
			int duration=0, distance=0;
			std::ostringstream ai_debugstream;
			ai_debugstream << "CREA[" << tempStr << "] AIPackage[" << pkgcount++ << "]: ";
			switch (it_aipackage->mType)
			{
			case ESM::AI_Wander:
				duration = it_aipackage->mWander.mDuration;
				distance = it_aipackage->mWander.mDistance;
				if (distance == 0)
					pkgEDID = "aaaDefaultStayAtEditorLocation";
				else if (distance <= 128)
					pkgEDID = "aaaDefaultExploreCurrentLoc256";
				else if (distance <= 512)
					pkgEDID = "aaaDefaultExploreEditorLoc512";
				else if (distance <= 1000)
					pkgEDID = "aaaDefaultExploreEditorLoc1024";
				else if (distance <= 2000)
					pkgEDID = "aaaDefaultExploreEditorLoc3000";
				pkgFormID = esm.crossRefStringID(pkgEDID, false);
				esm.startSubRecordTES4("PKID");
				esm.writeT<uint32_t>(pkgFormID);
				esm.endSubRecordTES4("PKID");
				ai_debugstream << "Wander Dist:" << distance << " Dur:" << duration;
				break;
			}
			ai_debugstream << std::endl;
			std::cout << ai_debugstream.str();
			OutputDebugString(ai_debugstream.str().c_str());
		}
		pkgEDID = "aaaCreatureExterior1500";
		pkgFormID = esm.crossRefStringID(pkgEDID, false);
		esm.startSubRecordTES4("PKID");
		esm.writeT<uint32_t>(pkgFormID);
		esm.endSubRecordTES4("PKID");
		pkgEDID = "aaaCreatureInterior512";
		pkgFormID = esm.crossRefStringID(pkgEDID, false);
		esm.startSubRecordTES4("PKID");
		esm.writeT<uint32_t>(pkgFormID);
		esm.endSubRecordTES4("PKID");

		// KFFZ, animations

		// DATA
		esm.startSubRecordTES4("DATA");
		esm.writeT<unsigned char>(mData.mType); // Type Creature
		esm.writeT<unsigned char>(mData.mCombat); // Combat
		esm.writeT<unsigned char>(mData.mMagic); // Magic
		esm.writeT<unsigned char>(mData.mStealth); // Stealth
		if (mData.mSoul == 0)
			tempVal = 0;
		else if (mData.mSoul <= 150) // petty
			tempVal = 1;
		else if (mData.mSoul <= 300) // lesser
			tempVal = 2;
		else if (mData.mSoul <= 800) // common
			tempVal = 3;
		else if (mData.mSoul <= 1200) // greater
			tempVal = 4;
		else if (mData.mSoul <= 1600) // grand
			tempVal = 5;
		esm.writeT<uint8_t>(tempVal); // soul size
		esm.writeT<uint8_t>(0); // unused
		esm.writeT<uint16_t>(mData.mHealth); // health
		esm.writeT<uint16_t>(0); // unused
		esm.writeT<uint16_t>( (mData.mAttack[0]+mData.mAttack[1])/2 ); // attack damage
		esm.writeT<unsigned char>(mData.mStrength); // strength
		esm.writeT<unsigned char>(mData.mIntelligence); // int
		esm.writeT<unsigned char>(mData.mWillpower); // will
		esm.writeT<unsigned char>(mData.mAgility); // agi
		esm.writeT<unsigned char>(mData.mSpeed); // speed
		esm.writeT<unsigned char>(mData.mEndurance); // end
		esm.writeT<unsigned char>(mData.mPersonality); // per
		esm.writeT<unsigned char>(mData.mLuck); // luck
		esm.endSubRecordTES4("DATA");

		// RNAM, attack reach
		tempVal = 64;
		esm.startSubRecordTES4("RNAM");
		esm.writeT<uint8_t>(tempVal);
		esm.endSubRecordTES4("RNAM");

		// ZNAM, combat style formID (CSTY)
		tempFormID = 0x2d6f4; // defaultminotaurlord
		esm.startSubRecordTES4("ZNAM");
		esm.writeT<uint32_t>(tempFormID);
		esm.endSubRecordTES4("ZNAM");

		// TNAM, turning speed (float)
		tempFloat = 0;
		esm.startSubRecordTES4("TNAM");
		esm.writeT<float>(tempFloat);
		esm.endSubRecordTES4("TNAM");

		// BNAM, base scale (float)
		esm.startSubRecordTES4("BNAM");
		esm.writeT<float>(mScale); 
		esm.endSubRecordTES4("BNAM");

		// WNAM, foot weight (float)
		tempFloat = 0;
		esm.startSubRecordTES4("WNAM");
		esm.writeT<float>(tempFloat);
		esm.endSubRecordTES4("WNAM");

		// NAM0, blood spray (string)
		// NAM1, blood decal (string)

		// CSCR, inherit sound from: formID CREA
		tempFormID = esm.crossRefStringID(mOriginal);
		if (tempFormID != 0)
		{
			esm.startSubRecordTES4("CSCR");
			esm.writeT<uint32_t>(tempFormID);
			esm.endSubRecordTES4("CSCR");
		}

		// CSDTs...

		return true;
	}

    void Creature::blank()
    {
        mData.mType = 0;
        mData.mLevel = 0;
        mData.mStrength = mData.mIntelligence = mData.mWillpower = mData.mAgility =
            mData.mSpeed = mData.mEndurance = mData.mPersonality = mData.mLuck = 0;
        mData.mHealth = mData.mMana = mData.mFatigue = 0;
        mData.mSoul = 0;
        mData.mCombat = mData.mMagic = mData.mStealth = 0;
        for (int i=0; i<6; ++i) mData.mAttack[i] = 0;
        mData.mGold = 0;
        mFlags = 0;
        mScale = 1.f;
        mModel.clear();
        mName.clear();
        mScript.clear();
        mOriginal.clear();
        mInventory.mList.clear();
        mSpells.mList.clear();
        mHasAI = false;
        mAiData.blank();
        mAiData.mServices = 0;
        mAiPackage.mList.clear();
        mTransport.mList.clear();
    }

    const std::vector<Transport::Dest>& Creature::getTransport() const
    {
        return mTransport.mList;
    }
}

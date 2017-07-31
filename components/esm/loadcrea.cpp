#include "loadcrea.hpp"

#include <iostream>

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
		// export CREA
		std::string *newEDID = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(*newEDID);
		esm.endSubRecordTES4("EDID");
		delete newEDID;

		// ACBS group
		esm.startSubRecordTES4("ACBS");
		esm.writeT<uint32_t>(0); //flags
		esm.writeT<uint16_t>(0); // spell pts
		esm.writeT<uint16_t>(0); // fatigue
		esm.writeT<uint16_t>(0); // barter gold
		esm.writeT<int16_t>(mData.mLevel); // level / offset level
		esm.writeT<uint16_t>(0); // calc min
		esm.writeT<uint16_t>(0); // calc max
		esm.endSubRecordTES4("ACBS");

		// AI Data
		esm.startSubRecordTES4("AIDT");
		esm.writeT<unsigned char>(0); // aggression
		esm.writeT<unsigned char>(0); // confidence
		esm.writeT<unsigned char>(0); // energy
		esm.writeT<unsigned char>(0); // responsibility
		esm.writeT<uint32_t>(0); // flags (buy/sell/services)
		esm.writeT<unsigned char>(0); // trainer Skill
		esm.writeT<unsigned char>(0); // trainer level
		esm.writeT<uint16_t>(0); // ?
		esm.endSubRecordTES4("AIDT");

		// DATA
		esm.startSubRecordTES4("DATA");
		esm.writeT<unsigned char>(0); // Type Creature
		esm.writeT<unsigned char>(mData.mCombat); // Combat
		esm.writeT<unsigned char>(mData.mMagic); // Magic
		esm.writeT<unsigned char>(mData.mStealth); // Stealth
		esm.writeT<uint16_t>(mData.mSoul); // soul size
		esm.writeT<uint16_t>(mData.mHealth); // health
		esm.writeT<uint16_t>(0); // ?
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

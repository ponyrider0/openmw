#include "loadfact.hpp"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Faction::sRecordId = REC_FACT;

    int& Faction::FADTstruct::getSkill (int index, bool ignored)
    {
        if (index<0 || index>=7)
            throw std::logic_error ("skill index out of range");

        return mSkills[index];
    }

    int Faction::FADTstruct::getSkill (int index, bool ignored) const
    {
        if (index<0 || index>=7)
            throw std::logic_error ("skill index out of range");

        return mSkills[index];
    }

    void Faction::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        mReactions.clear();
        for (int i=0;i<10;++i)
            mRanks[i].clear();

        int rankCounter = 0;
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
                case ESM::FourCC<'R','N','A','M'>::value:
                    if (rankCounter >= 10)
                        esm.fail("Rank out of range");
                    mRanks[rankCounter++] = esm.getHString();
                    break;
                case ESM::FourCC<'F','A','D','T'>::value:
                    esm.getHT(mData, 240);
                    if (mData.mIsHidden > 1)
                        esm.fail("Unknown flag!");
                    hasData = true;
                    break;
                case ESM::FourCC<'A','N','A','M'>::value:
                {
                    std::string faction = esm.getHString();
                    int reaction;
                    esm.getHNT(reaction, "INTV");
                    mReactions[faction] = reaction;
                    break;
                }
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
            esm.fail("Missing FADT subrecord");
    }

    void Faction::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNOCString("FNAM", mName);

        for (int i = 0; i < 10; i++)
        {
            if (mRanks[i].empty())
                break;

            esm.writeHNString("RNAM", mRanks[i], 32);
        }

        esm.writeHNT("FADT", mData, 240);

        for (std::map<std::string, int>::const_iterator it = mReactions.begin(); it != mReactions.end(); ++it)
        {
            esm.writeHNString("ANAM", it->first);
            esm.writeHNT("INTV", it->second);
        }
    }

	void Faction::exportTESx(ESMWriter &esm, int export_type) const
	{
		uint32_t tempFormID;
		std::string tempStr;
		std::ostringstream tempStream, debugstream;

		// export EDID
		tempStr = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(tempStr);
		esm.endSubRecordTES4("EDID");

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mName);
		esm.endSubRecordTES4("FULL");

		// XNAMs, Relations: { faction/race formID, int32 modifier }
		for (auto reaction = mReactions.begin(); reaction != mReactions.end(); reaction++)
		{
			tempStr = esm.generateEDIDTES4(reaction->first);
			// TODO: substitute generic EDID for hardcoded Morroblivion EDID
			tempStr = esm.substituteMorroblivionEDID(tempStr, ESM::REC_FACT);
			tempFormID = esm.crossRefStringID(tempStr, "FACT", false);
			if (tempFormID != 0)
			{
				esm.startSubRecordTES4("XNAM");
				esm.writeT<uint32_t>(tempFormID);
				esm.writeT<int32_t>(reaction->second);
				esm.endSubRecordTES4("XNAM");
			}
			else
			{
				debugstream.str(""); debugstream.clear();
				debugstream << "Export FACT ERROR: could not resolve Faction name relation: FACT[ " <<
					esm.generateEDIDTES4(mId) << "] " << "relation: " << esm.generateEDIDTES4(reaction->first) << std::endl;
//				std::cout << debugstream.str();
				OutputDebugString(debugstream.str().c_str());
			}
		}

		// DATA
		int flags=0;
		if (mData.mIsHidden)
			flags |= 0x01;
		esm.startSubRecordTES4("DATA");
		// uint8, flags [hidden, evil, special combat]
		esm.writeT<uint8_t>(flags);
		esm.endSubRecordTES4("DATA");

		// CNAM, float (crime gold multilier)
		float fCrimeGoldModifier = 1.0;
		esm.startSubRecordTES4("CNAM");
		esm.writeT<float>(fCrimeGoldModifier);
		esm.endSubRecordTES4("CNAM");

		// Ranks...
		for (int i=0; i < 10; i++)
		{
			if (mRanks[i].size() > 0)
			{
				// RNAM, rank
				esm.startSubRecordTES4("RNAM");
				esm.writeT<int32_t>(i);
				esm.endSubRecordTES4("RNAM");
				// MNAM, male name
				esm.startSubRecordTES4("MNAM");
				esm.writeHCString(mRanks[i]);
				esm.endSubRecordTES4("MNAM");
				// FNAM string (female name)
				// INAM string (insignia)
			}
		}
	}

    void Faction::blank()
    {
        mName.clear();
        mData.mAttribute[0] = mData.mAttribute[1] = 0;
        mData.mIsHidden = 0;

        for (int i=0; i<10; ++i)
        {
            mData.mRankData[i].mAttribute1 = mData.mRankData[i].mAttribute2 = 0;
            mData.mRankData[i].mSkill1 = mData.mRankData[i].mSkill2 = 0;
            mData.mRankData[i].mFactReaction = 0;

            mRanks[i].clear();
        }

        for (int i=0; i<7; ++i)
            mData.mSkills[i] = 0;

        mReactions.clear();
    }
}

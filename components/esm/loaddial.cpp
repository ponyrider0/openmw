#include "loaddial.hpp"

#include <iostream>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int Dialogue::sRecordId = REC_DIAL;

    void Dialogue::load(ESMReader &esm, bool &isDeleted)
    {
        loadId(esm);
        loadData(esm, isDeleted);
    }

    void Dialogue::loadId(ESMReader &esm)
    {
        mId = esm.getHNString("NAME");
    }

    void Dialogue::loadData(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'D','A','T','A'>::value:
                {
                    esm.getSubHeader();
                    int size = esm.getSubSize();
                    if (size == 1)
                    {
                        esm.getT(mType);
                    }
                    else
                    {
                        esm.skip(size);
                    }
                    break;
                }
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    mType = Unknown;
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }
    }

    void Dialogue::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);
        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
        }
        else
        {
            esm.writeHNT("DATA", mType);
        }
    }

	void Dialogue::exportTESx(ESMWriter & esm, int export_format) const
	{

		bool bIsQuest=false;
		bool bIsTopic=false;
		bool bIsGreeting=false;
		bool bIsVoice=false;

		uint32_t tempFormID;
		std::string strEDID, thisSIG, tempStr;
		std::ostringstream tempStream, debugstream;

		uint8_t newType = 0;
		switch (mType)
		{
		case ESM::Dialogue::Journal:
			bIsQuest = true;
			thisSIG = "QUST";
			break;
		case ESM::Dialogue::Topic:
			bIsTopic=true;
			thisSIG = "DIAL";
			break;
		case ESM::Dialogue::Greeting:
			bIsGreeting = true;
			thisSIG = "DIAL";
			std::cout << "WARNING: Dialog export: unhandled condition, dialog type is Greeting." << std::endl;
			break;
		case ESM::Dialogue::Voice: // contains newTypes: Combat, Detection
			newType = 1;
			bIsVoice = true;
			thisSIG = "DIAL";
			std::cout << "WARNING: Dialog export: unhandled condition, dialog type is Voice." << std::endl;
			break;
		case ESM::Dialogue::Persuasion:
			newType = 3;
			bIsVoice = true;
			thisSIG = "DIAL";
			std::cout << "WARNING: Dialog export: unhandled condition, dialog type is Voice." << std::endl;
			break;
		default:
			newType = 6;
			thisSIG = "DIAL";
			std::cout << "WARNING: Dialog export: unhandled condition, dialog type is unknown." << std::endl;
			break;
		}

		strEDID = esm.generateEDIDTES4(mId, 4, thisSIG);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(strEDID);
		esm.endSubRecordTES4("EDID");

		if (!bIsQuest)
		{
			uint32_t questFormID = esm.crossRefStringID("MorroDefaultQuest", "QUST", false);
			esm.startSubRecordTES4("QSTI");
			esm.writeT<uint32_t>(questFormID); // can have multiple
			esm.endSubRecordTES4("QSTI");

			if (false)
			{
				esm.startSubRecordTES4("QSTR");
				esm.writeT<uint32_t>(0);
				esm.endSubRecordTES4("QSTR");
			}
		}

		esm.startSubRecordTES4("FULL");
		esm.writeHCString(mId);
		esm.endSubRecordTES4("FULL");

		if (bIsQuest)
		{
			// ICON
		}

		if (!bIsQuest)
		{
			esm.startSubRecordTES4("DATA");
			esm.writeT<uint8_t>(newType);
			esm.endSubRecordTES4("DATA");
		}

		if (bIsQuest)
		{
			uint8_t questFlags = 0; // start game enabled = 0x01; allow repeat topics=0x04; allow repeat stages=0x08;
			uint8_t questPriority = 0;
			esm.startSubRecordTES4("DATA");
			esm.writeT<uint8_t>(questFlags);
			esm.writeT<uint8_t>(questPriority);
			esm.endSubRecordTES4("DATA");

			// SCRI ... script FormID

			// quest conditions to activate quest
			// CTDA...

			// quest stages
			// INDX, stage index
			// QSDT, stage flags
			// CNAM, stage log entry
			// SCHR, stage result script data
			// SCDA, compiled result script
			// SCTX, result script source
			// SCRO, formID for each global reference
			
			// quest targets
			// QSTA, formID
			// CTDA, target conditions
		}

	}

    void Dialogue::blank()
    {
        mInfo.clear();
    }

    void Dialogue::readInfo(ESMReader &esm, bool merge)
    {
        ESM::DialInfo info;
        bool isDeleted = false;
        info.load(esm, isDeleted);

        if (!merge || mInfo.empty())
        {
            mLookup[info.mId] = std::make_pair(mInfo.insert(mInfo.end(), info), isDeleted);
            return;
        }

        InfoContainer::iterator it = mInfo.end();

        LookupMap::iterator lookup;
        lookup = mLookup.find(info.mId);

        if (lookup != mLookup.end())
        {
            it = lookup->second.first;
            // Since the new version of this record may have changed the next/prev linked list connection, we need to re-insert the record
            mInfo.erase(it);
            mLookup.erase(lookup);
        }

        if (info.mNext.empty())
        {
            mLookup[info.mId] = std::make_pair(mInfo.insert(mInfo.end(), info), isDeleted);
            return;
        }
        if (info.mPrev.empty())
        {
            mLookup[info.mId] = std::make_pair(mInfo.insert(mInfo.begin(), info), isDeleted);
            return;
        }

        lookup = mLookup.find(info.mPrev);
        if (lookup != mLookup.end())
        {
            it = lookup->second.first;

            mLookup[info.mId] = std::make_pair(mInfo.insert(++it, info), isDeleted);
            return;
        }

        lookup = mLookup.find(info.mNext);
        if (lookup != mLookup.end())
        {
            it = lookup->second.first;

            mLookup[info.mId] = std::make_pair(mInfo.insert(it, info), isDeleted);
            return;
        }

        std::cerr << "Warning: Failed to insert info " << info.mId << std::endl;
    }

    void Dialogue::clearDeletedInfos()
    {
        LookupMap::const_iterator current = mLookup.begin();
        LookupMap::const_iterator end = mLookup.end();
        for (; current != end; ++current)
        {
            if (current->second.second)
            {
                mInfo.erase(current->second.first);
            }
        }
        mLookup.clear();
    }
}

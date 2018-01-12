#include "loadlevlist.hpp"
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

namespace ESM
{
    void LevelledListBase::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;

        bool hasName = false;
        bool hasList = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mFlags);
                    break;
                case ESM::FourCC<'N','N','A','M'>::value:
                    esm.getHT(mChanceNone);
                    break;
                case ESM::FourCC<'I','N','D','X'>::value:
                {
                    int length = 0;
                    esm.getHT(length);
                    mList.resize(length);

                    // If this levelled list was already loaded by a previous content file,
                    // we overwrite the list. Merging lists should probably be left to external tools,
                    // with the limited amount of information there is in the records, all merging methods
                    // will be flawed in some way. For a proper fix the ESM format would have to be changed
                    // to actually track list changes instead of including the whole list for every file
                    // that does something with that list.
                    for (size_t i = 0; i < mList.size(); i++)
                    {
                        LevelItem &li = mList[i];
                        li.mId = esm.getHNString(mRecName);
                        esm.getHNT(li.mLevel, "INTV");
                    }

                    hasList = true;
                    break;
                }
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                {
                    if (!hasList)
                    {
                        // Original engine ignores rest of the record, even if there are items following
                        mList.clear();
                        esm.skipRecord();
                    }
                    else
                    {
                        esm.fail("Unknown subrecord");
                    }
                    break;
                }
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
    }

    void LevelledListBase::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNT("DATA", mFlags);
        esm.writeHNT("NNAM", mChanceNone);
        esm.writeHNT<int>("INDX", mList.size());

        for (std::vector<LevelItem>::const_iterator it = mList.begin(); it != mList.end(); ++it)
        {
            esm.writeHNCString(mRecName, it->mId);
            esm.writeHNT("INTV", it->mLevel);
        }
    }

	bool ItemLevList::exportTESx(ESMWriter &esm, int export_format) const
	{
		std::stringstream errStream;

		std::string newEDID = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(newEDID);
		esm.endSubRecordTES4("EDID");

		// Chance, LVLD
		esm.startSubRecordTES4("LVLD");
		esm.writeT<unsigned char>(mChanceNone);
		esm.endSubRecordTES4("LVLD");

		// Flags, LVLF
		unsigned char flags=0;
		if (mFlags & ESM::ItemLevList::Flags::AllLevels)
			flags |= 0x01;
		esm.startSubRecordTES4("LVLF");
		esm.writeT<unsigned char>(flags);
		esm.endSubRecordTES4("LVLF");

		// for each mList record, write LVLO subrecord
		// LVLO
		std::vector<LevelItem>::const_iterator it_LVLO = mList.begin();
		if (it_LVLO == mList.end())
		{
			errStream << "WARNING, Leveled List contains no items: " << newEDID << std::endl;
		}
		for (it_LVLO=mList.begin(); it_LVLO != mList.end(); it_LVLO++)
		{
			esm.startSubRecordTES4("LVLO");
			esm.writeT<short>(it_LVLO->mLevel); // level
			esm.writeT<uint16_t>(0); // unknown?
			std::string itemEDID = esm.generateEDIDTES4(it_LVLO->mId);
			itemEDID = esm.substituteMorroblivionEDID(itemEDID, (ESM::RecNameInts) sRecordId);
			uint32_t refID = esm.crossRefStringID(itemEDID, "LVLI", false);
			if (refID != 0)
			{
				esm.writeT<uint32_t>(refID); //formID
			}
			else
			{
				errStream << "WARNING: LVLI[" << newEDID << "] Can't resolve formID for item[" << itemEDID << "]" << std::endl;
				esm.writeT<uint32_t>(0x19116); //formID=Fork
			}
			esm.writeT<uint16_t>(1); //count
			esm.writeT<uint16_t>(0); //unknown
			esm.endSubRecordTES4("LVLO");
		}

		esm.startSubRecordTES4("DATA");
		esm.writeT<unsigned char>(0); // unused
		esm.endSubRecordTES4("DATA");

		OutputDebugString(errStream.str().c_str());

		return true;
	}

    void LevelledListBase::blank()
    {
        mFlags = 0;
        mChanceNone = 0;
        mList.clear();
    }

	bool CreatureLevList::exportTESx(ESMWriter &esm, int export_format) const
	{
		std::stringstream errStream;

		// export LVC
		std::string newEDID = esm.generateEDIDTES4(mId);
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(newEDID);
		esm.endSubRecordTES4("EDID");

		// Chance, LVLD
		esm.startSubRecordTES4("LVLD");
		esm.writeT<unsigned char>(mChanceNone);
		esm.endSubRecordTES4("LVLD");

		// Flags, LVLF
		unsigned char flags=0;
		if (mFlags & ESM::CreatureLevList::Flags::AllLevels)
			flags |= 0x01;
		esm.startSubRecordTES4("LVLF");
		esm.writeT<unsigned char>(flags);
		esm.endSubRecordTES4("LVLF");

		// for each mList record, write LVLO subrecord
		// LVLO
		std::vector<LevelItem>::const_iterator it_LVLO = mList.begin();
		if (it_LVLO == mList.end())
		{
//			std::cout << "WARNING, Leveled Creature List contains no creatures: " << newEDID << std::endl;
			errStream << "WARNING, Leveled Creature List contains no creatures: " << newEDID << std::endl;
		}
		for (it_LVLO=mList.begin(); it_LVLO != mList.end(); it_LVLO++)
		{
			esm.startSubRecordTES4("LVLO");
			esm.writeT<short>(it_LVLO->mLevel); // level
			esm.writeT<uint16_t>(0); // unknown?
			// creature reference ID
			std::string itemEDID = esm.generateEDIDTES4(it_LVLO->mId);
			itemEDID = esm.substituteMorroblivionEDID(itemEDID, (ESM::RecNameInts)sRecordId);
			uint32_t refID = esm.crossRefStringID(itemEDID, "LVLC", false);
			if (refID != 0)
			{
				esm.writeT<uint32_t>(refID); //formID
			}
			else
			{
//				std::cout << "WARNING: LVLC[" << newEDID << "] Can't resolve formID for creature[" << itemEDID << "]" << std::endl;
				errStream << "WARNING: LVLC[" << newEDID << "] Can't resolve formID for creature[" << itemEDID << "]" << std::endl;
				esm.writeT<uint32_t>(0x1D0B6); //formID=TestBear
			}
			esm.writeT<uint16_t>(1); //count
			esm.writeT<uint16_t>(0); //unknown
			esm.endSubRecordTES4("LVLO");
		}

/*
		// script formID, SCRI
		// creature template formID, TNAM
*/
		OutputDebugString(errStream.str().c_str());

		return true;
	}

    unsigned int CreatureLevList::sRecordId = REC_LEVC;

    unsigned int ItemLevList::sRecordId = REC_LEVI;
}

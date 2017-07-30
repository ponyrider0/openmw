#include "loadlevlist.hpp"

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

	bool LevelledListBase::exportTESx(ESMWriter &esm, int export_format) const
	{
		return false;
	}

    void LevelledListBase::blank()
    {
        mFlags = 0;
        mChanceNone = 0;
        mList.clear();
    }

	bool CreatureLevList::exportTESx(ESMWriter &esm, int export_format) const
	{
		// export LVC
		esm.startSubRecordTES4("EDID");
		esm.writeHCString(mId);
		esm.endSubRecordTES4("EDID");

		// Chance, LVLD
		esm.startSubRecordTES4("LVLD");
		esm.writeT<char>(0);
		esm.endSubRecordTES4("LVLD");

		// Flags, LVLF
		esm.startSubRecordTES4("LVLF");
		esm.writeT<unsigned char>(0);
		esm.endSubRecordTES4("LVLF");

		// script formID, SCRI
		esm.startSubRecordTES4("SCRI");
		esm.writeT<uint32_t>(0);
		esm.endSubRecordTES4("SCRI");

		// creature template formID, TNAM
		esm.startSubRecordTES4("TNAM");
		esm.writeT<uint32_t>(0);
		esm.endSubRecordTES4("TNAM");

		// LVLO
		esm.startSubRecordTES4("LVLO");
		esm.writeT<uint16_t>(0);
		esm.writeT<uint16_t>(0);
		esm.writeT<uint32_t>(0);
		esm.writeT<uint16_t>(0);
		esm.writeT<uint16_t>(0);
		esm.endSubRecordTES4("LVLO");

		return true;
	}

    unsigned int CreatureLevList::sRecordId = REC_LEVC;

    unsigned int ItemLevList::sRecordId = REC_LEVI;
}

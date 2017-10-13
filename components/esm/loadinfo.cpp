#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

namespace ESM
{
    unsigned int DialInfo::sRecordId = REC_INFO;

    void DialInfo::load(ESMReader &esm, bool &isDeleted)
    {
        mId = esm.getHNString("INAM");

        isDeleted = false;

        mQuestStatus = QS_None;
        mFactionLess = false;

        mPrev = esm.getHNString("PNAM");
        mNext = esm.getHNString("NNAM");

        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().intval)
            {
                case ESM::FourCC<'D','A','T','A'>::value:
                    esm.getHT(mData, 12);
                    break;
                case ESM::FourCC<'O','N','A','M'>::value:
                    mActor = esm.getHString();
                    break;
                case ESM::FourCC<'R','N','A','M'>::value:
                    mRace = esm.getHString();
                    break;
                case ESM::FourCC<'C','N','A','M'>::value:
                    mClass = esm.getHString();
                    break;
                case ESM::FourCC<'F','N','A','M'>::value:
                {
                    mFaction = esm.getHString();
                    if (mFaction == "FFFF")
                    {
                        mFactionLess = true;
                    }
                    break;
                }
                case ESM::FourCC<'A','N','A','M'>::value:
                    mCell = esm.getHString();
                    break;
                case ESM::FourCC<'D','N','A','M'>::value:
                    mPcFaction = esm.getHString();
                    break;
                case ESM::FourCC<'S','N','A','M'>::value:
                    mSound = esm.getHString();
                    break;
                case ESM::SREC_NAME:
                    mResponse = esm.getHString();
                    break;
                case ESM::FourCC<'S','C','V','R'>::value:
                {
                    SelectStruct ss;
                    ss.mSelectRule = esm.getHString();
                    ss.mValue.read(esm, Variant::Format_Info);
                    mSelects.push_back(ss);
                    break;
                }
                case ESM::FourCC<'B','N','A','M'>::value:
                    mResultScript = esm.getHString();
                    break;
                case ESM::FourCC<'Q','S','T','N'>::value:
                    mQuestStatus = QS_Name;
                    esm.skipRecord();
                    break;
                case ESM::FourCC<'Q','S','T','F'>::value:
                    mQuestStatus = QS_Finished;
                    esm.skipRecord();
                    break;
                case ESM::FourCC<'Q','S','T','R'>::value:
                    mQuestStatus = QS_Restart;
                    esm.skipRecord();
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
    }

    void DialInfo::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("INAM", mId);
        esm.writeHNCString("PNAM", mPrev);
        esm.writeHNCString("NNAM", mNext);

        if (isDeleted)
        {
            esm.writeHNCString("DELE", "");
            return;
        }

        esm.writeHNT("DATA", mData, 12);
        esm.writeHNOCString("ONAM", mActor);
        esm.writeHNOCString("RNAM", mRace);
        esm.writeHNOCString("CNAM", mClass);
        esm.writeHNOCString("FNAM", mFaction);
        esm.writeHNOCString("ANAM", mCell);
        esm.writeHNOCString("DNAM", mPcFaction);
        esm.writeHNOCString("SNAM", mSound);
        esm.writeHNOString("NAME", mResponse);

        for (std::vector<SelectStruct>::const_iterator it = mSelects.begin(); it != mSelects.end(); ++it)
        {
            esm.writeHNString("SCVR", it->mSelectRule);
            it->mValue.write (esm, Variant::Format_Info);
        }

        esm.writeHNOString("BNAM", mResultScript);

        switch(mQuestStatus)
        {
        case QS_Name: esm.writeHNT("QSTN",'\1'); break;
        case QS_Finished: esm.writeHNT("QSTF", '\1'); break;
        case QS_Restart: esm.writeHNT("QSTR", '\1'); break;
        default: break;
        }
    }

	void DialInfo::exportTESx(ESMWriter & esm, int export_format, int newType) const
	{
		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream tempStream, debugstream;
		strEDID = esm.generateEDIDTES4(mId, 4);

		esm.startSubRecordTES4("DATA");
		esm.writeT<uint8_t>(newType); // Type
		esm.writeT<uint8_t>(0); // next speaker (target=0, self=1, either=2)
		esm.writeT<uint8_t>(0); // flags (goodbye=1, random=2, say once=4, run immed.=8, info refusal=10, rand end=20, rumors=40)
		esm.endSubRecordTES4("DATA");

		uint32_t questFormID = esm.crossRefStringID("MorroDefaultQuest", "QUST", false);
		esm.startSubRecordTES4("QSTI");
		esm.writeT<uint32_t>(questFormID); // can have multiple
		esm.endSubRecordTES4("QSTI");

		// dialog topic
		uint32_t topicFormID = 0;
//		topicFormID = esm.crossRefStringID("", "DIAL", false);
		if (topicFormID != 0)
		{
			esm.startSubRecordTES4("TPIC");
			esm.writeT<uint32_t>(topicFormID); // can have multiple
			esm.endSubRecordTES4("TPIC");
		}

		// prev topic record
		uint32_t infoFormID = 0;
		if (mPrev != "")
		{
			std::string prevEDID = "info#" + mPrev;
			infoFormID = esm.crossRefStringID(prevEDID, "INFO", false);
			if (infoFormID == 0)
			{
//				infoFormID = esm.reserveFormID(0, prevEDID, "INFO");
			}
		}
		esm.startSubRecordTES4("PNAM");
		esm.writeT<uint32_t>(infoFormID); // can have multiple
		esm.endSubRecordTES4("PNAM");

		// Add Topics
		// NAME...

		// responses
		int responseCounter=1;
		int currentOffset=0;
		bool bHyphenateStart=false, bHyphenateEnd=false;
		int lineMax = 150;
		while (currentOffset < mResponse.size() )
		{
			int bytesToRead=0;
			// check against lineMax+10 to avoid a final line which is shorter than 10 characters.
			if ( (mResponse.size() - currentOffset) < lineMax+10)
			{
				bytesToRead = mResponse.size() - currentOffset;
			}
			else
			{
				while (bytesToRead < lineMax)
				{
					int lookAhead1 = mResponse.find(". ", currentOffset+bytesToRead) - currentOffset;
					int lookAhead2 = mResponse.find(", ", currentOffset+bytesToRead) - currentOffset;
					if (lookAhead1 > lineMax) lookAhead1 = -1;
					if (lookAhead2 > lineMax) lookAhead2 = -1;
					if (lookAhead1 < bytesToRead && lookAhead2 < bytesToRead)
					{
						break;
					}
					bytesToRead = (lookAhead1 > lookAhead2) ? lookAhead1 : lookAhead2;
					bytesToRead += 2; // advance by the size of search term ". " or ", "
				}

				if (bytesToRead == 0)
				{
					// set to lineMax, then slowly backtrack until we see whitepace
					bytesToRead = (currentOffset + lineMax);
					while (mResponse[bytesToRead] != ' ')
					{
						bytesToRead--;
						// sanity check in case there is no whitespace.
						if (bytesToRead == 0)
						{
							bytesToRead = lineMax;
							bHyphenateEnd=true;
							break;
						}
					}
				}
			}
			std::string partialString = mResponse.substr(currentOffset, bytesToRead);
			currentOffset += bytesToRead;
			if (bHyphenateStart == true)
			{
				bHyphenateStart=false;
				partialString = '-' + partialString;
			}
			if (bHyphenateEnd == true)
			{
				bHyphenateStart=true;
				bHyphenateEnd=false;
				partialString = partialString + '-';
			}
			esm.startSubRecordTES4("TRDT");
			esm.writeT<uint32_t>(0); // uint32 emotion type (neutral=0, anger=1,disgust=2, fear=3, sad=4, happy=5, surprise=6)
			esm.writeT<int32_t>(50); // int32 emotion val
			esm.writeT<uint32_t>(0);// uint8 x4 (unused)
			esm.writeT<uint8_t>(responseCounter++); // uint8 response number (start with 1)
			esm.writeT<uint8_t>(0);// uint8 x3 (unused)
			esm.writeT<uint8_t>(0);
			esm.writeT<uint8_t>(0);
			esm.endSubRecordTES4("TRDT");
			// , NAM1 (response text 1, 2, ...)
			esm.startSubRecordTES4("NAM1");
			esm.writeHCString(partialString);
			esm.endSubRecordTES4("NAM1");
			// , NAM2 (actor notes)
			std::string actorNotesString="";
			esm.startSubRecordTES4("NAM2");
			esm.writeHCString(actorNotesString);
			esm.endSubRecordTES4("NAM2");
		}

		// Conditions CTDAs...
		int numConditions=0;
		for (int i=0; i < numConditions; i++)
		{
			esm.startSubRecordTES4("CTDA");
			esm.writeT<uint8_t>(0); // type
			esm.writeT<uint8_t>(0); // unused x3
			esm.writeT<uint8_t>(0); // unused x3
			esm.writeT<uint8_t>(0); // unused x3
			esm.writeT<float>(0); // comparison value
			esm.writeT<uint32_t>(0); // comparison function
			esm.writeT<uint32_t>(0); // comparison argument
			esm.endSubRecordTES4("CTDA");
		}
		// ...

		// Choices TCLT
		std::vector<std::string> choiceList;
		for (auto choice = choiceList.begin(); choice != choiceList.end(); choice++)
		{
			// resolve choice
			uint32_t choiceFormID = esm.crossRefStringID(*choice, "DIAL", false);
			if (choiceFormID != 0)
			{
				esm.startSubRecordTES4("TCLT");
				esm.writeT<uint32_t>(choiceFormID); // dialog formID
				esm.endSubRecordTES4("TCLT");
			}
		}

		// Link From TCLF

		// Result Script...
		// SCHR...
		// SCDA
		// SCTX
		// references


	}

    void DialInfo::blank()
    {
        mData.mUnknown1 = 0;
        mData.mDisposition = 0;
        mData.mRank = 0;
        mData.mGender = 0;
        mData.mPCrank = 0;
        mData.mUnknown2 = 0;

        mSelects.clear();
        mPrev.clear();
        mNext.clear();
        mActor.clear();
        mRace.clear();
        mClass.clear();
        mFaction.clear();
        mPcFaction.clear();
        mCell.clear();
        mSound.clear();
        mResponse.clear();
        mResultScript.clear();
        mFactionLess = false;
        mQuestStatus = QS_None;
    }
}

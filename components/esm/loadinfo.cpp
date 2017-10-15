#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

#include <apps/opencs/model/world/infoselectwrapper.hpp>
#include <iostream>

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
		// hard-coded conditions based on ESM datastructs
		// first condition == IsActor
		if (mActor != "")
		{
			uint32_t actorFormID = esm.crossRefStringID(mActor, "NPC_");
			uint32_t compareFunction = 0x0048; // GetIsID (decimal 72)
			esm.exportConditionalExpression(compareFunction, actorFormID, "=", 1.0);
		}
		if (mRace != "")
		{
			uint32_t raceFormID = esm.crossRefStringID(mRace, "RACE");
			uint32_t compareFunction = 0x0045; // GetIsRace (decimal 69)
			esm.exportConditionalExpression(compareFunction, raceFormID, "=", 1.0);
		}
		if (mClass != "")
		{
			uint32_t classFormID = esm.crossRefStringID(mClass, "CLAS");
			uint32_t compareFunction = 0x0044; // GetIsClass (decimal 68)
			esm.exportConditionalExpression(compareFunction, classFormID, "=", 1.0);
		}
		if (mFaction != "")
		{
			uint32_t factionFormID = esm.crossRefStringID(mFaction, "FACT");
			uint32_t compareFunction = 0x0049; // GetFactionRank (decimal 73)
			float factionRankVal = mData.mRank;
			esm.exportConditionalExpression(compareFunction, factionFormID, ">", factionRankVal);
		}
		if (mPcFaction != "")
		{
			uint32_t pcFactFormID = esm.crossRefStringID(mPcFaction, "FACT");
			uint32_t compareFunction = 0x0049; // GetFactionRank (decimal 73)
			uint8_t flags = 0x02; // run on target (aka PC)
			float factionRankVal = mData.mPCrank;
			esm.exportConditionalExpression(compareFunction, pcFactFormID, ">", factionRankVal, flags);
		}
		if (mCell != "")
		{
			uint32_t cellFormID = esm.crossRefStringID(mCell, "CELL");
			uint32_t compareFunction = 0x0043; // GetInCell (decimal 67)
			esm.exportConditionalExpression(compareFunction, cellFormID, "=", 1.0);
		}

		for (auto selectItem = mSelects.begin(); selectItem != mSelects.end(); selectItem++)
		{
			CSMWorld::ConstInfoSelectWrapper selectWrapper(*selectItem);
			uint32_t compareFunction=0;
			uint32_t compareArg1=0;
			uint32_t compareArg2=0;
			std::string compareOperator="";
			float compareVal=0.0f;
			uint8_t flags=0;

			switch (selectWrapper.getRelationType())
			{
			case CSMWorld::ConstInfoSelectWrapper::Relation_Equal:
				compareOperator = "=";
				break;
			case CSMWorld::ConstInfoSelectWrapper::Relation_Greater:
				compareOperator = ">";
				break;
			case CSMWorld::ConstInfoSelectWrapper::Relation_GreaterOrEqual:
				compareOperator = ">=";
				break;
			case CSMWorld::ConstInfoSelectWrapper::Relation_Less:
				compareOperator = "<";
				break;
			case CSMWorld::ConstInfoSelectWrapper::Relation_LessOrEqual:
				compareOperator = "<=";
				break;
			case CSMWorld::ConstInfoSelectWrapper::Relation_NotEqual:
				compareOperator = "!=";
				break;
			}

			std::string sSIG = "";
			std::string varName = "";
			switch (selectWrapper.getFunctionName())
			{
			case CSMWorld::ConstInfoSelectWrapper::Function_Choice:
				// just skip since this is already rolled into ChoiceTopic
				break;
			case CSMWorld::ConstInfoSelectWrapper::Function_Journal:
				compareFunction = 0x3A; // GetStage (int 58)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "QUST");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcExpelled:
				compareFunction = 0xC1; // GetPCExpelled (int 193)
				compareArg1 = esm.crossRefStringID(mFaction, "FACT");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
//				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Global:
				compareFunction = 0x4A; // GetGlobalValue (int 74)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "GLOB");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_SameFaction:
				compareFunction = 0x2A; // SameFaction (int 42)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "FACT");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_RankRequirement:
				// must change into Rank eligibility??
				// rank == 1 (duty/loyalty) not enough faction reputation
				// rank == 2 (skills)
				// rank == 3 Eligible, add in resultscript: PCRaiseRank to levelup rank
				compareFunction = 0x0; // ?
				varName = selectWrapper.getVariableName();
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				std::cout << selectWrapper.toString() << std::endl;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Item:
				compareFunction = 0x2F; // GetItemCount (int 47)
				varName = selectWrapper.getVariableName();
				if ( esm.mStringTypeMap.find(varName) != esm.mStringTypeMap.end())
					sSIG = esm.mStringTypeMap.find(varName)->second;
				compareArg1 = esm.crossRefStringID(varName, sSIG);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
//				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_NotId:
				compareFunction = 0x48; // GetIsID (int 72)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "NPC_");
				compareOperator = "=";
				compareVal = 0.0f;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcGender:
				compareFunction = 0x46; // GetIsSex (int 70)
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Local:
				varName = selectWrapper.getVariableName();
				if (Misc::StringUtils::lowerCase(varName) == "nolore")
				{
					compareFunction = 0x47; // GetInFaction (int 71)
					compareArg1 = esm.crossRefStringID("noLore", "FACT", false);
					compareOperator = "=";
					compareVal = 1.0f;
					break;
				}
				else if (Misc::StringUtils::lowerCase(varName) == "tr_map")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "state")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "spelltaught")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "control")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "controlq")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "wantsdeath")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "demoted")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "slavestatus")
				{

				}
				else if (Misc::StringUtils::lowerCase(varName) == "cattlekilled")
				{

				} // codaflower, firepetal, goldkanet, goldensedge, heather, noblesedge, stoneflower, timsacomeby, blackrose, redfireflower, blackanther, hozgub, bandits, pcdiseased, dayspassed, localdayspassed, drinks, warned, placeonce, gocommonersrewardcount, ptmessagedelivered, ptslavesrewardcount, ptcommonersrewardcount, answered
				compareFunction = 0x35; // GetScriptVariable
				compareArg1 = 0x0;
				compareArg2 = 1;
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				std::cout << "Local: " << varName << " " << compareOperator << " " << compareVal << std::endl;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Dead:
				compareFunction = 0x54; // GetDeadCount (int 84)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "NPC_");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();				
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcReputation:
				compareFunction = 0xF9; // GetPCFame (int 249)
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcCrimeLevel:
				compareFunction = 0x74; // GetCrimeGold (int 116)
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_NotLocal:
				varName = selectWrapper.getVariableName();
				if (Misc::StringUtils::lowerCase(varName) == "nolore")
				{
					compareFunction = 0x47; // GetInFaction (int 71)
					compareArg1 = esm.crossRefStringID("noLore", "FACT", false);
					compareOperator = "=";
					compareVal = 0.0f;
					break;
				}
				compareFunction = 0x4F; // GetQuestVariable (int 79)
				std::cout << "Not Local: " << varName << std::endl;
				compareArg1 = esm.crossRefStringID("MorroDefaultQuest", "QUST", false, false);
				compareArg2 = 1;
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				std::cout << selectWrapper.toString() << std::endl;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_SameSex:
				compareFunction = 0x2C; // SameSex (decimal 206)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "NPC_");
				compareOperator = "=";
				compareVal = 1.0f;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_NotClass:
				compareFunction = 0x44; // GetIsClass (decimal 68)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "CLAS");
				compareOperator = "=";
				compareVal = 0.0f;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_NotCell:
				compareFunction = 0x43; // GetInCell (decimal 67)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "CELL");
				compareOperator = "=";
				compareVal = 0.0f;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_NotFaction:
				compareFunction = 0x47; // GetInFaction (decimal 71)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "FACT");
				compareOperator = "=";
				compareVal = 0.0f;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_CreatureTarget:
				compareFunction = 0x0; // ?
				varName = selectWrapper.getVariableName();
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				std::cout << selectWrapper.toString() << std::endl;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcDestruction:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Destruction);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Hello:
				compareFunction = 0x0; // ?
				varName = selectWrapper.getVariableName();
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				std::cout << selectWrapper.toString() << std::endl;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_NotRace:
				compareFunction = 0x45; // GetIsRace
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "RACE");
				compareOperator = "=";
				compareVal = 0.0f;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Reputation:
				compareFunction = 0x0; // ?
				varName = selectWrapper.getVariableName();
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				std::cout << selectWrapper.toString() << std::endl;
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcClothingModifier:
				compareFunction = 0x29; // GetClothingVal
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcCommonDisease:
				compareFunction = 0x27; // GetDisease
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcBlightDisease:
				// TODO: replace with user quest var
				compareFunction = 0x27; // GetDisease
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_TalkedToPc:
				compareFunction = 0x32; // GetTalkedToPC
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcEnchant:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Enchant);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcIntelligence:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Intelligence);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcSneak:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Sneak);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcSecurity:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Security);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Level:
				compareFunction = 0x50; // GetLevel
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcConjuration:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Conjuration);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_FactionRankDifference:
				compareFunction = 0x3C; // GetFactionRankDifference
				compareArg1 = esm.crossRefStringID(mFaction, "FACT");
				varName = selectWrapper.getVariableName();
				compareArg2 = esm.crossRefStringID(varName, "NPC_");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcAlchemy:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Alchemy);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcVampire:
				compareFunction = 0x28; // GetVampire (decimal 14)
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcStrength:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Strength);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcPersonality:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Personality);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int )
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcMerchantile:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Mercantile);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcSpeechcraft:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				compareArg1 = esm.skillToActorValTES4(ESM::Skill::Speechcraft);
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_PcLevel:
				compareFunction = 0x50; // GetLevel
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_Alarm:
				compareFunction = 0x3D; // GetAlarmed
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
//				flags = 0x02; // Run on Target
				break;

			case CSMWorld::ConstInfoSelectWrapper::Function_SameRace:
				compareFunction = 0x0E; // GetActorVal (decimal 14)
				varName = selectWrapper.getVariableName();
				compareArg1 = esm.crossRefStringID(varName, "NPC_");
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();
				break;

			default:
				// record stats on missing function and occurences
				std::cout << "ERROR: Conditional Export: no export for CompareFunction: " << selectWrapper.toString() << std::endl;
				break;
			}

			esm.exportConditionalExpression(compareFunction, compareArg1, compareOperator, compareVal, flags, compareArg2);

		}
		// ...

		//TODO: Process Result script early to obtain Choice statements
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

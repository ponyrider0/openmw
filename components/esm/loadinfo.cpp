#include "loadinfo.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "defs.hpp"

#include <apps/opencs/model/world/infoselectwrapper.hpp>
#include <iostream>
#include "scriptconverter.hpp"
#include "loadnpc.hpp"
#include <apps/opencs/model/doc/document.hpp>

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

	void DialInfo::exportTESx(CSMDoc::Document &doc, ESMWriter & esm, int export_format, int newType, const std::string& topicEDID, const std::vector<std::string>& addTopicList, uint32_t prevRecordID) const
	{
		bool bIsTopic=false;
		bool bIsGreeting=false;
		bool bIsQuestStage=false;
		bool bIsVoice=false;

		uint32_t tempFormID;
		std::string strEDID, tempStr;
		std::ostringstream debugstream;
		strEDID = esm.generateEDIDTES4(mId, 4);

		switch (newType)
		{
		case 0: // ESM4 Topic type
			bIsTopic=true;
			break;
		case 1: // ESM4 conversation type (voice) 
		case 2: // ESM4 combat type (voice) 
			bIsTopic=true;
			bIsVoice=true;
			break;
		case 7: // non-ESM type, hack for greetings
			bIsTopic = true;
			bIsGreeting=true;
			break;
		case 10: // non-ESM type, hack for quest stages
			bIsQuestStage = true;
			break;
		default:
			bIsTopic=true;
			break;
		}

		// process script early for use by other subrecords
		ESM::ScriptConverter scriptConverter(mResultScript, esm);
		// create new response string with substituted names early
		std::string newResponse = mResponse;
		// There are two passes for each variable: 
		//	the first pass is the ideal match which will replace the leading space,
		//	the second pass is a fuzzy match which will add an extra leading space.
		newResponse = Misc::StringUtils::replaceAll(newResponse, " %PCName", "&sUActnQuick1;");
		newResponse = Misc::StringUtils::replaceAll(newResponse, "%PCName", "&sUActnQuick1;");
		newResponse = Misc::StringUtils::replaceAll(newResponse, " %Name", "&sUActnQuick2;");
		newResponse = Misc::StringUtils::replaceAll(newResponse, "%Name", "&sUActnQuick2;");
		// first pass catches uncapitalized instances of words
		// second pass should catch remaining instances needing capitalization
		newResponse = Misc::StringUtils::replaceAll(newResponse, " %PCRace", " outlander");
		newResponse = Misc::StringUtils::replaceAll(newResponse, "%PCRace", "Outlander");
		newResponse = Misc::StringUtils::replaceAll(newResponse, " %PCClass", " outlander");
		newResponse = Misc::StringUtils::replaceAll(newResponse, "%PCClass", "Outlander");
		newResponse = Misc::StringUtils::replaceAll(newResponse, " %PCRank", " outlander");
		newResponse = Misc::StringUtils::replaceAll(newResponse, "%PCRank", "Outlander");

		if (bIsQuestStage)
		{
			// Stage Index
			int questStage = mData.mJournalIndex;
			esm.startSubRecordTES4("INDX");
			esm.writeT<int16_t>(questStage);
			esm.endSubRecordTES4("INDX");

			// QSDT, flags (0x01=complete quest)
			uint8_t flags=0;
			esm.startSubRecordTES4("QSDT");
			esm.writeT<uint8_t>(flags);
			esm.endSubRecordTES4("QSDT");

			// CTDAs for quest stage

			// CNAM, log entry
			esm.startSubRecordTES4("CNAM");
			esm.writeHCString(newResponse);
			esm.endSubRecordTES4("CNAM");

		}

		if (bIsTopic)
		{
			uint8_t flags = 0;
			if (scriptConverter.PerformGoodbye())
				flags |= 1;
			esm.startSubRecordTES4("DATA");
			esm.writeT<uint8_t>(newType); // Type
			esm.writeT<uint8_t>(0); // next speaker (target=0, self=1, either=2)
			esm.writeT<uint8_t>(flags); // flags (goodbye=1, random=2, say once=4, run immed.=8, info refusal=10, rand end=20, rumors=40)
			esm.endSubRecordTES4("DATA");

			uint32_t questFormID = esm.crossRefStringID("MorroDefaultQuest", "QUST", false);
			esm.startSubRecordTES4("QSTI");
			esm.writeT<uint32_t>(questFormID); // can have multiple
			esm.endSubRecordTES4("QSTI");

			// TPIC, formID [DIAL], dialog topic - unused??

			// PNAM, prev topic record
			// IF prevRecordID is present, it overrides stringID-based recordID resolution
			uint32_t infoFormID = 0;
			if (prevRecordID != 0)
			{
				infoFormID = prevRecordID;
			}
			else
			{
				if (mPrev != "")
				{
					std::string prevEDID = "info#" + mPrev;
					infoFormID = esm.crossRefStringID(prevEDID, "INFO", false);
					if (infoFormID == 0)
					{
						// can not use --> breaks unresolved.csv export function
						//				infoFormID = esm.reserveFormID(0, prevEDID, "INFO");
					}
				}
			}
			esm.startSubRecordTES4("PNAM");
			esm.writeT<uint32_t>(infoFormID); // can have multiple
			esm.endSubRecordTES4("PNAM");

			// NAME... Add Topics: Bugged behavior in TES4 engine: preferred method is by addtopic command in result script
			if (addTopicList.empty() != true)
			{
				uint32_t topicFormID = 0;
				for (auto addTopicItem = addTopicList.begin(); addTopicItem != addTopicList.end(); addTopicItem++)
				{
					topicFormID = esm.crossRefStringID(*addTopicItem, "DIAL", false);
					esm.startSubRecordTES4("NAME");
					esm.writeT<uint32_t>(topicFormID); // can have multiple
					esm.endSubRecordTES4("NAME");
				}
			}

			// TRDT, NAM1, NAM2... responses
			int responseCounter=1;
			int currentOffset=0;
			bool bHyphenateStart=false, bHyphenateEnd=false;
			int lineMax = 150;
			while (currentOffset < newResponse.size() )
			{
				int bytesToRead=0;
				// check against lineMax+10 to avoid a final line which is shorter than 10 characters.
				if ( (newResponse.size() - currentOffset) < lineMax+10)
				{
					bytesToRead = newResponse.size() - currentOffset;
				}
				else
				{
					while (bytesToRead < lineMax)
					{
						int lookAhead1 = newResponse.find(". ", currentOffset+bytesToRead) - currentOffset;
						int lookAhead2 = newResponse.find(", ", currentOffset+bytesToRead) - currentOffset;
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
						while (newResponse[bytesToRead] != ' ')
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
				std::string partialString = newResponse.substr(currentOffset, bytesToRead);
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

			// CTDAs... response conditions
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
				if (classFormID == 0)
				{
					std::cout << "ERROR: classFormID resolved to null: [" << mClass << "]\n";
				}
				else
				{
					uint32_t compareFunction = 0x0044; // GetIsClass (decimal 68)
					esm.exportConditionalExpression(compareFunction, classFormID, "=", 1.0);
				}
			}
			if (mFaction != "")
			{
				uint32_t factionFormID = esm.crossRefStringID(mFaction, "FACT");
// BREAK if factionFormID is null
				if (factionFormID == 0)
				{
					std::cout << "ERROR: FactionFormID resolved to null: [" << mFaction << "]\n";
				}
				else
				{
					uint32_t compareFunction = 0x0049; // GetFactionRank (decimal 73)
					float factionRankVal = mData.mRank;
					esm.exportConditionalExpression(compareFunction, factionFormID, ">", factionRankVal);
				}
			}
			if (mPcFaction != "")
			{
				uint32_t pcFactFormID = esm.crossRefStringID(mPcFaction, "FACT");
				if (pcFactFormID == 0)
				{
					std::cout << "ERROR: PCFactionFormID resolved to null: [" << mPcFaction << "]\n";
				}
				else
				{
					uint32_t compareFunction = 0x0049; // GetFactionRank (decimal 73)
					uint8_t flags = 0x02; // run on target (aka PC)
					float factionRankVal = mData.mPCrank;
					esm.exportConditionalExpression(compareFunction, pcFactFormID, ">", factionRankVal, flags);
				}
			}
			if (mCell != "")
			{
				uint32_t cellFormID = esm.crossRefStringID(mCell, "CELL");
				if (cellFormID != 0)
				{
					uint32_t compareFunction = 0x0043; // GetInCell (decimal 67)
					esm.exportConditionalExpression(compareFunction, cellFormID, "=", 1.0);
				}
				else
				{
					uint32_t compareFunction = 0x4F; // GetQuestVariable
					std::string varName = "GetIn_" + esm.generateEDIDTES4(mCell, 0, "CELL");
					varName = Misc::StringUtils::lowerCase( varName );
					uint32_t compareArg1 = esm.crossRefStringID("mwDialogHelper", "QUST", false);
					uint32_t compareArg2;
					if (esm.mLocalVarIndexmap.find(varName) == esm.mLocalVarIndexmap.end())
					{
						std::cout << "Unresolved Condition:[" << topicEDID << "] " << " mCell == [" << mCell << "]" << std::endl;
						if (esm.mUnresolvedLocalVars.find(varName) == esm.mUnresolvedLocalVars.end())
							esm.mUnresolvedLocalVars[varName] = 1;
						else
							esm.mUnresolvedLocalVars[varName] += 1;
//						compareFunction = 0;
						compareArg2 = 0;
					}
					else
					{
						compareArg2 = esm.mLocalVarIndexmap[varName];
					}
					if (compareFunction != 0)
						esm.exportConditionalExpression(compareFunction, compareArg1, "=", 1.0, 0, compareArg2);
				}
			}
			if (mData.mGender != ESM::DialInfo::Gender::NA)
			{
				uint32_t compareFunction = 0x46; // GetIsSex
				uint32_t compareArg1 = mData.mGender;
				esm.exportConditionalExpression(compareFunction, compareArg1, "=", 1.0);
			}
			if (mData.mDisposition > 0) // include dispo check if greater than 0
			{
				uint32_t compareFunction = 0x4C; // GetDisposition
				uint32_t compareArg1 = 0; // Target of conversation (player)
				float compareVal = mData.mDisposition;
				esm.exportConditionalExpression(compareFunction, compareArg1, ">=", compareVal);
			}

			for (auto selectItem = mSelects.begin(); selectItem != mSelects.end(); selectItem++)
			{
				CSMWorld::ConstInfoSelectWrapper selectWrapper(*selectItem);
				std::stringstream tempStream;
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
				if (selectWrapper.getVariant().getType() == ESM::VT_Float)
					compareVal = selectWrapper.getVariant().getFloat();
				if (selectWrapper.getVariant().getType() == ESM::VT_Int)
					compareVal = selectWrapper.getVariant().getInteger();

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
// BREAK if Quest value is null
					if (compareArg1 == 0)
					{
						std::cout << "Dialog INFO: Could not resolve quest varname: " << varName << "\n";
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcExpelled:
					compareFunction = 0xC1; // GetPCExpelled (int 193)
					// retrieve NPC's faction
					if (mActor != "")
					{
						auto npcIndexRecord = doc.getData().getReferenceables().getDataSet().searchId(mActor);
						std::string actorFaction = doc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(npcIndexRecord.first).get().mFaction;
						compareArg1 = esm.crossRefStringID(actorFaction, "FACT");
					}
					else
					{
						// TODO: record unresolved faction
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Global:
					varName = selectWrapper.getVariableName();
					if (Misc::StringUtils::lowerCase(varName) == "random100")
					{
						compareFunction = 0x4D; // GetRandomPercent
						break;
					}
					if (Misc::StringUtils::lowerCase(varName) == "pcrace")
					{
						compareFunction = 0x82; // GetPCIsRace
						int tempRaceVal = compareVal;
						uint32_t raceFormID;
						switch (tempRaceVal)
						{
						case 1: // argonian
							raceFormID = esm.crossRefStringID("argonian", "race");
							break;
						case 2: // breton
							raceFormID = esm.crossRefStringID("breton", "race");
							break;
						case 3: // dark elf
							raceFormID = esm.crossRefStringID("dark elf", "race");
							break;
						case 4: // high elf
							raceFormID = esm.crossRefStringID("high elf", "race");
							break;
						case 5: // imperial
							raceFormID = esm.crossRefStringID("imperial", "race");
							break;
						case 6: // khajiit
							raceFormID = esm.crossRefStringID("khajiit", "race");
							break;
						case 7: // nord
							raceFormID = esm.crossRefStringID("nord", "race");
							break;
						case 8: // orc
							raceFormID = esm.crossRefStringID("orc", "race");
							break;
						case 9: // redguard
							raceFormID = esm.crossRefStringID("redguard", "race");
							break;
						case 10: // wood elf
							raceFormID = esm.crossRefStringID("wood elf", "race");
							break;
						}
						compareArg1 = raceFormID;
						compareVal = 1;
						break;
					}
					compareFunction = 0x4A; // GetGlobalValue (int 74)
					compareArg1 = esm.crossRefStringID(varName, "GLOB");
// BREAK if Global is NULL
					if (compareArg1 == 0)
					{
						// TODO: record unresolved global varname
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_SameFaction:
/*
					compareFunction = 0x2A; // SameFaction (int 42)
					compareArg1 = 0;
					if (compareArg1 == 0)
					{
						auto npcIndexRecord = doc.getData().getReferenceables().getDataSet().searchId(mActor);
						std::string actorFaction = doc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(npcIndexRecord.first).get().mFaction;
						compareArg1 = esm.crossRefStringID(actorFaction, "FACT");
					}
*/
// BREAK if compareArg is NULL
					compareFunction = 0x85; // SameFactionAsPC
//					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_RankRequirement:
					// must change into Rank eligibility??
					// rank == 1 (duty/loyalty) not enough faction reputation
					// rank == 2 (skills)
					// rank == 3 Eligible, add in resultscript: PCRaiseRank to levelup rank
					compareFunction = 0x4F; // GetQuestVariable (int 79)
					compareArg1 = esm.crossRefStringID("mwDialogHelper", "QUST", false);
					varName = "Function_RankRequirement";
					varName = Misc::StringUtils::lowerCase( varName );
					if (esm.mLocalVarIndexmap.find(varName) == esm.mLocalVarIndexmap.end())
					{
						std::cout << "Unresolved Condition:[" << topicEDID << "] " << selectWrapper.toString() << std::endl;
						if (esm.mUnresolvedLocalVars.find(varName) == esm.mUnresolvedLocalVars.end())
							esm.mUnresolvedLocalVars[varName] = 1;
						else
							esm.mUnresolvedLocalVars[varName] += 1;
						compareFunction = 0;
						compareArg2 = 0;
					}
					else
					{
						compareArg2 = esm.mLocalVarIndexmap[varName];
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Item:
					compareFunction = 0x2F; // GetItemCount (int 47)
					varName = selectWrapper.getVariableName();
					if ( esm.mStringTypeMap.find(varName) != esm.mStringTypeMap.end())
						sSIG = esm.mStringTypeMap.find(varName)->second;
					compareArg1 = esm.crossRefStringID(varName, sSIG);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_NotId:
					compareOperator = flipCompareOperator(compareOperator);
					compareFunction = 0x48; // GetIsID (int 72)
					varName = selectWrapper.getVariableName();
					compareArg1 = esm.crossRefStringID(varName, "NPC_");
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcGender:
					compareFunction = 0x46; // GetIsSex (int 70)
					compareArg1 = 1; // female==1?
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Local:
					varName = Misc::StringUtils::lowerCase( selectWrapper.getVariableName() );
					if ( varName == "nolore")
					{
						compareFunction = 0x47; // GetInFaction (int 71)
						compareArg1 = esm.crossRefStringID("noLore", "FACT", false);
						break;
					}
					// use quest variable instead of directly accessing script-variable, so that
					//   variable indexes don't need to be hardcoded across multiple NPC scripts
					compareFunction = 0x4F; // GetQuestVariable (int 79)
					compareArg1 = esm.crossRefStringID("mwDialogHelper", "QUST", false);
					if (esm.mLocalVarIndexmap.find(varName) == esm.mLocalVarIndexmap.end())
					{
						std::cout << "Unresolved Condition:[" << topicEDID << "] " << selectWrapper.toString() << std::endl;
						if (esm.mUnresolvedLocalVars.find(varName) == esm.mUnresolvedLocalVars.end())
							esm.mUnresolvedLocalVars[varName] = 1;
						else
							esm.mUnresolvedLocalVars[varName] += 1;
						compareFunction = 0;
						compareArg2 = 0;
					}
					else
					{
						compareArg2 = esm.mLocalVarIndexmap[varName];
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Dead:
					compareFunction = 0x54; // GetDeadCount (int 84)
					varName = selectWrapper.getVariableName();
					compareArg1 = esm.crossRefStringID(varName, "NPC_");
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcReputation:
					compareFunction = 0xF9; // GetPCFame (int 249)
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcCrimeLevel:
					compareFunction = 0x74; // GetCrimeGold (int 116)
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_NotLocal:
					compareOperator = flipCompareOperator(compareOperator);
					varName = Misc::StringUtils::lowerCase( selectWrapper.getVariableName() );
					if (varName == "nolore")
					{
						compareFunction = 0x47; // GetInFaction (int 71)
						compareArg1 = esm.crossRefStringID("noLore", "FACT", false);
						break;
					}
					compareFunction = 0x4F; // GetQuestVariable (int 79)
					compareArg1 = esm.crossRefStringID("mwDialogHelper", "QUST", false);
					if (esm.mLocalVarIndexmap.find(varName) == esm.mLocalVarIndexmap.end())
					{
						std::cout << "Unresolved Condition:[" << topicEDID << "] " << selectWrapper.toString() << std::endl;
						if (esm.mUnresolvedLocalVars.find(varName) == esm.mUnresolvedLocalVars.end())
							esm.mUnresolvedLocalVars[varName] = 1;
						else
							esm.mUnresolvedLocalVars[varName] += 1;
						compareFunction = 0;
						compareArg2 = 0;
					}
					else
					{
						compareArg2 = esm.mLocalVarIndexmap[varName];
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_SameSex:
//					compareFunction = 0x2C; // SameSex (decimal 206)
//					varName = selectWrapper.getVariableName();
//					compareArg1 = esm.crossRefStringID(varName, "NPC_");
					compareFunction = 0x87; // SameSexAsPC
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_NotClass:
					compareOperator = flipCompareOperator(compareOperator);
					compareFunction = 0x44; // GetIsClass (decimal 68)
					varName = selectWrapper.getVariableName();
					compareArg1 = esm.crossRefStringID(varName, "CLAS");
// BREAK if compareArg is null
					if (compareArg1 == 0)
					{
						// TODO: record unresolved record
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_NotCell:
					compareOperator = flipCompareOperator(compareOperator);
					compareFunction = 0x43; // GetInCell (decimal 67)
					varName = selectWrapper.getVariableName();
					compareArg1 = esm.crossRefStringID(varName, "CELL");
// REPLACE CompareFunction with equivalent REGION CompareFunction
//					if (compareArg1 == 0)
//						compareArg1 = esm.crossRefStringID(varName, "REGN");
					if (compareArg1 == 0)
					{
						compareFunction = 0x4F; // GetQuestVariable
						varName = "GetIn_" + esm.generateEDIDTES4(varName, 0, "CELL");
						varName = Misc::StringUtils::lowerCase( varName );
						compareArg1 = esm.crossRefStringID("mwDialogHelper", "QUST", false);
						if (esm.mLocalVarIndexmap.find(varName) == esm.mLocalVarIndexmap.end())
						{
							std::cout << "Unresolved Condition:[" << topicEDID << "] " << " mCell == [" << mCell << "]" << std::endl;
							if (esm.mUnresolvedLocalVars.find(varName) == esm.mUnresolvedLocalVars.end())
								esm.mUnresolvedLocalVars[varName] = 1;
							else
								esm.mUnresolvedLocalVars[varName] += 1;
							compareFunction = 0;
							compareArg2 = 0;
						}
						else
						{
							compareArg2 = esm.mLocalVarIndexmap[varName];
						}
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_NotFaction:
					compareOperator = flipCompareOperator(compareOperator);
					compareFunction = 0x47; // GetInFaction (decimal 71)
					varName = selectWrapper.getVariableName();
					compareArg1 = esm.crossRefStringID(varName, "FACT");
					if (compareArg1 == 0)
					{
						// TODO: record unresolved record
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_CreatureTarget:
					compareFunction = 0x40; // GetIsCreature
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcDestruction:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Destruction);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Hello:
					compareFunction = 0x0; // ?
					varName = selectWrapper.getVariableName();
					std::cout << "Unresolved Condition: [" << topicEDID << "] " << selectWrapper.toString() << std::endl;
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_NotRace:
					compareOperator = flipCompareOperator(compareOperator);
					compareFunction = 0x45; // GetIsRace
					varName = selectWrapper.getVariableName();
					compareArg1 = esm.crossRefStringID(varName, "RACE");
					if (compareArg1 == 0)
					{
						// TODO: record unresolved record
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Reputation:
					compareFunction = 0x0; // ?
					varName = selectWrapper.getVariableName();
					std::cout << selectWrapper.toString() << std::endl;
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcClothingModifier:
					compareFunction = 0x29; // GetClothingVal
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcCommonDisease:
					compareFunction = 0x27; // GetDisease
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcBlightDisease:
					// TODO: replace with user quest var
					compareFunction = 0x27; // GetDisease
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_TalkedToPc:
					compareFunction = 0x32; // GetTalkedToPC
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcEnchant:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Enchant);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcIntelligence:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Intelligence);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcAgility:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Agility);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcSneak:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Sneak);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcSecurity:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Security);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Level:
					compareFunction = 0x50; // GetLevel
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcConjuration:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Conjuration);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_FactionRankDifference:
					compareFunction = 0x3C; // GetFactionRankDifference
					compareArg2 = esm.crossRefStringID(mActor, "NPC_");
					if (compareArg2 != 0)
					{
						auto npcIndexRecord = doc.getData().getReferenceables().getDataSet().searchId(mActor);
						std::string actorFaction = doc.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(npcIndexRecord.first).get().mFaction;
						compareArg1 = esm.crossRefStringID(actorFaction, "FACT");
					}
					else
					{
						// TODO: record unresolved record
						compareFunction = 0;
						break;
					}
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcAlchemy:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Alchemy);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcVampire:
					compareFunction = 0x28; // GetVampire (decimal 14)
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcStrength:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Strength);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcPersonality:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.attributeToActorValTES4(ESM::Attribute::Personality);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcMerchantile:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Mercantile);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcSpeechcraft:
					compareFunction = 0x0E; // GetActorVal (decimal 14)
					compareArg1 = esm.skillToActorValTES4(ESM::Skill::Speechcraft);
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_PcLevel:
					compareFunction = 0x50; // GetLevel
					flags = 0x02; // Run on Target
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_Alarm:
					compareFunction = 0x3D; // GetAlarmed
					// TODO: replace with generalized Alarm == val
					break;

				case CSMWorld::ConstInfoSelectWrapper::Function_SameRace:
					compareFunction = 0x86; // SameRaceAsPC
					break;

				default:
					// record stats on missing function and occurences
					std::cout << "Unhandled Condition: [" << topicEDID << "] " << selectWrapper.toString() << std::endl;
					break;
				}

// DISABLED TO FIX CSE CRASHES
				if (compareFunction != 0)
					esm.exportConditionalExpression(compareFunction, compareArg1, compareOperator, compareVal, flags, compareArg2);

			} // for each mSelects

		} // (bIsTopic)

		if (bIsTopic)
		{
			// Choice... TCLT (DIAL records)
			for (auto choice = scriptConverter.mChoicesList.begin(); choice != scriptConverter.mChoicesList.end(); choice++)
			{
				// resolve choice
				std::stringstream choiceTopicStr;
				choiceTopicStr << topicEDID << "Choice" << choice->first;
				uint32_t choiceFormID = esm.crossRefStringID(choiceTopicStr.str(), "DIAL", false);
				if (choiceFormID != 0)
				{
					esm.startSubRecordTES4("TCLT");
					esm.writeT<uint32_t>(choiceFormID); // dialog formID
					esm.endSubRecordTES4("TCLT");
				}
			}
			// Link From... TCLF (DIAL records)
			// ....

		} // (bIsTopic)

		// Result Script block
		// SCHR... (basic script data)
		// [unused x4, refcount, compiled size, varcount, script type]
		uint32_t refCount = scriptConverter.mReferenceList.size();
		uint32_t compiledSize = scriptConverter.GetByteBufferSize();
		uint32_t varCount = scriptConverter.mLocalVarList.size();
		uint32_t scriptType; // Object=0x0, Quest=0x01, MagicEffect=0x100
		if (bIsQuestStage)
			scriptType = 0x01;
		else
			scriptType = 0x0;

		esm.startSubRecordTES4("SCHR");
		esm.writeT<uint8_t>(0); // unused * 4
		esm.writeT<uint8_t>(0); // unused * 4
		esm.writeT<uint8_t>(0); // unused * 4
		esm.writeT<uint8_t>(0); // unused * 4
		esm.writeT<uint32_t>(0); // refcount
		esm.writeT<uint32_t>(0); // compiledsize
		esm.writeT<uint32_t>(0); // varcount
		esm.writeT<uint32_t>(0); // scripttype
//		esm.writeT<uint32_t>(refCount);
//		esm.writeT<uint32_t>(compiledSize);
//		esm.writeT<uint32_t>(varCount);
//		esm.writeT<uint32_t>(scriptType);
		esm.endSubRecordTES4("SCHR");

/*
		// SCDA... (compiled script)
		esm.startSubRecordTES4("SCDA");
		esm.write(0, 0);
//		esm.write(scriptConverter.GetCompiledByteBuffer(), scriptConverter.GetByteBufferSize());
		esm.endSubRecordTES4("SCDA");

		// SCTX... (script source text)
		esm.startSubRecordTES4("SCTX");
		esm.writeHCString(scriptConverter.GetConvertedScript());
		esm.endSubRecordTES4("SCTX");

		// SCRO, global references
		for (auto refItem = scriptConverter.mReferenceList.begin(); refItem != scriptConverter.mReferenceList.end(); refItem++)
		{
			esm.startSubRecordTES4("SCRO");
			esm.writeT<uint32_t>(*refItem); 
			esm.endSubRecordTES4("SCRO");
		}
*/
	}

	std::string DialInfo::flipCompareOperator(const std::string & compareOp) const
	{
		std::string flippedOperator;

		if (compareOp == "=")
			flippedOperator = "!=";
		else if (compareOp == ">")
			flippedOperator = "<=";
		else if (compareOp == ">=")
			flippedOperator = "<";
		else if (compareOp == "<")
			flippedOperator = ">=";
		else if (compareOp == "<=")
			flippedOperator = ">";

		return flippedOperator;
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

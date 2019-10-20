#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#define SKIP_MASTER_RECORDS true

#include <components/misc/stringops.hpp>
#include <boost/filesystem.hpp>
#include <QUndoStack>

#include "exportToTES4.hpp"
#include "document.hpp"
#include <components/esm/scriptconverter.hpp>
#include <components/vfs/manager.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <osg/Image>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <components/esm/loadarmo.hpp>

#include <components/nif/niffile.hpp>

/*
namespace
{

    char normalize_char(char ch)
    {
        return ( ch == '\\' ) ? '/' : ch;
    }

    std::string get_normalized_path(const std::string& path_orig)
    {
        std::string path = "";

        for (auto ci = path_orig.begin(); ci != path_orig.end(); ci++)
        {
            path += normalize_char(*ci);
        }

        return path;
    }
    
}
*/

CSMDoc::ExportToTES4::ExportToTES4() : ExportToBase()
{
    std::cout << "TES4 Exporter Initialized " << std::endl;
}

void CSMDoc::ExportToTES4::defineExportOperation(Document& currentDoc, SavingState& currentSave)
{
	bool bDoGlobals = true;
	bool bDoScripts = true;
	bool bDoSpells = true;
	bool bDoRaces = true;
	bool bDoSounds = true;
	bool bDoClasses = true;
	bool bDoFactions = true;
	bool bDoEnchantments = true;
	bool bDoLandTextures = false;
	bool bDoWeapons = true;
	bool bDoAmmo = true;
	bool bDoMisc = true;
	bool bDoKeys = true;
	bool bDoSoulgems = true;
	bool bDoLights = true;
	bool bDoIngredients = true;
	bool bDoClothing = true;
	bool bDoBooks = true;
	bool bDoArmor = true;
	bool bDoApparati = true;
	bool bDoPotions = true;
	bool bDoLeveledItems = true;
	bool bDoFurniture = true;
	bool bDoDoors = true;
	bool bDoContainers = true;
	bool bDoFlora = true;
	bool bDoNPCs = true;
	bool bDoCreatures = true;
	bool bDoLeveledCreatures = true;
	bool bDoReferences = true;
	bool bDoExteriors = true;
	bool bDoInteriors = true;

	bool bDoRegions = false;
	bool bDoDialog = true;
	bool bDoQuests = true;
	bool bDoActivators = true;
	bool bDoStatics = true;

	bool bVWD_Only = false;
	bool bStatics_Only = false;
	bool skipMasterRecords = SKIP_MASTER_RECORDS;
//	bool bLTEX_Override = skipMasterRecords;
	bool bPluginless_Override = false;

	std::string esmName = currentDoc.getSavePath().filename().stem().string();
	if (esmName == "Morrowind" ||
		esmName == "Tribunal" ||
		esmName == "Bloodmoon")
	{
		esmName = "pluginless export";
        bDoGlobals = false;
        bDoScripts = false;
        bDoSpells = false;
        bDoRaces = false;
        bDoClasses = false;
        bDoFactions = false;
        bDoEnchantments = false;
        bDoLeveledItems = false;
        bDoLeveledCreatures = false;
        bDoNPCs = false;
        bDoReferences = false;
        bDoExteriors = false;
        bDoInteriors = false;
        bDoRegions = false;
        bDoDialog = false;
        bDoQuests = false;
		bPluginless_Override = true;
	}

#ifdef _WIN32
	std::string csvRoot = "";
#else
	std::string csvRoot = getenv("HOME");
	csvRoot += "/";
#endif
	currentSave.loadESMMastersMap(csvRoot + "ESMMastersMap.csv", esmName);

	// Export to ESM file
	appendStage (new OpenExportTES4Stage (currentDoc, currentSave, true));
	appendStage (new ExportHeaderTES4Stage (currentDoc, currentSave, false));

	ESM::ESMWriter &esm = currentSave.getWriter();

	if (esm.mConversionOptions.find("#enablemasters") != std::string::npos)
		skipMasterRecords = false;

	if (esm.mConversionOptions.find("#enableregions") != std::string::npos)
		bDoRegions = true;

	if (esm.mConversionOptions.find("#skipdialog") != std::string::npos)
		bDoDialog = false;

	if (esm.mConversionOptions.find("#skipquests") != std::string::npos)
		bDoQuests = false;

	if (esm.mConversionOptions.find("#vwdonly") != std::string::npos)
	{
		bVWD_Only = true;
	}

	if (esm.mConversionOptions.find("#staticsonly") != std::string::npos)
	{
		bStatics_Only = true;
	}

	if (bVWD_Only || bStatics_Only)
	{
		bDoGlobals = false;
		bDoScripts = false;
		bDoSpells = false;
		bDoRaces = false;
		bDoSounds = false;
		bDoClasses = false;
		bDoFactions = false;
		bDoEnchantments = false;
		bDoLandTextures = false;
		bDoWeapons = false;
		bDoAmmo = false;
		bDoMisc = false;
		bDoKeys = false;
		bDoSoulgems = false;
		bDoLights = false;
		bDoIngredients = false;
		bDoClothing = false;
		bDoBooks = false;
		bDoArmor = false;
		bDoApparati = false;
		bDoPotions = false;
		bDoLeveledItems = false;
		bDoFurniture = false;
		bDoDoors = false;
		bDoContainers = false;
		bDoFlora = false;
		bDoNPCs = false;
		bDoCreatures = false;
		bDoLeveledCreatures = false;
		bDoReferences = false;
		bDoExteriors = false;
		bDoInteriors = false;
		bDoRegions = false;
		bDoDialog = false;
		bDoQuests = false;
	}

	if (bStatics_Only)
	{
		bDoStatics = true;
		bDoActivators = false;
	}

	if (esm.mConversionOptions.find("#ltex") != std::string::npos)
	{
		bDoLandTextures = true;
//		bLTEX_Override = true;
	}

	if (bPluginless_Override)
	{
//		bDoDoors = true;
//		bDoLandTextures = true;
//		bLTEX_Override = true;
	}

	if (esm.mConversionOptions.find("#clothing") != std::string::npos)
	{
		bDoClothing = true;
	}

	if (esm.mConversionOptions.find("#armor") != std::string::npos)
	{
		bDoArmor = true;
	}

	if (esm.mConversionOptions.find("#skipcreatures") != std::string::npos)
	{
		bDoCreatures = false;
	}

	if (esm.mConversionOptions.find("#skipnpcs") != std::string::npos)
	{
		bDoNPCs = false;
	}


	//	appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::GameSetting> >
	//		(currentDoc.getData().getGmsts(), currentSave));

	//	appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BirthSign> >
	//		(currentDoc.getData().getBirthsigns(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	//	appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BodyPart> >
	//		(currentDoc.getData().getBodyParts(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	//	appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::SoundGenerator> >
	//		(currentDoc.getData().getSoundGens(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	//	appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::MagicEffect> >
	//		(currentDoc.getData().getMagicEffects(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	//	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Skill> >
	//		(currentDoc.getData().getSkills(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	if (bDoGlobals) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Global> >
		(currentDoc.getData().getGlobals(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	if (bDoScripts) appendStage(new ExportScriptTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoSpells) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Spell> >
		(currentDoc.getData().getSpells(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	if (bDoRaces) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Race> >
		(currentDoc.getData().getRaces(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	if (bDoSounds) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Sound> >
		(currentDoc.getData().getSounds(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	if (bDoClasses) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Class> >
		(currentDoc.getData().getClasses(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	if (bDoFactions) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Faction> >
		(currentDoc.getData().getFactions(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	if (bDoEnchantments) appendStage(new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Enchantment> >
		(currentDoc.getData().getEnchantments(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	if (bDoRegions) appendStage (new ExportRegionDataTES4Stage (currentDoc, currentSave, skipMasterRecords));
//	appendStage (new ExportClimateCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

	appendStage(new ExportLandTextureCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords, bDoLandTextures));

	if (bDoWeapons) appendStage(new ExportWeaponCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoAmmo) appendStage(new ExportAmmoCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoMisc) appendStage(new ExportMiscCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoKeys) appendStage(new ExportKeyCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoSoulgems) appendStage(new ExportSoulgemCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoLights) appendStage(new ExportLightCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoIngredients) appendStage(new ExportIngredientCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoClothing) appendStage(new ExportClothingCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoBooks) appendStage(new ExportBookCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoArmor) appendStage(new ExportArmorCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoApparati) appendStage(new ExportApparatusCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoPotions) appendStage(new ExportPotionCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoLeveledItems) appendStage(new ExportLeveledItemCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));

	if (bDoActivators) appendStage (new ExportActivatorCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	if (bDoStatics) appendStage (new ExportSTATCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));


	if (bDoFurniture) appendStage(new ExportFurnitureCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoDoors) appendStage(new ExportDoorCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoContainers) appendStage(new ExportContainerCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoFlora) appendStage(new ExportFloraCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));

	if (bDoNPCs) appendStage(new ExportNPCCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoCreatures) appendStage(new ExportCreatureCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));
	if (bDoLeveledCreatures) appendStage(new ExportLeveledCreatureCollectionTES4Stage(currentDoc, currentSave, skipMasterRecords));

	// CREATE WORLD REFERENCES, THEN INTERIOR & EXTERIOR WORLD
	if (bDoReferences) appendStage (new ExportReferenceCollectionTES4Stage (currentDoc, currentSave));
	if (bDoExteriors) appendStage (new ExportExteriorCellCollectionTES4Stage (currentDoc, currentSave));
	if (bDoInteriors) appendStage (new ExportInteriorCellCollectionTES4Stage (currentDoc, currentSave));

	// quests
	if (bDoQuests) appendStage(new ExportDialogueCollectionTES4Stage(currentDoc, currentSave, true, skipMasterRecords));
	// dialogs
	if (bDoDialog) appendStage(new ExportDialogueCollectionTES4Stage(currentDoc, currentSave, false, skipMasterRecords));

	// close file and clean up
	appendStage (new CloseExportTES4Stage (currentSave));
	appendStage (new FinalizeExportTES4Stage (currentDoc, currentSave));

}

CSMDoc::OpenExportTES4Stage::OpenExportTES4Stage (Document& document, SavingState& state, bool projectFile)
	: mDocument (document), mState (state), mProjectFile (projectFile)
{}

int CSMDoc::OpenExportTES4Stage::setup()
{
	return 1;
}

void CSMDoc::OpenExportTES4Stage::perform (int stage, Messages& messages)
{
	mState.start (mDocument, mProjectFile); // resets & clears iostream

	mState.getStream().open (
		mProjectFile ? mState.getPath() : mState.getTmpPath(),
		std::ios::binary);

	if (!mState.getStream().is_open())
	{
		throw std::runtime_error("failed to open stream for saving");
		std::cout << "ERROR: Could not open [" << mState.getPath() << "] for export.\n";
	}
}


CSMDoc::ExportHeaderTES4Stage::ExportHeaderTES4Stage (Document& document, SavingState& state, bool simple)
	: mDocument (document), mState (state), mSimple (simple)
{}

int CSMDoc::ExportHeaderTES4Stage::setup()
{
	mState.getWriter().setVersion();
	mState.getWriter().clearMaster();

	std::string esmName = mDocument.getSavePath().filename().stem().string();
	mState.getWriter().clearReservedFormIDs();
	mState.initializeSubstitutions(esmName);

	if (mSimple)
	{
		mState.getWriter().setAuthor ("");
		mState.getWriter().setDescription ("");
		mState.getWriter().setRecordCount (0);
		mState.getWriter().setFormat (ESM::Header::CurrentFormat);
	}
	else
	{
		mDocument.getData().getMetaData().exportTES4 (mState.getWriter());
		mState.getWriter().setRecordCount (
			mDocument.getData().countTES3 (CSMWorld::RecordBase::State_Modified) +
			mDocument.getData().countTES3 (CSMWorld::RecordBase::State_ModifiedOnly) +
			mDocument.getData().countTES3 (CSMWorld::RecordBase::State_Deleted));

		/// \todo refine dependency list (at least remove redundant dependencies)
		std::vector<boost::filesystem::path> dependencies = mDocument.getContentFiles();
		std::vector<boost::filesystem::path>::const_iterator end (--dependencies.end());

/*
		for (std::vector<boost::filesystem::path>::const_iterator iter (dependencies.begin());
			iter!=end; ++iter)
		{
			std::string name = iter->filename().string();
			uint64_t size = boost::filesystem::file_size (*iter);
			
			mState.getWriter().addMaster (name, size);
		}
*/
		mState.getWriter().addMaster ("Oblivion.esm", 0);
		mState.getWriter().addMaster ("Morrowind_ob.esm", 0);
//		mState.getWriter().addMaster ("Morrowind_ob - UCWUS.esp", 0);
//		mState.getWriter().addMaster ("morroblivion-fixes.esp", 0);
		mState.getWriter().addMaster ("Morrowind_Compatibility_Layer.esp", 0);

/*
if ((esmName.find("TR_Mainland") != std::string::npos) ||
			(esmName.find("TR_Preview") != std::string::npos))
		{
			mState.getWriter().addMaster("Tamriel_Data.esp", 0);
		}
*/

		esmName = Misc::StringUtils::lowerCase(esmName);
		if (mState.getWriter().mESMMastersmap.find(esmName) != mState.getWriter().mESMMastersmap.end())
		{
			std::vector<std::string> masterList = mState.getWriter().mESMMastersmap[esmName];
			for (auto masterItem = masterList.begin(); masterItem != masterList.end(); masterItem++)
			{
				mState.getWriter().addMaster(*masterItem, 0);
			}
		}

	}

	return 1;
}

void CSMDoc::ExportHeaderTES4Stage::perform (int stage, Messages& messages)
{
	mState.getWriter().exportTES4 (mState.getStream());
}


CSMDoc::ExportDialogueCollectionTES4Stage::ExportDialogueCollectionTES4Stage (Document& document,
	SavingState& state, bool processQuests, bool skipMasters)
	: mDocument(document), mState (state),
	mTopics (processQuests ? document.getData().getJournals() : document.getData().getTopics()),
	mInfos (processQuests ? document.getData().getJournalInfos() : document.getData().getTopicInfos()),
	mSkipMasterRecords (skipMasters),
	mQuestMode (processQuests)
{}

int CSMDoc::ExportDialogueCollectionTES4Stage::setup()
{
	ESM::ESMWriter& writer = mState.getWriter();

	if (mQuestMode)
	{
		// Loop through StartScript Collection and record scriptIDs to auto start (via questscripts)
		int numStartScripts = mDocument.getData().getStartScripts().getSize();
		for (int i = 0; i < numStartScripts; i++)
		{
			auto autostartRecord = mDocument.getData().getStartScripts().getRecord(i);
			bool bExportRecord = false;
			if (mSkipMasterRecords)
				bExportRecord |= autostartRecord.isModified() | autostartRecord.isDeleted();
			else
				bExportRecord = true;
			if (bExportRecord)
			{
				// retrieve script reference
				std::string scriptEDID = writer.generateEDIDTES4(autostartRecord.get().mId, 0, "SCPT");
				std::string scriptEDID_lowercase = Misc::StringUtils::lowerCase(scriptEDID);
				// add scriptID to autostart list
				writer.mAutoStartScriptEDID[scriptEDID_lowercase] = 1;
				// add to scriptquest list if not already there
				writer.RegisterScriptToQuest(autostartRecord.get().mId);
			}
		}

	}

	int collectionSize = mTopics.getSize();
	for (int i=0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Dialogue>& topic = mTopics.getRecord(i);
		ESM::Dialogue dial = topic.get();
		std::string keyphrase = Misc::StringUtils::lowerCase(dial.mId);
		mKeyPhraseList.push_back(keyphrase);

		bool exportMe = false;
		if (mSkipMasterRecords == true)
		{
			if (topic.isModified() || topic.isDeleted())
				exportMe = true;
		}
		else
		{
			exportMe = true;
		}

		if (exportMe == true)
		{
			mActiveRecords.push_back(i);
		}

	}
	
	if (mActiveRecords.size() == 0)
	{
		// return one so we can later check if any ScriptQuests need to be generated
		return 1;
	}

	return mActiveRecords.size();
}

std::vector<std::string> CSMDoc::ExportDialogueCollectionTES4Stage::CreateAddTopicList(std::string infoText)
{
	std::string searchText = Misc::StringUtils::lowerCase(infoText);
	std::vector<std::string> addTopicList;

	for (auto keyphrase = mKeyPhraseList.begin(); keyphrase != mKeyPhraseList.end(); keyphrase++)
	{
		int searchResult;
		if ( (searchResult = searchText.find(*keyphrase)) != searchText.npos)
		{
			if (searchResult == 0 || (searchText[searchResult - 1] < 'a' || searchText[searchResult - 1] > 'z'))
			{
				addTopicList.push_back(mState.getWriter().generateEDIDTES4(*keyphrase, 4));
			}
		}
	}

	return addTopicList;
}

void CSMDoc::ExportDialogueCollectionTES4Stage::appendSpecialRecords()
{
	ESM::ESMWriter& writer = mState.getWriter();

	int totalGreetingSize = 0;
	for (int i=0; i < 10; i++)
		totalGreetingSize += mGreetingInfoList[i].size();

	// Append special records to end of group (GREETING topic, ScriptToQuests)
	if (mQuestMode == false && totalGreetingSize > 0)
	{
		// write out GREETINGS info
		std::string infoTopic = "GREETING";
		uint32_t greetingID = writer.crossRefStringID(infoTopic, "DIAL", false, true);
		if (greetingID == 0)
		{
			greetingID = writer.reserveFormID(greetingID, infoTopic, "DIAL");
		}
		uint32_t flags = 0;
		writer.startRecordTES4("DIAL", flags, greetingID, infoTopic);
		writer.startSubRecordTES4("EDID");
		writer.writeHCString(infoTopic);
		writer.endSubRecordTES4("EDID");
		writer.startSubRecordTES4("FULL");
		writer.writeHCString(infoTopic);
		writer.endSubRecordTES4("FULL");
		writer.startSubRecordTES4("DATA");
		writer.writeT<uint8_t>(0);
		writer.endSubRecordTES4("DATA");
		writer.endRecordTES4("DIAL");
		writer.startGroupTES4(greetingID, 7);

		uint32_t prevRecordID = 0;
		char duplicateChar = 'A';
		for (int i = 0; i < 10; i++)
		{
			for (auto greetingItem = mGreetingInfoList[i].begin(); greetingItem != mGreetingInfoList[i].end(); greetingItem++)
			{
				std::string infoEDID = "info#" + greetingItem->second + greetingItem->first.mId;
				uint32_t infoFormID = writer.crossRefStringID(infoEDID, "INFO", false, true);
				if (infoFormID == 0)
				{
					infoFormID = writer.reserveFormID(infoFormID, infoEDID, "INFO");
				}
				uint32_t infoFlags = 0;
				// ***** didn't store records to check deleted/disabled status *****

				// avoid creating duplicate info records
				if ((infoFormID != 0) && (writer.mUniqueIDcheck.find(infoFormID) != writer.mUniqueIDcheck.end()))
				{
					// do not write, issue warning
					std::cout << "ESMWRITER WARNING: duplicate INFO record detected, will skip: (" << greetingItem->second << ") [" << std::hex << infoFormID << "]" << std::endl;
				}
				else
				{
					writer.startRecordTES4("INFO", infoFlags, infoFormID, infoEDID);
					greetingItem->first.exportTESx(mDocument, writer, 4, 0, greetingItem->second, CreateAddTopicList(greetingItem->first.mResponse), prevRecordID);
					writer.endRecordTES4("INFO");
					prevRecordID = infoFormID;
				}

			}

		}
		writer.endGroupTES4(greetingID);

		if (mHelloInfoList.empty() != true)
		{
			// write out HELLO info
			uint32_t helloID = writer.crossRefStringID("HELLO", "DIAL", false, true);
			flags = 0;
			writer.startRecordTES4("DIAL", flags, helloID, "HELLO");
			writer.startSubRecordTES4("EDID");
			writer.writeHCString("HELLO");
			writer.endSubRecordTES4("EDID");
			writer.startSubRecordTES4("FULL");
			writer.writeHCString("HELLO");
			writer.endSubRecordTES4("FULL");
			writer.startSubRecordTES4("DATA");
			writer.writeT<uint8_t>(0);
			writer.endSubRecordTES4("DATA");
			writer.endRecordTES4("DIAL");
			writer.startGroupTES4(helloID, 7);
			for (auto helloItem = mHelloInfoList.begin(); helloItem != mHelloInfoList.end(); helloItem++)
			{
				std::string infoEDID = "info#" + helloItem->second + helloItem->first.mId;
				uint32_t infoFormID = writer.crossRefStringID(infoEDID, "INFO", false, true);
				uint32_t infoFlags = 0;
				//			if (topic.isDeleted()) infoFlags |= 0x800; // DISABLED
				writer.startRecordTES4("INFO", infoFlags, infoFormID, infoEDID);
				//			helloItem->first.exportTESx(mDocument, writer, 4, 0, helloItem->second);
				helloItem->first.exportTESx(mDocument, writer, 4, 0, helloItem->second, CreateAddTopicList(helloItem->first.mResponse));
				writer.endRecordTES4("INFO");
			}
			writer.endGroupTES4(helloID);
		}
		
	}
	else if (mQuestMode == true && writer.mScriptToQuestList.size() > 0)
	{
		// QUEST MODE == true
		for (auto questItem = writer.mScriptToQuestList.begin(); questItem != writer.mScriptToQuestList.end(); questItem++)
		{
			std::string scriptEDID = "";
			std::string questEDID = "";
			if (questItem->second.first == "")
			{
				scriptEDID = questItem->first;
				questEDID = "mw" + scriptEDID + "quest";
			}
			else
			{
				scriptEDID = writer.generateEDIDTES4(questItem->second.first, 0, "SCPT");
				questEDID = writer.generateEDIDTES4(questItem->second.first, 0, "SQUST");
			}

			uint32_t questFormID = writer.crossRefStringID(questEDID, "QUST", false, true);
			if (questFormID == 0)
			{
				questFormID = writer.reserveFormID(questFormID, questEDID, "QUST");
			}
			uint32_t scriptFormID = writer.crossRefStringID(scriptEDID, "SCPT", false, true);
			if (scriptFormID == 0)
			{
				std::stringstream errMesg;
				errMesg << "Export ScriptQuest: could not find formID for script: " << scriptEDID << std::endl;
				OutputDebugString(errMesg.str().c_str());
			}
			uint32_t flags = 0;
			// ******************* did not store record data to lookup deleted/disabled status
//			if (topic.isDeleted()) flags |= 0x800; // DISABLED
			writer.startRecordTES4("QUST", flags, questFormID, questEDID);

			writer.startSubRecordTES4("EDID");
			writer.writeHCString(questEDID);
			writer.endSubRecordTES4("EDID");

			if (scriptFormID != 0)
			{
				writer.startSubRecordTES4("SCRI");
				writer.writeT<uint32_t>(scriptFormID);
				writer.endSubRecordTES4("SCRI");
			}

			// crossref autostart
			uint8_t questFlags = 0; // start game enabled = 0x01; allow repeat topics=0x04; allow repeat stages=0x08;
			std::string scriptEDID_lowercase = Misc::StringUtils::lowerCase(scriptEDID);
			if (writer.mAutoStartScriptEDID.find(scriptEDID_lowercase) != writer.mAutoStartScriptEDID.end())
			{
				questFlags |= 0x01;
			}
			uint8_t questPriority = 0;
			writer.startSubRecordTES4("DATA");
			writer.writeT<uint8_t>(questFlags);
			writer.writeT<uint8_t>(questPriority);
			writer.endSubRecordTES4("DATA");

			writer.endRecordTES4("QUST");
		}

	}

}

void CSMDoc::ExportDialogueCollectionTES4Stage::perform (int stage, Messages& messages)
{
	bool bIsGreeting=false;
	bool bHasInfoGroup=false;
	ESM::ESMWriter& writer = mState.getWriter();
	std::string sSIG;

	if (mQuestMode)
		sSIG = "QUST";
	else
		sSIG = "DIAL";

	if (stage == 0)
	{
		if (mActiveRecords.size() > 0)
		{
			writer.startGroupTES4(sSIG, 0);
		}
		else
		{
			// check for existence of special records
			if (mQuestMode == true && writer.mScriptToQuestList.size() > 0)
			{
				writer.startGroupTES4(sSIG, 0);
				appendSpecialRecords();
				writer.endGroupTES4(sSIG);
			}
			return;
		}
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Dialogue>& topic = mTopics.getRecord(recordIndex);

	if (topic.isDeleted() && mQuestMode==false)
	{
		// if the topic is deleted, we do not need to bother with INFO records.
		ESM::Dialogue dialogue = topic.get();
		std::string strEDID = writer.generateEDIDTES4(dialogue.mId, 4);
		uint32_t formID = writer.crossRefStringID(strEDID, "DIAL", false, true);
		uint32_t flags = 0;
		flags |= 0x800; // DISABLED
		writer.startRecordTES4("DIAL", flags, formID, strEDID);
		dialogue.exportTESx(writer, 4);
		writer.endRecordTES4("DIAL");

		return;
	}

	// Test, if we need to save anything associated info records.
	bool infoModified = false;
	CSMWorld::InfoCollection::Range range = mInfos.getTopicRange (topic.get().mId);

	for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second; ++iter)
	{
		if (iter->isModified() || iter->mState == CSMWorld::RecordBase::State_Deleted)
		{
			infoModified = true;
			break;
		}
	}

	std::map<std::string, std::vector<ESM::DialInfo> > infoChoiceList;
	std::map<std::string, std::string> infoChoiceTopicNames;

	if (infoModified && mQuestMode==true)
	{
		ESM::Dialogue dialog;
		if (infoModified && topic.mState != CSMWorld::RecordBase::State_Modified
			&& topic.mState != CSMWorld::RecordBase::State_ModifiedOnly)
		{
			dialog = topic.mBase;
		}
		else
		{
			dialog = topic.mModified;
		}
		std::string topicEDID = writer.generateEDIDTES4(dialog.mId, 0, "QUST");
		uint32_t formID = writer.crossRefStringID(topicEDID, "QUST", false, true);
		if (formID == 0)
		{
			formID = writer.reserveFormID(formID, topicEDID, "QUST");
		}
		uint32_t flags = 0;
		if (topic.isDeleted()) flags |= 0x800; // DISABLED
		writer.startRecordTES4("QUST", flags, formID, topicEDID);
//		dialog.exportTESx(writer, 4); --broken design, can't use for quests
		writer.startSubRecordTES4("EDID");
		writer.writeHCString(topicEDID);
		writer.endSubRecordTES4("EDID");

		bool hasQuestData=false;
		ESM::DialInfo questDataRecord = range.first->get();
		std::string questFullName;
		if (questDataRecord.mData.mJournalIndex == 0)
		{
			hasQuestData=true;
		}
		// sanity check:
		if (hasQuestData)
		{
			questFullName = range.first->get().mResponse;
			writer.startSubRecordTES4("FULL");
			writer.writeHCString(questFullName);
			writer.endSubRecordTES4("FULL");
		}

		uint8_t questFlags = 0; // start game enabled = 0x01; allow repeat topics=0x04; allow repeat stages=0x08;
		uint8_t questPriority = 0;
		writer.startSubRecordTES4("DATA");
		writer.writeT<uint8_t>(questFlags);
		writer.writeT<uint8_t>(questPriority);
		writer.endSubRecordTES4("DATA");

		if (hasQuestData)
		{
			// use CTDAs as global conditions for quest
		}

		// write modified quest stages (info records)
		for (CSMWorld::InfoCollection::RecordConstIterator iter(range.first); iter != range.second; ++iter)
		{
			if (iter->isModified() || iter->mState == CSMWorld::RecordBase::State_Deleted)
			{
				ESM::DialInfo info = iter->get();
				info.mId = info.mId.substr(info.mId.find_last_of('#') + 1);

				info.mPrev = "";
				if (iter != range.first)
				{
					CSMWorld::InfoCollection::RecordConstIterator prev = iter;
					--prev;

					info.mPrev = prev->get().mId.substr(prev->get().mId.find_last_of('#') + 1);
				}

				CSMWorld::InfoCollection::RecordConstIterator next = iter;
				++next;

				info.mNext = "";
				if (next != range.second)
				{
					info.mNext = next->get().mId.substr(next->get().mId.find_last_of('#') + 1);
				}

				// quest stage structure [INDX, ... SCRO]
				// INDX, stage index
				// QSDT, stage flags
				// CNAM, stage log entry
				// SCHR, stage result script data
				// SCDA, compiled result script
				// SCTX, result script source
				// SCRO, formID for each global reference
				if (info.mData.mJournalIndex == 0)
					continue;

				// quest stage export
				info.exportTESx(mDocument, writer, 4, 10, topicEDID, std::vector<std::string>());
			}

		}

		// quest target list:
		// QSTA, formID
		// CTDA, target conditions
		writer.endRecordTES4("QUST");

	}

	if (infoModified && mQuestMode==false)
	{
		ESM::Dialogue dialog;
		if (infoModified && topic.mState != CSMWorld::RecordBase::State_Modified 
						&& topic.mState != CSMWorld::RecordBase::State_ModifiedOnly)
		{
			 dialog = topic.mBase;
		}
		else
		{
			dialog = topic.mModified;
		}
		// check if greeting, collect separately and write out at once
		std::string topicEDID = writer.generateEDIDTES4(dialog.mId, 4);
		uint32_t formID = writer.crossRefStringID(topicEDID, "DIAL", false, true);
		if (formID == 0)
		{
			formID = writer.reserveFormID(formID, topicEDID, "DIAL");
		}
		if ((Misc::StringUtils::lowerCase(dialog.mId).find("greeting ") != std::string::npos && dialog.mId.size() == 10)
			|| formID == 0xC8)
		{
			bIsGreeting = true;
		}
		if (!bIsGreeting)
		{
			uint32_t flags = 0;
			if (topic.isDeleted()) flags |= 0x800; // DISABLED
			writer.startRecordTES4("DIAL", flags, formID, topicEDID);
			// dialog topic export
			dialog.exportTESx(writer, 4);
			writer.endRecordTES4("DIAL");
		}

		// write modified selected info records
		uint32_t prevRecordID = 0;
		for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second; ++iter)
		{
			if (iter->isModified() || iter->isDeleted())
			{
				ESM::DialInfo info = iter->get();
				info.mId = info.mId.substr (info.mId.find_last_of ('#')+1);

				info.mPrev = "";
				if (iter!=range.first)
				{
					CSMWorld::InfoCollection::RecordConstIterator prev = iter;
					--prev;

					info.mPrev = prev->get().mId.substr (prev->get().mId.find_last_of ('#')+1);
				}

				CSMWorld::InfoCollection::RecordConstIterator next = iter;
				++next;

				info.mNext = "";
				if (next!=range.second)
				{
					info.mNext = next->get().mId.substr (next->get().mId.find_last_of ('#')+1);
				}

				// check Result script for names of future ChoiceTopics
				ESM::ScriptConverter scriptReader(info.mResultScript, writer, mDocument);
				scriptReader.ExtractChoices();
				for (auto choicePair = scriptReader.mChoicesList.begin(); choicePair != scriptReader.mChoicesList.end(); choicePair++)
				{
					std::string actorEDID = writer.generateEDIDTES4(info.mActor, 0, "NPC_");
					std::stringstream choiceTopicKey;
					choiceTopicKey << topicEDID << "X" << actorEDID << "X" << choicePair->first;
					if (choicePair->second == "")
					{
						std::string errorMesg = "ERROR: empty choice text: " + info.mId + "\n";
						OutputDebugString(errorMesg.c_str());
					}
					infoChoiceTopicNames.insert(std::make_pair(choiceTopicKey.str(), choicePair->second));
				}

				bool bIsHello = false;
				bool bSkipInfo = false;
				// iterate through INFO conditional statements to see if there is a Choice function used
				for (auto selectRule = info.mSelects.begin(); selectRule != info.mSelects.end(); selectRule++)
				{
					int choiceNum = 0;
					CSMWorld::ConstInfoSelectWrapper selectWrapper(*selectRule);
					if (selectWrapper.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_Choice)
					{
						std::string actorEDID = writer.generateEDIDTES4(info.mActor, 0, "NPC_");
						choiceNum = selectWrapper.getVariant().getInteger();
						std::stringstream choiceTopicKey;
						choiceTopicKey << topicEDID << "X" << actorEDID << "X" << choiceNum;
						infoChoiceList[choiceTopicKey.str()].push_back(info);
						bSkipInfo = true;

						uint32_t choiceformID = writer.crossRefStringID(choiceTopicKey.str(), "DIAL", false, true);
						if (choiceformID == 0)
						{
							choiceformID = writer.reserveFormID(choiceformID, choiceTopicKey.str(), "DIAL");
						}
						break;
					}

					if (selectWrapper.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_Hello)
					{
						bIsHello = true;
					}

				}

				if (bSkipInfo == true)
				{
					continue;
				}

				// only add to greeting list after bSkipInfo section above
				if (bIsGreeting == true)
				{
					if (dialog.mId.find("0") != std::string::npos)
						mGreetingInfoList[0].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("1") != std::string::npos)
						mGreetingInfoList[1].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("2") != std::string::npos)
						mGreetingInfoList[2].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("3") != std::string::npos)
						mGreetingInfoList[3].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("4") != std::string::npos)
						mGreetingInfoList[4].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("5") != std::string::npos)
						mGreetingInfoList[5].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("6") != std::string::npos)
						mGreetingInfoList[6].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("7") != std::string::npos)
						mGreetingInfoList[7].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("8") != std::string::npos)
						mGreetingInfoList[8].push_back(std::make_pair(info, topicEDID));
					else if (dialog.mId.find("9") != std::string::npos)
						mGreetingInfoList[9].push_back(std::make_pair(info, topicEDID));
					continue;
				}

				if (bIsHello)
				{
//					std::cout << "HELLO used by : [" << topicEDID << "] '" << info.mResponse <<"'" << std::endl;
					mHelloInfoList.push_back(std::make_pair(info, topicEDID));
					continue;
				}

				if (bHasInfoGroup == false)
				{
					bHasInfoGroup = true;
					// create child group
					writer.startGroupTES4(formID, 7);
				}

				std::string infoEDID = "info#" + topicEDID + info.mId;
				uint32_t infoFormID = writer.crossRefStringID(infoEDID, "INFO", false, true);
				int newType = 0;
				switch (topic.get().mType)
				{
				case ESM::Dialogue::Topic:
					newType = 0;
					break;
				case ESM::Dialogue::Voice:
					newType = 1;
					break;
				case ESM::Dialogue::Greeting:
					newType = 0;
					break;
				case ESM::Dialogue::Persuasion:
					newType = 3;
					break;
				case ESM::Dialogue::Journal:
					// issue error
					throw std::runtime_error("ERROR: unexpected journal/quest data while processing dialog");
					abort();
					break;
				case ESM::Dialogue::Unknown:
					newType = 0;
					break;
				default:
					newType = 0;
				}
				uint32_t infoFlags = 0;
				if (topic.isDeleted()) infoFlags |= 0x800; // DISABLED
				bool bSuccess;

				// avoid creating duplicate info records
				if ((infoFormID != 0) && (writer.mUniqueIDcheck.find(infoFormID) != writer.mUniqueIDcheck.end()))
				{
					// do not write, issue warning
					std::cout << "ESMWRITER WARNING: duplicate INFO record detected, will skip: (" << info.mResponse << ") [" << std::hex << infoFormID << "]" << std::endl;
				}
				else
				{
					writer.startRecordTES4("INFO", infoFlags, infoFormID, infoEDID);
					// todo: resolve mActor->mFaction to put factionID with PCExpelled
//					info.exportTESx(mDocument, writer, 4, newType, topicEDID, CreateAddTopicList(info.mResponse));
					info.exportTESx(mDocument, writer, 4, newType, topicEDID, CreateAddTopicList(info.mResponse), prevRecordID);
					writer.endRecordTES4("INFO");
					prevRecordID = infoFormID;
				}

			}
		}

		// close child group
		if (bHasInfoGroup == true)
		{
			bHasInfoGroup = false;
			writer.endGroupTES4(formID);
		}

		// Process "Choice" Dialog Topics... each choice becomes new Topic + N, where N is choice number
		for (auto choiceNumPair = infoChoiceList.begin(); choiceNumPair != infoChoiceList.end(); choiceNumPair++)
		{
//			int choiceNum = choiceNumPair->first;
			std::string choiceTopicKey = choiceNumPair->first;
			std::string fullString = infoChoiceTopicNames[choiceTopicKey];
			std::stringstream choiceTopicStr(choiceTopicKey);
//			choiceTopicStr << topicEDID << "Choice" << choiceNum;

			if (fullString == "")
			{
				std::string errorMesg = "ERROR: choice topic is empty: " + choiceTopicStr.str() + "\n";
				OutputDebugString(errorMesg.c_str());
			}

			uint32_t choiceformID = writer.crossRefStringID(choiceTopicStr.str(), "DIAL", false, true);
			if (choiceformID == 0)
			{
				choiceformID = writer.reserveFormID(choiceformID, choiceTopicStr.str(), "DIAL");
			}
			uint32_t flags = 0;
			writer.startRecordTES4("DIAL", flags, choiceformID, choiceTopicStr.str());
			writer.startSubRecordTES4("EDID");
			writer.writeHCString(choiceTopicStr.str());
			writer.endSubRecordTES4("EDID");
			writer.startSubRecordTES4("FULL");
			writer.writeHCString(fullString);
			writer.endSubRecordTES4("FULL");
			writer.startSubRecordTES4("DATA");
			writer.writeT<uint8_t>(0);
			writer.endSubRecordTES4("DATA");
			writer.endRecordTES4("DIAL");

			writer.startGroupTES4(choiceformID, 7);
			for (auto infoChoiceItem = choiceNumPair->second.begin(); infoChoiceItem != choiceNumPair->second.end(); infoChoiceItem++)
			{
				std::string infoEDID = "info#" + choiceTopicStr.str() + infoChoiceItem->mId;
				uint32_t infoFormID = writer.crossRefStringID(infoEDID, "INFO", false, true);
				uint32_t infoFlags = 0;
				if (topic.isDeleted()) infoFlags |= 0x800; // DISABLED
				bool bSuccess;
				bSuccess = writer.startRecordTES4("INFO", infoFlags, infoFormID, infoEDID);
				if (bSuccess)
				{
					infoChoiceItem->exportTESx(mDocument, writer, 4, 0, topicEDID, CreateAddTopicList(infoChoiceItem->mResponse));
					writer.endRecordTES4("INFO");
				}
				else
				{
					std::cout << "[INFO] startRecordTES4() failed... [" << std::hex << infoFormID << "] " << infoEDID << std::endl;
				}
			}
			writer.endGroupTES4(choiceformID);
		}
	}

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		appendSpecialRecords();
		writer.endGroupTES4(sSIG);
	}

}

CSMDoc::ExportRefIdCollectionTES4Stage::ExportRefIdCollectionTES4Stage (Document& document, SavingState& state)
	: mDocument (document), mState (state)
{}
int CSMDoc::ExportRefIdCollectionTES4Stage::setup()
{
	mActiveRefCount=0;
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportRefIdCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().exportTESx (stage, mState.getWriter(), 4);
//	mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().exportTESx (stage, mState.getWriter(), 4);

	if (stage == mActiveRefCount-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportRegionDataTES4Stage::ExportRegionDataTES4Stage(Document& document, SavingState& state, bool skipMasters)
	: mDocument(document), mState(state)
{
	mSkipMasterRecords = skipMasters;
}
void CSMDoc::ExportRegionDataTES4Stage::buildRegionMaps()
{
	// create regionID to CellList
	int numCells = mDocument.getData().getCells().getSize();
	for (int cellIndex = 0; cellIndex < numCells; cellIndex++)
	{
		auto cellRecord = mDocument.getData().getCells().getNthRecord(cellIndex);
		if (!cellRecord.isModified() && !cellRecord.isDeleted())
		{
			// mark regionid for skipping?
			continue;
		}
		auto cell = cellRecord.get();
		std::string regionId = cell.mRegion;
		if (regionId != "")
		{
			// insert X
			for (int a=0; a < 2; a++)
				for (int b = 0; b < 2; b++)
				{
					int cellX = (cell.mData.mX + a) * 2;
					int cellY = (cell.mData.mY + b) * 2;
					insertX(regionId, cellX, cellY);
				}
		}
	}
}
void CSMDoc::ExportRegionDataTES4Stage::insertX(std::string regionId, int X, int Y)
{
	std::vector<std::pair<int, MinMaxPair>> &sortedList = mRegionIDToCellList[regionId];
	auto sort_item = sortedList.begin();
	while (sort_item != sortedList.end() && sort_item->first < X)
		sort_item++;
	if (sort_item == sortedList.end())
	{
		sortedList.push_back(std::make_pair(X, MinMaxPair(Y)));
		return;
	}
	else if (sort_item->first == X)
	{
		sort_item->second.updateMinMax(Y);
		return;
	}
	else if (sort_item->first > X)
	{
		// insert new item
		sortedList.insert(sort_item, std::make_pair(X, MinMaxPair(Y)));
		return;
	}

}
void CSMDoc::ExportRegionDataTES4Stage::MinMaxPair::updateMinMax(int newVal)
{
	min = (min < newVal) ? min : newVal;
	max = (max > newVal) ? max : newVal;
}
CSMDoc::ExportRegionDataTES4Stage::MinMaxPair::MinMaxPair(int newVal)
{
	min = newVal;
	max = newVal;
}
int CSMDoc::ExportRegionDataTES4Stage::setup()
{
	ESM::ESMWriter &writer = mState.getWriter();
	std::string sSIG = "REGN";

	buildRegionMaps();

	int regionListSize = mDocument.getData().getRegions().getSize();
	for (int i = 0; i < regionListSize; i++)
	{
		const CSMWorld::Record<ESM::Region>& record = mDocument.getData().getRegions().getNthRecord(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (!exportOrSkip)
		{
			std::string regionId = record.get().mId;
			if (mRegionIDToCellList.find(regionId) != mRegionIDToCellList.end())
				exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();

}
void CSMDoc::ExportRegionDataTES4Stage::traceBoundaryBack(std::vector<std::pair<int, MinMaxPair>>::iterator start_cell, std::vector<std::pair<int, MinMaxPair>>::iterator end_cell)
{
	ESM::ESMWriter &writer = mState.getWriter();

	std::vector<std::pair<int, MinMaxPair>>::iterator current_cell;
	for (current_cell = start_cell; current_cell != end_cell; current_cell--)
	{
		// insert interpolation point
		// insert interpolation point
		if (current_cell != start_cell)
		{
			auto prevCell = current_cell; prevCell++;
			if (prevCell->second.max != current_cell->second.max)
			{
				// pick highest minimum or lowest maximum
				float x = current_cell->first * 4096;
				float y = prevCell->second.max * 4096;
				if (current_cell->second.max < prevCell->second.max)
				{
					x = prevCell->first * 4096;
					y = current_cell->second.max * 4096;
				}
				writer.writeT<float>(x);
				writer.writeT<float>(y);
			}
		}
		float x = current_cell->first * 4096;
		float y = current_cell->second.max * 4096;
		writer.writeT<float>(x);
		writer.writeT<float>(y);
	}
	// process end_cell
	auto prevCell = current_cell; prevCell++;
	if (prevCell->second.max != current_cell->second.max)
	{
		// pick highest minimum or lowest maximum
		float x = current_cell->first * 4096;
		float y = prevCell->second.max * 4096;
		if (current_cell->second.max < prevCell->second.max)
		{
			x = prevCell->first * 4096;
			y = current_cell->second.max * 4096;
		}
		writer.writeT<float>(x);
		writer.writeT<float>(y);
	}
	float x = current_cell->first * 4096;
	float y = current_cell->second.max * 4096;
	writer.writeT<float>(x);
	writer.writeT<float>(y);

	writer.endSubRecordTES4("RPLD");
}
std::vector<std::pair<int, CSMDoc::ExportRegionDataTES4Stage::MinMaxPair>>::iterator CSMDoc::ExportRegionDataTES4Stage::traceBoundaryForward(std::vector<std::pair<int, MinMaxPair>>::iterator start_cell, std::vector<std::pair<int, MinMaxPair>>::iterator end_cell)
{
	ESM::ESMWriter &writer = mState.getWriter();

	std::vector<std::pair<int, MinMaxPair>>::iterator current_cell;
	for (current_cell = start_cell; current_cell != end_cell; current_cell++)
	{
		// step 1: check if starting cell to initialize regiondata
		if (current_cell == start_cell)
		{
			writer.startSubRecordTES4("RPLI");
			writer.writeT<uint32_t>(1024); // RPLI aka edge-falloff
			writer.endSubRecordTES4("RPLI");
			writer.startSubRecordTES4("RPLD");
		}
		else
		{
			auto prevCell = current_cell; prevCell--;

			// step 2: always check if this point is continuous with last
			if (current_cell->first - prevCell->first > 2)
			{
				traceBoundaryBack(prevCell, start_cell);
				return current_cell;
			}

			// interpolate from last point
			if (prevCell->second.min != current_cell->second.min)
			{
				// pick highest minimum or lowest maximum
				float x = current_cell->first * 4096;
				float y = prevCell->second.min * 4096;
				if (current_cell->second.min > prevCell->second.min)
				{
					x = prevCell->first * 4096;
					y = current_cell->second.min * 4096;
				}
				writer.writeT<float>(x);
				writer.writeT<float>(y);
			}
		}
		float x = current_cell->first * 4096;
		float y = current_cell->second.min * 4096;
		writer.writeT<float>(x);
		writer.writeT<float>(y);
	}
	if (current_cell == end_cell)
	{
		auto prevCell = current_cell; prevCell--;
		// end of list, traceback
		traceBoundaryBack(prevCell, start_cell);
	}
	return current_cell;

}
void CSMDoc::ExportRegionDataTES4Stage::perform(int stage, Messages& messages)
{
	std::string sSIG = "REGN";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int index = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Region> regionRec = mDocument.getData().getRegions().getNthRecord(index);
	auto region = regionRec.get();

//	std::string strEDID = writer.generateEDIDTES4(region.mId, 0, sSIG);
	//uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	StartModRecord(sSIG, region.mId, writer, regionRec.mState);
	regionRec.get().exportTESx(writer, 4); // initial output...

	// WNAM - Worldspace
	writer.startSubRecordTES4("WNAM");
	writer.writeT<uint32_t>(0x01380000); // World FormID
	writer.endSubRecordTES4("WNAM");

	// Region Areas - array of region areas
	// one region area: RPLI + array of RPLD(X,Y)
	std::vector<std::pair<int,MinMaxPair>> &cellList = mRegionIDToCellList[region.mId];
	if (cellList.empty())
	{
		writer.startSubRecordTES4("RPLI");
		writer.writeT<uint32_t>(1024); // RPLI aka edge-falloff
		writer.endSubRecordTES4("RPLI");
		writer.startSubRecordTES4("RPLD");
		writer.endSubRecordTES4("RPLD");
	}
	else
	{
		auto current_cell = cellList.begin();
		while (current_cell != cellList.end())
		{
			current_cell = traceBoundaryForward(current_cell, cellList.end());
		}
	}

	// Region Data Entries?
	// Objects, Map, Grass, Sound, Weather
	writer.startSubRecordTES4("RDAT");
	writer.writeT<uint32_t>(4); // Type (Map == 4)
	writer.writeT<uint8_t>(0); // Flags
	writer.writeT<uint8_t>(50); // Priority
	writer.writeT<uint8_t>(4); // Unused
	writer.writeT<uint8_t>(4); // Unused
	writer.endSubRecordTES4("RDAT");
	writer.startSubRecordTES4("RDMP");
	writer.writeHCString(region.mName);
	writer.endSubRecordTES4("RDMP");

	writer.startSubRecordTES4("RDAT");
	writer.writeT<uint32_t>(3); // Type (Weather == 3)
	writer.writeT<uint8_t>(0); // Flags
	writer.writeT<uint8_t>(50); // Priority
	writer.writeT<uint8_t>(4); // Unused
	writer.writeT<uint8_t>(4); // Unused
	writer.endSubRecordTES4("RDAT");
	writer.startSubRecordTES4("RDWT");
	region.exportWeatherListTES4(writer);
	writer.endSubRecordTES4("RDWT");

	writer.endRecordTES4(sSIG);

	if (mActiveRecords.size() > 0 && stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportAmmoCollectionTES4Stage::ExportAmmoCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportAmmoCollectionTES4Stage::setup()
{
	mActiveRefCount = mState.mAmmoFromWeaponList.size();
	return mActiveRefCount;
}
void CSMDoc::ExportAmmoCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "AMMO";
	ESM::ESMWriter& writer = mState.getWriter();

	int numRecords = mState.mAmmoFromWeaponList.size();
	// GRUP
	if (stage == 0 && numRecords > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int ammoIndex = mState.mAmmoFromWeaponList.at(stage);
	const CSMWorld::Record<ESM::Weapon> weaponRec = mDocument.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(ammoIndex);
//	std::string strEDID = writer.generateEDIDTES4(weaponRec.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, "AMMO", false, true);

	StartModRecord(sSIG, weaponRec.get().mId, writer, weaponRec.mState);
	weaponRec.get().exportAmmoTESx(writer, 4);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRefCount-1 && numRecords > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportWeaponCollectionTES4Stage::ExportWeaponCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportWeaponCollectionTES4Stage::setup()
{
	std::string sSIG = "WEAP";
	ESM::ESMWriter& writer = mState.getWriter();

	int weaponListSize= mDocument.getData().getReferenceables().getDataSet().getWeapons().getSize();
	for (int i=0; i < weaponListSize; i++)
	{
		const CSMWorld::Record<ESM::Weapon>& record = mDocument.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			// Move ammunition (arrows, bolts) to ammo collection
			if (exportOrSkip)
			{
				if ((record.get().mData.mType == ESM::Weapon::Type::Arrow) ||
					(record.get().mData.mType == ESM::Weapon::Type::Bolt))
				{
					mState.mAmmoFromWeaponList.push_back(i);
					continue;
				}
			}

			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, "WEAP");
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportWeaponCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "WEAP";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getWeapons().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Weapon> weaponRec = mDocument.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(recordIndex);
//	std::string strEDID = writer.generateEDIDTES4(weaponRec.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	bool exportOrSkip=false;
	if (mSkipMasterRecords)
	{
		exportOrSkip = weaponRec.isModified() || weaponRec.isDeleted();
/*
		exportOrSkip = weaponRec.isModified() || weaponRec.isDeleted() ||
			 (formID == 0) || ((formID & 0xFF000000) > 0x01000000);
*/
	}
	else
	{
		exportOrSkip = true;
	}

	if (exportOrSkip)
	{
/*
		std::string strEDID = writer.generateEDIDTES4(weaponRec.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID);
		uint32_t flags=0;
		if (weaponRec.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
		StartModRecord(sSIG, weaponRec.get().mId, writer, weaponRec.mState);
		weaponRec.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportFurnitureCollectionTES4Stage::ExportFurnitureCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportFurnitureCollectionTES4Stage::setup()
{
	int collectionSize = mState.mFurnitureFromActivatorList.size() + mState.mFurnitureFromStaticList.size();

	if (collectionSize > 0)
		mActiveRefCount=1;
	else
		mActiveRefCount=0;

	return mActiveRefCount;
}
void CSMDoc::ExportFurnitureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::ostringstream debugstream;
	std::string sSIG = "FURN";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	for (auto furnIndex = mState.mFurnitureFromActivatorList.begin(); furnIndex != mState.mFurnitureFromActivatorList.end(); furnIndex++)
	{
		const CSMWorld::Record<ESM::Activator> activatorRec = mDocument.getData().getReferenceables().getDataSet().getActivators().mContainer.at(*furnIndex);
//		std::string strEDID = writer.generateEDIDTES4(activatorRec.get().mId);
//		uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

		StartModRecord(sSIG, activatorRec.get().mId, writer, activatorRec.mState);
		activatorRec.get().exportTESx(mDocument, writer, 4);
		// MNAM
		writer.startSubRecordTES4("MNAM");
		writer.writeT<uint32_t>(0);
		writer.endSubRecordTES4("MNAM");
		writer.endRecordTES4(sSIG);
	}

	for (auto furnIndex = mState.mFurnitureFromStaticList.begin(); furnIndex != mState.mFurnitureFromStaticList.end(); furnIndex++)
	{
		const CSMWorld::Record<ESM::Static> staticRec = mDocument.getData().getReferenceables().getDataSet().getStatics().mContainer.at(*furnIndex);
//		std::string strEDID = writer.generateEDIDTES4(staticRec.get().mId);
//		uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

		StartModRecord(sSIG, staticRec.get().mId, writer, staticRec.mState);
		staticRec.get().exportTESx(writer, 4);
		// MNAM
		writer.startSubRecordTES4("MNAM");
		writer.writeT<uint32_t>(0);
		writer.endSubRecordTES4("MNAM");
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportSoulgemCollectionTES4Stage::ExportSoulgemCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportSoulgemCollectionTES4Stage::setup()
{
	mActiveRefCount = 1;
	return mActiveRefCount;
}
void CSMDoc::ExportSoulgemCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "SLGM";
	ESM::ESMWriter& writer = mState.getWriter();

	int numReferences = mState.mSoulgemFromMiscList.size();
	// GRUP
	if (stage == 0 && numReferences > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	//	mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	for (auto soulgemIndex = mState.mSoulgemFromMiscList.begin(); soulgemIndex != mState.mSoulgemFromMiscList.end(); soulgemIndex++)
	{
		const CSMWorld::Record<ESM::Miscellaneous> soulgemRec = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(*soulgemIndex);

/*
		std::string strEDID = writer.generateEDIDTES4(soulgemRec.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID);
		uint32_t flags=0;
		if (soulgemRec.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
		StartModRecord(sSIG, soulgemRec.get().mId, writer, soulgemRec.mState);
		soulgemRec.get().exportTESx(writer, 4);
		writer.startSubRecordTES4("SOUL");
		writer.writeT<uint8_t>(0); // contained soul
		writer.endSubRecordTES4("SOUL");
		writer.startSubRecordTES4("SLCP");
		writer.writeT<uint8_t>(0); // maximum capacity
		writer.endSubRecordTES4("SLCP");
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1 && numReferences > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportKeyCollectionTES4Stage::ExportKeyCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportKeyCollectionTES4Stage::setup()
{
	mActiveRefCount = 1;
	return mActiveRefCount;
}
void CSMDoc::ExportKeyCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "KEYM";
	ESM::ESMWriter& writer = mState.getWriter();

	int numReferences = mState.mKeyFromMiscList.size();
	// GRUP
	if (stage == 0 && numReferences > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	for (auto keyIndex = mState.mKeyFromMiscList.begin(); keyIndex != mState.mKeyFromMiscList.end(); keyIndex++)
	{
		const CSMWorld::Record<ESM::Miscellaneous> keyRecord = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(*keyIndex);

/*
		std::string strEDID = writer.generateEDIDTES4(keyRecord.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID);
		uint32_t flags=0;
		if (keyRecord.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
		StartModRecord(sSIG, keyRecord.get().mId, writer, keyRecord.mState);
		keyRecord.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1 && numReferences > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportMiscCollectionTES4Stage::ExportMiscCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportMiscCollectionTES4Stage::setup()
{
	std::string sSIG="MISC";
	ESM::ESMWriter& writer = mState.getWriter();

	int miscListSize = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().getSize();

	for (int i=0; i < miscListSize; i++)
	{
		const CSMWorld::Record<ESM::Miscellaneous>& record = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			std::string searchString;
			// put Keys in separate collection
			bool bIsKey = record.get().mData.mIsKey;
			if (bIsKey == false)
			{
				searchString = Misc::StringUtils::lowerCase(record.get().mId);
				if (searchString.find("key", 0) != searchString.npos)
				{
					bIsKey = true;
				}
				else
				{
					searchString = Misc::StringUtils::lowerCase(record.get().mName);
					if (searchString.find("key", 0) != searchString.npos)
						bIsKey = true;
				}
			}
			if (bIsKey)
			{
				// add to KeyCollection, and skip
				mState.mKeyFromMiscList.push_back(i);
				continue;
			}
			// put Soulgems in separate collection
			bool bIsSoulGem = false;
			searchString = Misc::StringUtils::lowerCase(record.get().mId);
			if (searchString.find("soulgem", 0) != searchString.npos)
			{
				bIsSoulGem = true;
			}
			else
			{
				searchString = Misc::StringUtils::lowerCase(record.get().mName);
				if (searchString.find("soulgem", 0) != searchString.npos)
					bIsSoulGem = true;
			}
			if (bIsSoulGem)
			{
				// add to KeyCollection, and skip
				mState.mSoulgemFromMiscList.push_back(i);
				continue;
			}

			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, "MISC");
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}
	return mActiveRecords.size();
}

void CSMDoc::ExportMiscCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "MISC";
	std::string searchString;
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Miscellaneous> miscRecord = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(recordIndex);

//	std::string strEDID = writer.generateEDIDTES4(miscRecord.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	bool exportOrSkip=false;
	if (mSkipMasterRecords)
	{
		exportOrSkip = miscRecord.isModified() || miscRecord.isDeleted();
/*
		exportOrSkip = miscRecord.isModified() || miscRecord.isDeleted() || 
			(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
*/
	}
	else
	{
		exportOrSkip = true;
	}


	if (exportOrSkip)
	{
/*
		std::string strEDID = writer.generateEDIDTES4(miscRecord.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID);
		uint32_t flags=0;
		if (miscRecord.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
		StartModRecord(sSIG, miscRecord.get().mId, writer, miscRecord.mState);
		miscRecord.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportLightCollectionTES4Stage::ExportLightCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportLightCollectionTES4Stage::setup()
{
	std::string sSIG="LIGH";
	ESM::ESMWriter& writer = mState.getWriter();

	int lightsListSize = mDocument.getData().getReferenceables().getDataSet().getLights().getSize();

	for (int i=0; i < lightsListSize; i++)
	{
		const CSMWorld::Record<ESM::Light>& record =  mDocument.getData().getReferenceables().getDataSet().getLights().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, "LIGH");
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportLightCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "LIGH";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getLights().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportLeveledItemCollectionTES4Stage::ExportLeveledItemCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportLeveledItemCollectionTES4Stage::setup()
{
	std::string sSIG="LVLI";
	ESM::ESMWriter& writer = mState.getWriter();

	int LVLI_ListSize = mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().getSize();

	for (int i=0; i < LVLI_ListSize; i++)
	{
		const CSMWorld::Record<ESM::ItemLevList>& record = mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, "LVLI");
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportLeveledItemCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "LVLI";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportIngredientCollectionTES4Stage::ExportIngredientCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportIngredientCollectionTES4Stage::setup()
{
	std::string sSIG="INGR";
	ESM::ESMWriter& writer = mState.getWriter();
	int ingredListSize = mDocument.getData().getReferenceables().getDataSet().getIngredients().getSize();

	for (int i = 0; i < ingredListSize; i++)
	{
		const CSMWorld::Record<ESM::Ingredient>& record = mDocument.getData().getReferenceables().getDataSet().getIngredients().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, "INGR");
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportIngredientCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "INGR";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
		std::cout << "Ingredients:";
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getIngredients().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);
	std::cout << "..." << stage;


	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		writer.endGroupTES4(sSIG);
		std::cout << "...ingredients complete." << std::endl;
	}
}

CSMDoc::ExportFloraCollectionTES4Stage::ExportFloraCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportFloraCollectionTES4Stage::setup()
{
// Flora record list is not built until perform phase, so we can only return 1
	mActiveRefCount = mState.mFloraFromContList.size();
	return mActiveRefCount;
}
void CSMDoc::ExportFloraCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "FLOR";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRefCount > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mState.mFloraFromContList.at(stage);
	const CSMWorld::Record<ESM::Container> containerRecord = mDocument.getData().getReferenceables().getDataSet().getContainers().mContainer.at(recordIndex);
	
	std::string ingredientEDID="";
	/** TODO: resolve inventory item to ingredient */
	for (auto inventoryItem = containerRecord.get().mInventory.mList.begin();
		inventoryItem != containerRecord.get().mInventory.mList.end();
		inventoryItem++)
	{
		// get itemName, resolve itemType
		std::string itemName = inventoryItem->mItem.toString();
		std::string itemEDID = writer.generateEDIDTES4(itemName, 0, "");
		auto typeResult = writer.mStringTypeMap.find(Misc::StringUtils::lowerCase(itemEDID));
		if (typeResult == writer.mStringTypeMap.end())
			continue;

		if (Misc::StringUtils::lowerCase(typeResult->second) == "ingr")
		{
			ingredientEDID = writer.generateEDIDTES4(itemName, 0, "INGR");
			break;
		}
		else if (Misc::StringUtils::lowerCase(typeResult->second) == "lvli")
		{
			// resolve lvli to ingredient
			auto searchResult = mDocument.getData().getReferenceables().getDataSet().searchId(itemName);
			const CSMWorld::Record<ESM::ItemLevList>& recordLVLI = mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().mContainer.at(searchResult.first);
			for (auto itemLVLI = recordLVLI.get().mList.begin(); 
				itemLVLI != recordLVLI.get().mList.end();
				itemLVLI++)
			{
				// check lvli items for an ingredient
				std::string lvli_itemname = itemLVLI->mId;
				std::string lvli_itemEDID = writer.generateEDIDTES4(lvli_itemname, 0, "");
				// check type
				auto lvli_typeResult = writer.mStringTypeMap.find(Misc::StringUtils::lowerCase(lvli_itemEDID));
				if (lvli_typeResult == writer.mStringTypeMap.end())
				{
					continue;
				}

				if (Misc::StringUtils::lowerCase(lvli_typeResult->second) == "ingr")
				{
					ingredientEDID = lvli_itemEDID;
					break;
				}

			}
		}

	}

	StartModRecord(sSIG, containerRecord.get().mId, writer, containerRecord.mState);
	containerRecord.get().exportAsFlora(writer, ingredientEDID);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRefCount-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportContainerCollectionTES4Stage::ExportContainerCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportContainerCollectionTES4Stage::setup()
{
	std::string sSIG = "CONT";
	ESM::ESMWriter& writer = mState.getWriter();

	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getContainers().getSize();
	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Container> containerRecord = mDocument.getData().getReferenceables().getDataSet().getContainers().mContainer.at(i);
		std::string strEDID = writer.generateEDIDTES4(containerRecord.get().mId, 0, sSIG);
//		uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

		bool isFlora = false;
		bool exportOrSkip = false;

		if (mSkipMasterRecords)
		{
			exportOrSkip = containerRecord.isModified() || containerRecord.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			// get matched Record Type
			auto recordType = writer.mStringTypeMap.find(Misc::StringUtils::lowerCase(strEDID));
			if (recordType != writer.mStringTypeMap.end() &&
				Misc::StringUtils::lowerCase(recordType->second) == "flor")
			{
				isFlora = true;
			}

			if (isFlora == true)
			{
				mState.mFloraFromContList.push_back(i);
				exportOrSkip = false;
			}
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			// reserveFormID
		}

	}

	return mActiveRecords.size();
}
void CSMDoc::ExportContainerCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "CONT";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Container> containerRecord = mDocument.getData().getReferenceables().getDataSet().getContainers().mContainer.at(recordIndex);
//	std::string strEDID = writer.generateEDIDTES4(containerRecord.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	StartModRecord(sSIG, containerRecord.get().mId, writer, containerRecord.mState);
	containerRecord.get().exportTESx(writer, 4);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportClothingCollectionTES4Stage::ExportClothingCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportClothingCollectionTES4Stage::setup()
{
	std::string sSIG = "CLOT";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getClothing().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		bool exportOrSkip = false;
		const CSMWorld::Record<ESM::Clothing> record = mDocument.getData().getReferenceables().getDataSet().getClothing().mContainer.at(i);
//		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
//		uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

		if (mSkipMasterRecords == true)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		// filter out left glove
		if (exportOrSkip)
		{
			if (record.get().mData.mType == ESM::Clothing::Type::LGlove)
				exportOrSkip = false;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}

	}

	return mActiveRecords.size();

//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getClothing().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportClothingCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "CLOT";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Clothing> clothingRecord = mDocument.getData().getReferenceables().getDataSet().getClothing().mContainer.at(recordIndex);
//	std::string strEDID = writer.generateEDIDTES4(clothingRecord.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	StartModRecord(sSIG, clothingRecord.get().mId, writer, clothingRecord.mState);
	clothingRecord.get().exportTESx(mDocument, writer, 4);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportClimateCollectionTES4Stage::ExportClimateCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportClimateCollectionTES4Stage::setup()
{
	mActiveRefCount = mDocument.getData().getRegions().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportClimateCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "CLMT";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	ESM::Region region = mDocument.getData().getRegions().getNthRecord(stage).get();
	std::string strEDID = writer.generateEDIDTES4(region.mId, 0, "CLMT") + "Clmt";
	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
	if ((formID & 0xFF000000) > 0x01000000 || formID == 0)
	{
		writer.startRecordTES4("CLMT", 0, formID, strEDID);
		region.exportClimateTESx(writer, 4);
		writer.endRecordTES4("CLMT");
	}

	if (stage == mActiveRefCount-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportBookCollectionTES4Stage::ExportBookCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportBookCollectionTES4Stage::setup()
{
	std::string sSIG = "BOOK";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getBooks().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Book>& record = mDocument.getData().getReferenceables().getDataSet().getBooks().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();

//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getBooks().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportBookCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "BOOK";

	// GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getBooks().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportArmorCollectionTES4Stage::ExportArmorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportArmorCollectionTES4Stage::setup()
{
	std::string sSIG = "ARMO";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getArmors().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		bool exportOrSkip = false;
		const CSMWorld::Record<ESM::Armor> record = mDocument.getData().getReferenceables().getDataSet().getArmors().mContainer.at(i);
//		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
//		uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

		if (mSkipMasterRecords == true)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			switch (record.get().mData.mType)
			{
			case ESM::Armor::Type::Helmet:
			case ESM::Armor::Type::Cuirass:
			case ESM::Armor::Type::Greaves:
			case ESM::Armor::Type::Boots:
			case ESM::Armor::Type::Shield:
			case ESM::Armor::Type::RGauntlet:
				exportOrSkip = true;
				break;
			default:
				exportOrSkip = false;
			}
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}

	}

	return mActiveRecords.size();

//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getArmors().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportArmorCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "ARMO";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Armor> record = mDocument.getData().getReferenceables().getDataSet().getArmors().mContainer.at(recordIndex);
//	std::string strEDID = writer.generateEDIDTES4(record.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	StartModRecord(sSIG, record.get().mId, writer, record.mState);
	record.get().exportTESx(mDocument, writer, 4);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportApparatusCollectionTES4Stage::ExportApparatusCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportApparatusCollectionTES4Stage::setup()
{
	std::string sSIG="APPA";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getApparati().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Apparatus>& record = mDocument.getData().getReferenceables().getDataSet().getApparati().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportApparatusCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "APPA";

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getApparati().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportPotionCollectionTES4Stage::ExportPotionCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportPotionCollectionTES4Stage::setup()
{
	std::string sSIG="ALCH";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getPotions().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Potion>& record = mDocument.getData().getReferenceables().getDataSet().getPotions().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportPotionCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "ALCH";

	// GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getPotions().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportActivatorCollectionTES4Stage::ExportActivatorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportActivatorCollectionTES4Stage::setup()
{
	std::string sSIG = "ACTI";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getActivators().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Activator>& record = mDocument.getData().getReferenceables().getDataSet().getActivators().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			// if mName contains Bed, Chair, Bench, Hammock then add to furnList and skip
			std::string tempStr;
			if (record.get().mName.size() > 0)
				tempStr = Misc::StringUtils::lowerCase(record.get().mName);
			else
				tempStr = Misc::StringUtils::lowerCase(record.get().mId);

			if ((tempStr.find("bed", 0) != tempStr.npos) ||
				(tempStr.find("chair", 0) != tempStr.npos) ||
				(tempStr.find("bench", 0) != tempStr.npos) ||
				(tempStr.find("hammock", 0) != tempStr.npos) ||
				(tempStr.find("stool", 0) != tempStr.npos)
				)
			{
				exportOrSkip = false;
				mState.mFurnitureFromActivatorList.push_back(i);
			}
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();

//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getActivators().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportActivatorCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::ostringstream debugstream;
	std::string sSIG = "ACTI";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Activator> record = mDocument.getData().getReferenceables().getDataSet().getActivators().mContainer.at(recordIndex);
//	std::string strEDID = writer.generateEDIDTES4(record.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

// pre-load NIF and calc bounds
	std::string nifInputName = "meshes/" + Misc::ResourceHelpers::correctActorModelPath(record.get().mModel, mDocument.getVFS());
    mDocument.getVFS()->normalizeFilename(nifInputName);
	float modelBounds = 0.0f;
	try
	{
		Files::IStreamPtr fileStream = NULL;
		fileStream = mDocument.getVFS()->get(nifInputName);
		// read stream into NIF parser...
		Nif::NIFFile nifFile(fileStream, nifInputName);
		modelBounds = nifFile.mModelBounds;
	}
	catch (std::runtime_error e)
	{
		std::cout << "ExportActivatorCollection: (" << nifInputName << ") " << std::string(e.what()) << "\n";
	}
	int vwdMode = VWD_MODE_NORMAL_ONLY;
	if (writer.mConversionOptions.find("#vwd") != std::string::npos)
		vwdMode = VWD_MODE_NORMAL_AND_LOD; // normals + vwd
	if (writer.mConversionOptions.find("#vwdonly") != std::string::npos)
	{
		vwdMode = VWD_MODE_LOD_ONLY; // no normal nifs
		if (writer.mConversionOptions.find("#vwdonly++") != std::string::npos)
			vwdMode = VWD_MODE_LOD_AND_LARGE_NORMAL; // vwds + large normals
	}
	float vwdThreshold = VWD_QUAL_MEDIUM;
	if (writer.mConversionOptions.find("#vwdfast") != std::string::npos)
	{
		vwdThreshold = VWD_QUAL_LOW;
	}
	if (writer.mConversionOptions.find("#vwdhd") != std::string::npos)
	{
		vwdThreshold = VWD_QUAL_HIGH;
	}
	if (writer.mConversionOptions.find("#vwdultra") != std::string::npos)
	{
		vwdThreshold = VWD_QUAL_ULTRA;
	}

	bool bExport = true;
	if (vwdMode == VWD_MODE_LOD_ONLY || vwdMode == VWD_MODE_LOD_AND_LARGE_NORMAL)
	{
		if (modelBounds < vwdThreshold)
			bExport = false;
	}

	if (bExport)
	{
		StartModRecord(sSIG, record.get().mId, writer, record.mState);
		record.get().exportTESx(mDocument, writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportLeveledCreatureCollectionTES4Stage::ExportLeveledCreatureCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportLeveledCreatureCollectionTES4Stage::setup()
{
	std::string sSIG="LVLC";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().getSize();

	for (int i=0; i < collectionSize; i++)
	{
//		std::cout << "export: LVLC[" << i << "] " << mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i).get().mId << " = " << formID << std::endl;
		const CSMWorld::Record<ESM::CreatureLevList>& record = mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = record.isModified() || record.isDeleted();
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportLeveledCreatureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// LVLC GRUP
	if (stage == 0 && mActiveRecords.size() > 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("LVLC", 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1 && mActiveRecords.size() > 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("LVLC");
	}
}

CSMDoc::ExportCreatureCollectionTES4Stage::ExportCreatureCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportCreatureCollectionTES4Stage::setup()
{
	std::string sSIG = "CREA";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getCreatures().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Creature>& record = mDocument.getData().getReferenceables().getDataSet().getCreatures().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatures().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportCreatureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "CREA";
	ESM::ESMWriter& writer = mState.getWriter();

	// CREA GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
//	mDocument.getData().getReferenceables().getDataSet().getCreatures().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);
	const CSMWorld::Record<ESM::Creature>& record = mDocument.getData().getReferenceables().getDataSet().getCreatures().mContainer.at(recordIndex);

	StartModRecord(sSIG, record.get().mId, writer, record.mState);
	record.get().exportTESx(&mDocument, mState.getWriter(), 4);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportDoorCollectionTES4Stage::ExportDoorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportDoorCollectionTES4Stage::setup()
{
	std::string sSIG = "DOOR";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getDoors().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Door>& record = mDocument.getData().getReferenceables().getDataSet().getDoors().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getDoors().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportDoorCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "DOOR";
	ESM::ESMWriter& writer = mState.getWriter();

	// DOOR GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
//	mDocument.getData().getReferenceables().getDataSet().getDoors().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);
	const CSMWorld::Record<ESM::Door>& record = mDocument.getData().getReferenceables().getDataSet().getDoors().mContainer.at(recordIndex);

	StartModRecord(sSIG, record.get().mId, writer, record.mState);
	record.get().exportTESx(mDocument, writer, 4);
	writer.endRecordTES4(sSIG);

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportSTATCollectionTES4Stage::ExportSTATCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportSTATCollectionTES4Stage::setup()
{
	std::string sSIG = "STAT";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getStatics().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::Static>& record = mDocument.getData().getReferenceables().getDataSet().getStatics().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			// if mName contains Bed, Chair, Bench, Hammock then add to furnList and skip
			std::string tempStr;
			tempStr = Misc::StringUtils::lowerCase(record.get().mId);

			if ((tempStr.find("bed", 0) != tempStr.npos) ||
				(tempStr.find("chair", 0) != tempStr.npos) ||
				(tempStr.find("bench", 0) != tempStr.npos) ||
				(tempStr.find("hammock", 0) != tempStr.npos) ||
				(tempStr.find("stool", 0) != tempStr.npos)
				)
			{
				exportOrSkip = false;
				mState.mFurnitureFromStaticList.push_back(i);
			}
		}

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getStatics().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportSTATCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::ostringstream debugstream;
	std::string sSIG = "STAT";
	ESM::ESMWriter& writer = mState.getWriter();

	// STAT GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<ESM::Static> record = mDocument.getData().getReferenceables().getDataSet().getStatics().mContainer.at(recordIndex);
//	std::string strEDID = writer.generateEDIDTES4(record.get().mId);
//	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	// pre-load NIF and calc bounds
	std::string nifInputName = "meshes/" + Misc::ResourceHelpers::correctActorModelPath(record.get().mModel, mDocument.getVFS());
    mDocument.getVFS()->normalizeFilename(nifInputName);
	float modelBounds = 0.0f;
	try
	{
		Files::IStreamPtr fileStream = NULL;
		fileStream = mDocument.getVFS()->get(nifInputName);
		// read stream into NIF parser...
		Nif::NIFFile nifFile(fileStream, nifInputName);
		modelBounds = nifFile.mModelBounds;
	}
	catch (std::runtime_error e)
	{
        std::string errString(e.what());
		std::cout << "ExportSTATCollection: Error: (" << nifInputName << ") " << errString << "\n";
	}
	int vwdMode = VWD_MODE_NORMAL_ONLY;
	if (writer.mConversionOptions.find("#vwd") != std::string::npos)
		vwdMode = VWD_MODE_NORMAL_AND_LOD; // normals + vwd
	if (writer.mConversionOptions.find("#vwdonly") != std::string::npos)
	{
		vwdMode = VWD_MODE_LOD_ONLY; // no normal nifs
		if (writer.mConversionOptions.find("#vwdonly++") != std::string::npos)
			vwdMode = VWD_MODE_LOD_AND_LARGE_NORMAL; // vwds + large normals
	}
	float vwdThreshold = VWD_QUAL_MEDIUM;
	if (writer.mConversionOptions.find("#vwdfast") != std::string::npos)
	{
		vwdThreshold = VWD_QUAL_LOW;
	}
	if (writer.mConversionOptions.find("#vwdhd") != std::string::npos)
	{
		vwdThreshold = VWD_QUAL_HIGH;
	}
	if (writer.mConversionOptions.find("#vwdultra") != std::string::npos)
	{
		vwdThreshold = VWD_QUAL_ULTRA;
	}

	bool bExport = true;
	if (vwdMode == VWD_MODE_LOD_ONLY || vwdMode == VWD_MODE_LOD_AND_LARGE_NORMAL)
	{
		if (modelBounds < vwdThreshold)
			bExport = false;
	}

	if (bExport)
	{
		StartModRecord(sSIG, record.get().mId, writer, record.mState);
		record.get().exportTESx(mDocument, writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportNPCCollectionTES4Stage::ExportNPCCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportNPCCollectionTES4Stage::setup()
{
	std::string sSIG = "NPC_";
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getReferenceables().getDataSet().getNPCs().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<ESM::NPC>& record = mDocument.getData().getReferenceables().getDataSet().getNPCs().mContainer.at(i);

		bool exportOrSkip = false;
		if (mSkipMasterRecords)
			exportOrSkip = record.isModified() || record.isDeleted();
		else
			exportOrSkip = true;

		if (exportOrSkip)
		{
			mActiveRecords.push_back(i);
			std::string strEDID = writer.generateEDIDTES4(record.get().mId, 0, sSIG);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID, sSIG);
			}
		}
	}

	return mActiveRecords.size();
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getNPCs().getSize();
//	return mActiveRefCount;
}
void CSMDoc::ExportNPCCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "NPC_";
	ESM::ESMWriter& writer = mState.getWriter();

	// CREA GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	writer.CompressNextRecord();
	mDocument.getData().getReferenceables().getDataSet().getNPCs().exportTESx (recordIndex, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportReferenceCollectionTES4Stage::ExportReferenceCollectionTES4Stage (Document& document,
	SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportReferenceCollectionTES4Stage::setup()
{
	mState.getSubRecords().clear();

	int size = mDocument.getData().getReferences().getSize();

	// divide reference collection into batches of 100 references per stage
	int numstages = size/100;

	if (size%100) ++numstages;

	mNumStages = numstages;

	return numstages;
}

void CSMDoc::ExportReferenceCollectionTES4Stage::perform (int stage, Messages& messages)
{
	int size = mDocument.getData().getReferences().getSize();
	std::ostringstream debugstream;
	ESM::ESMWriter& writer = mState.getWriter();

	if (stage == 0)
	{
		debugstream << "Creating Reference Lists: (total refs to process=" << size << ")... " << std::endl;
		std::cout << debugstream.str();
		OutputDebugString(debugstream.str().c_str());
	}

	// process a batch of 100 references in each stage
	for (int i=stage*100; i<stage*100+100 && i<size; ++i)
	{
		std::string sSIG = "REFR";
		std::string baseRecordID = "";
		bool persistentRef = false;
		bool scriptedRef = false;

		// get record then baseObj ID
		const CSMWorld::Record<CSMWorld::CellRef>& record = mDocument.getData().getReferences().getRecord (i);
		baseRecordID = record.get().mRefID;
		CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(baseRecordID);

		// reference record ID
		std::string strEDID = record.get().mId;
		std::string tempStr = Misc::StringUtils::lowerCase(baseRecordID);

		// check BaseObjToPersistentRefList
		
		if (writer.mBaseObjToScriptedREFList.find(tempStr) != writer.mBaseObjToScriptedREFList.end() )
		{
			persistentRef = true;
			scriptedRef = true;
		}

		if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Npc)
		{
			persistentRef = true;
			sSIG = "ACHR";
		}
		else if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature)
		{
			persistentRef = true;
			sSIG = "ACRE";
		}
		else if ((baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Door) && (record.get().mTeleport == true))
		{
			persistentRef = true;
		}
		else if (tempStr.find("marker") != std::string::npos)
		{
			persistentRef = true;
		}

		if ( record.isModified() || record.isDeleted() )
		{
			// reserve EDID + formID
			uint32_t formID = writer.crossRefStringID(record.get().mId, sSIG, false, true);
			if (formID == 0)
			{
				formID = writer.reserveFormID(formID, strEDID, sSIG);
			}

			std::string cellId = ( record.get().mOriginalCell.empty() ? record.get().mCell : record.get().mOriginalCell );

			// determine if cell is interior or exterior
			bool interior = cellId.substr (0, 1)!="#";

			// if exterior, then recalculate to an Oblivion CellId
			if (!interior)
			{
				// Oblivion algorithm: cellX = coordX / 4096; cellY = coordY / 4096
				std::ostringstream tempStream;

				int cellX = record.get().mPos.pos[0] / 4096;
				if (record.get().mPos.pos[0] < 0) cellX--;

				int cellY = record.get().mPos.pos[1] / 4096;
				if (record.get().mPos.pos[1] < 0) cellY--;

				tempStream << "#" << cellX << " " << cellY;
				cellId = tempStream.str();
			}

	//		debugstream.str(""). debugstream.clear();
	//		debugstream << "(" << i << ")[" << cellId << "] ";
	//		OutputDebugString(debugstream.str().c_str());

			std::deque<int>& cellRefIndexList =
				mState.getSubRecords()[Misc::StringUtils::lowerCase (cellId)];

			// Collect and build DoorRef database: <position-cell, refIndex=i, formID >
			// if NPC or teleport door in exterior worldspace
			// then add to a global worldspace dummy cell
			// and skip to the next loop iteration
	
			if (persistentRef == true)
			{
				debugstream.str(""); debugstream.clear();
				debugstream << "Collecting Persistent reference: BaseID=[" << record.get().mRefID << "] cellID=[" << cellId << "]" << std::endl;
//				OutputDebugString(debugstream.str().c_str());
				std::string tempCellReference = cellId;
				if (!interior)
					tempCellReference = "worldspace-dummycell";
				std::deque<int>& persistentRefListOfCurrentCell = mState.mPersistentRefMap[Misc::StringUtils::lowerCase (tempCellReference)];

				persistentRefListOfCurrentCell.push_back(i);
				continue;
			}

			cellRefIndexList.push_back (i);
		}
		else // record is NOT modified or deleted
		{
			//**********TODO:  This code block is useless if create or link to the correc formID of the base reference record ***/
			// record is unmodified, so if it is important (teleport door) then keep track of it, otherwise just skip ahead
			CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(record.get().mRefID);
			if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Door && record.get().mTeleport == true)
			{
				// place it into correct cell list
				std::string cellId = (record.get().mOriginalCell.empty() ? record.get().mCell : record.get().mOriginalCell);
				// determine if cell is interior or exterior
				bool interior = cellId.substr (0, 1)!="#";
				// if exterior, then recalculate to an Oblivion CellId
				if (!interior)
				{
					// Oblivion algorithm: cellX = coordX / 4096; cellY = coordY / 4096
					std::ostringstream tempStream;

					int cellX = record.get().mPos.pos[0] / 4096;
					if (record.get().mPos.pos[0] < 0) cellX--;

					int cellY = record.get().mPos.pos[1] / 4096;
					if (record.get().mPos.pos[1] < 0) cellY--;

					tempStream << "#" << cellX << " " << cellY;
					cellId = tempStream.str();
				}

				std::deque<int>& cellRefIndexList = mState.mBaseGameDoorList[Misc::StringUtils::lowerCase (cellId)];
				cellRefIndexList.push_back(i);
			}
			else
			{
				continue;
			}
		} // if record modified or deleted
	}

	if (stage == mNumStages-1)
	{
		debugstream << "Reference Lists creation complete." << std::endl;
		std::cout << debugstream.str();
		OutputDebugString(debugstream.str().c_str());
	}
}


CSMDoc::ExportInteriorCellCollectionTES4Stage::ExportInteriorCellCollectionTES4Stage (Document& document,
	SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportInteriorCellCollectionTES4Stage::setup()
{
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getCells().getSize();

	// first, create dummycells derived from exterior cell names
	for (auto cellName = mState.mDummyCellNames.begin(); cellName != mState.mDummyCellNames.end(); cellName++)
	{
		if (cellName->find("#") != std::string::npos)
		{
			std::cout << "ERROR!!! Exterior Cell Record Name Address found in mDummyCellNames: " << *cellName << "\n";
			continue;
		}
		std::string cellEDID = writer.generateEDIDTES4(*cellName, 0, "CELL");
		uint32_t formID = writer.crossRefStringID(cellEDID, "CELL", false, true);
		if (formID == 0)
		{
			formID = writer.reserveFormID(formID, cellEDID, "CELL");
		}
		// skip dummy cells already registered with a different mod
		if (formID < writer.mESMoffset)
			continue;
		uint32_t localFormID = (formID & 0x00FFFFFF);
		int block = localFormID % 100;
		int subblock = localFormID % 10;
		block -= subblock;
		block /= 10;
		if (block < 0 || block >= 10 || subblock < 0 || subblock >= 10)
			throw std::runtime_error("export error: block/subblock calculation produced out of bounds index");
		Blocks[block][subblock].push_back(std::pair<uint32_t, CSMWorld::Record<CSMWorld::Cell>*>(formID, nullptr));
	}

	// look through all cells and add interior cells to appropriate subblock
	for (int i=0; i < collectionSize; i++)
	{
		CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr = &mDocument.getData().getCells().getNthRecord(i);
		bool interior = cellRecordPtr->get().mId.substr (0, 1)!="#";

		if (interior == true)
		{	
			// add to one of 100 subblocks
			std::string cellEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId, 1);
			if (cellEDID.find("#") != std::string::npos)
			{
				std::cout << "ERROR!!! Exterior Cell Record Name Address is being added to interior cell export list: " << cellEDID << "\n";
				continue;
			}
			uint32_t formID = writer.crossRefStringID(cellEDID, "CELL", false, true);
			if (formID == 0)
			{
				formID = writer.reserveFormID(formID, cellEDID, "CELL");
			}
			uint32_t localFormID = (formID & 0x00FFFFFF);
			int block = localFormID % 100;
			int subblock = localFormID % 10;
			block -= subblock;
			block /= 10;
			if (block < 0 || block >= 10 || subblock < 0 || subblock >= 10)
				throw std::runtime_error ("export error: block/subblock calculation produced out of bounds index");
			Blocks[block][subblock].push_back(std::pair<uint32_t, CSMWorld::Record<CSMWorld::Cell>*>(formID, cellRecordPtr) );
		}
	}

	// initialize blockInitialized[][] to track BLOCK initialization in perform()
	for (int i=0; i < 10; i++)
		for (int j=0; j < 10; j++)
			blockInitialized[i][j]=false;

	// count number of cells in all blocks to return
	mNumCells = 0;
	for (int i=0; i < 10; i++)
		for (int j=0; j < 10; j++)
			mNumCells += Blocks[i][j].size();

	return mNumCells;
}

void CSMDoc::ExportInteriorCellCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
//	CSMWorld::Record<CSMWorld::Cell>& cell = mDocument.getData().getCells().getRecord (stage);
	CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr=0;

	int block, subblock;
	uint32_t cellFormID=0;

	// iterate through Blocks[][] starting with 0,0 and sequentially remove each Cell until all are gone
	for (block=0; block < 10; block++)
	{
		for (subblock=0; subblock < 10; subblock++)
		{
			if (Blocks[block][subblock].size() > 0)
			{
                // retrieve formID from RecordPtr map (previously generated in setup() method)
				cellFormID = Blocks[block][subblock].back().first;
				cellRecordPtr = Blocks[block][subblock].back().second;
				Blocks[block][subblock].pop_back();
				break;
			}
		}
		if (cellFormID != 0)
			break;
	}
	if (cellFormID == 0)
	{
		// throw exception
		throw std::runtime_error ("export error: cellFormID uninitialized");
	}

	// check if this is the first record, to write first GRUP record
	if (stage == 0)
	{
		std::string sSIG = "CELL";
//		for (int i=0; i<4; ++i)
//			sSIG += reinterpret_cast<const char *>(&cellRecordPtr->get().sRecordId)[i];
		writer.startGroupTES4(sSIG, 0); // Top GROUP
		// initialize first block
		writer.startGroupTES4(block, 2);
		// initialize first subblock
		writer.startGroupTES4(subblock, 3);
		// document creation of first subblock
		blockInitialized[block][subblock] = true;
	}

	// check to see if we need to write the next GRUP record
	if (blockInitialized[block][subblock] == false)
	{
		blockInitialized[block][subblock] = true;
		// close previous subblock
		writer.endGroupTES4(0);
		// if subblock == 0, then close prior block as well
		if (subblock == 0)
		{
			writer.endGroupTES4(0);
			// start the next block if prior block was closed
			writer.startGroupTES4(block, 2);
		}
		// start new subblock
		writer.startGroupTES4(subblock, 3);
	}
	
//============================================================================================================
// EXPORT AN INTERIOR CELL + CELL'S REFERENCES
//============================================================================================================

	if (cellRecordPtr == nullptr)
	{
		// create dummy interior cell
		std::string dummyName = writer.crossRefFormID(cellFormID);
		writer.startRecordTES4("CELL", 0, cellFormID, dummyName);
		writer.startSubRecordTES4("EDID");
		writer.writeHCString(dummyName);
		writer.endSubRecordTES4("EDID");
		char dataFlags = 0x01; // Can't fast travel? Interior?
		writer.startSubRecordTES4("DATA");
		writer.writeT<char>(dataFlags);
		writer.endSubRecordTES4("DATA");
		writer.endRecordTES4("CELL");
	}
	else
	{

		// load the pre-generated SubRecord list of references for this cell
		std::map<std::string, std::deque<int> >::const_iterator references =
			mState.getSubRecords().find (Misc::StringUtils::lowerCase (cellRecordPtr->get().mId));

		bool bHasPersistentRefs = false;
		bool bHasTempRefs = false;

		std::deque<int>& interiorCellPersistentRefList = mState.mPersistentRefMap[Misc::StringUtils::lowerCase(cellRecordPtr->get().mId)];
		if (interiorCellPersistentRefList.empty() != true)
			bHasPersistentRefs = true;

		if (references != mState.getSubRecords().end() && references->second.empty() != true)
			bHasTempRefs = true;

		if (cellRecordPtr->isModified() || cellRecordPtr->isDeleted() &&
			(bHasPersistentRefs || bHasTempRefs) )
		{
			// formID was previously generated in setup() method, so throw an error if it's zero
			if (cellFormID == 0)
			{
				throw std::runtime_error ("export: cellFormID is 0: " + cellRecordPtr->get().mId);
			}

			// prepare record flags
			std::string cellEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId, 1);
	//		std::cout << "Exporting interior cell [" << strEDID << "]" << std::endl;

			uint32_t flags=0;
			if (cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x800; // DISABLED

			//***********EXPORT INTERIOR CELL*****************************/
			writer.startRecordTES4 (cellRecordPtr->get().sRecordId, flags, cellFormID, cellEDID);
			cellRecordPtr->get().exportTES4 (writer);
        
			// Cell record ends before creation of child records (which are full records and not subrecords)
			writer.endRecordTES4 (cellRecordPtr->get().sRecordId);

			// display a period for progress feedback
			std::cout << ".";
		
			// write reference records
			if (bHasPersistentRefs || bHasTempRefs)
			{
				// Create Cell children group
				writer.startGroupTES4(cellFormID, 6); // top Cell Children Group

				//***************** INTERIOR CELL Persistent Children Group ****************/
				if (bHasPersistentRefs)
				{
					writer.startGroupTES4(cellFormID, 8); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children
					for (std::deque<int>::const_iterator refindex_iter = interiorCellPersistentRefList.begin();
						refindex_iter != interiorCellPersistentRefList.end(); refindex_iter++)
					{
						const CSMWorld::Record<CSMWorld::CellRef>& ref =
							mDocument.getData().getReferences().getRecord (*refindex_iter);

						if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
						{
							CSMWorld::CellRef refRecord = ref.get();

//							uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID, "BASEREF");
							CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);
							if (baseRefIndex.first != -1)
							{
								std::string sSIG;
								switch (baseRefIndex.second)
								{
								case CSMWorld::UniversalId::Type::Type_Npc:
									sSIG = "ACHR";
									break;
								case CSMWorld::UniversalId::Type::Type_Creature:
									sSIG = "ACRE";
									break;
								case CSMWorld::UniversalId::Type::Type_Door:
								default:
	//								if (refRecord.mTeleport != true)
	//									continue;
									sSIG = "REFR";
									break;
								}
								std::string refEDID = "";
								std::string compString = Misc::StringUtils::lowerCase(refRecord.mRefID);
								if (writer.mBaseObjToScriptedREFList.find(compString) != writer.mBaseObjToScriptedREFList.end())
								{
									refEDID = writer.generateEDIDTES4(refRecord.mRefID, 0, "PREF");
								}
								else
								{
									refEDID = refRecord.mId;
								}
								uint32_t refFormID = writer.crossRefStringID(refRecord.mId, sSIG, false, true);
								if (refFormID == 0)
								{
									throw std::runtime_error("Interior Cell Persistent Reference: retrieved invalid formID from *refindex lookup");
								}

								uint32_t teleportDoorRefID = 0;
								ESM::Position returnPosition;
								//**************************TELEPORT DOOR*****************************/
								if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Door &&
									refRecord.mTeleport == true)
								{
									teleportDoorRefID = FindSiblingDoor(mDocument, mState, refRecord, refFormID, returnPosition);
								}
								//**************************TELEPORT DOOR*****************************/

								uint32_t refFlags=0;
								if (ref.mState == CSMWorld::RecordBase::State_Deleted)
									refFlags |= 0x800; // DISABLED
								refFlags |= 0x400; // persistent flag
								// check for NPCDremora
								if ((Misc::StringUtils::lowerCase(refRecord.mRefID).find("dremora") != std::string::npos &&
									Misc::StringUtils::lowerCase(sSIG) == "acre"))
								{
									sSIG = "ACHR";
								}
								// sanity check: try to resolve baseFormID here, if no formID found, then use static XMarker (0x03B)
								std::string baseEDID = writer.generateEDIDTES4(refRecord.mRefID, 0, "");
								uint32_t baseFormID = writer.crossRefStringID(baseEDID, "BASEREF", false);
								if (baseFormID == 0)
								{
									// TODO: write error message	
									sSIG = "REFR";
								}
								// start record
								writer.startRecordTES4(sSIG, refFlags, refFormID, refEDID);
								refRecord.exportTES4(writer, mDocument, refEDID, teleportDoorRefID, &returnPosition);
								// end record
								writer.endRecordTES4(sSIG);


							}
						}
					}
					// close cell children group
					writer.endGroupTES4(cellFormID); // cell children subgroup
				} // if (bHasPersistentRefs)

				//************EXPORT INTERIOR CELL TEMPORARY CHILDREN GROUP*******************/
				if (bHasTempRefs)
				{
					writer.startGroupTES4(cellFormID, 9); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children
					//****************EXPORT INTERIOR PATHGRID*****************/
					int pathgridIndex = mDocument.getData().getPathgrids().searchId(cellRecordPtr->get().mId);
					if (pathgridIndex != -1 && cellRecordPtr->isModified())
					{
						const CSMWorld::Record<CSMWorld::Pathgrid>& pathgrid
							= mDocument.getData().getPathgrids().getRecord (pathgridIndex);
						// check for over-riding and deleting and stuff
						std::string pathgridStringID = cellRecordPtr->get().mId + "-pathgrid";
						uint32_t pathgridFormID = writer.crossRefStringID(pathgridStringID, "PGRD", false, false);
	//					std::cout << "DEBUG: crossRef (interior) found " << pathgridStringID << " [" << std::hex << pathgridFormID << "]" << std::endl;
						writer.CompressNextRecord();
						writer.startRecordTES4("PGRD", 0, pathgridFormID, pathgridStringID);
						pathgrid.get().exportSubCellTES4(writer, 0, 0, true);
						writer.endRecordTES4("PGRD");
					}


					for (std::deque<int>::const_reverse_iterator iter(references->second.rbegin());
						iter != references->second.rend(); ++iter)
					{
						const CSMWorld::Record<CSMWorld::CellRef>& ref =
							mDocument.getData().getReferences().getRecord (*iter);

						CSMWorld::CellRef refRecord = ref.get();
						if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
						{
							CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);	
							if (baseRefIndex.first != -1)
							{
								std::string sSIG;
								switch (baseRefIndex.second)
								{
								case CSMWorld::UniversalId::Type::Type_Npc:
									sSIG = "ACHR";
									break;
		//                            continue;
								case CSMWorld::UniversalId::Type::Type_Creature:
									sSIG = "ACRE";
									break;
								case CSMWorld::UniversalId::Type::Type_Door:
								default:
									sSIG = "REFR";
									break;
								}
								// use mID for non-persistent (aka "temporary") ref
								uint32_t refFormID = writer.crossRefStringID(refRecord.mId, "REFR", false, false);

								uint32_t refFlags=0;
								if (ref.mState == CSMWorld::RecordBase::State_Deleted)
									refFlags |= 0x800; // DISABLED
								
								// check for NPCDremora
								if ( (Misc::StringUtils::lowerCase(refRecord.mRefID).find("dremora") != std::string::npos &&
									Misc::StringUtils::lowerCase(sSIG) == "acre") )
								{
									sSIG = "ACHR";
								}
								// sanity check: try to resolve baseFormID here, if no formID found, then use static XMarker (0x03B)
								std::string baseEDID = writer.generateEDIDTES4(refRecord.mRefID, 0, "");
								uint32_t baseFormID = writer.crossRefStringID(baseEDID, "BASEREF", false);
								if (baseFormID == 0)
								{
									// TODO: write error message	
									sSIG = "REFR";
								}

								// start record
								writer.startRecordTES4(sSIG, refFlags, refFormID, refRecord.mId);
//								refRecord.exportTES4(writer, mDocument, "");
								refRecord.exportTES4 (writer, mDocument, refRecord.mId);
								// end record
								writer.endRecordTES4(sSIG);
							}
						}
					}
					// close cell children group
					writer.endGroupTES4(cellFormID); // cell temp children subgroup
				}

				writer.endGroupTES4(cellFormID); // 6 top cell children group
			}
		}
	} // if cellRecordPtr == nullptr else...

	if (stage == (mNumCells-1))
	{
		// two for the block-subblock
		writer.endGroupTES4(0);
		writer.endGroupTES4(0);
		// third one is the top Group
		writer.endGroupTES4("CELL");
	}

}

CSMDoc::ExportExteriorCellCollectionTES4Stage::ExportExteriorCellCollectionTES4Stage (Document& document,
	SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportExteriorCellCollectionTES4Stage::setup()
{
    int blockX, blockY, subblockX, subblockY;
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getCells().getSize();
	std::ostringstream debugstream;

	for (int i=0; i < collectionSize; i++)
	{
		CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr = &mDocument.getData().getCells().getNthRecord(i);
		bool exterior = cellRecordPtr->get().mId.substr (0, 1)=="#";

		if (exterior == true)
		{
			// check if it has a name, create dummycellname entry
			if (cellRecordPtr->get().mName != "" && cellRecordPtr->isModified())
			{
				bool nameFound=false;
				std::string cellName = cellRecordPtr->get().mName;
				for (auto searchName = mState.mDummyCellNames.begin(); searchName != mState.mDummyCellNames.end(); searchName++)
				{
					if (Misc::StringUtils::lowerCase(*searchName) == Misc::StringUtils::lowerCase(cellName))
					{
						nameFound = true;
						break;
					}
				}
				if (nameFound == false)
				{
					if (cellName.find("#") != std::string::npos)
					{
						std::cout << "ERROR!!!!!! Exterior Cell Address Name being added to DummyCellNames list: " << cellName << "\n";
					}
					mState.mDummyCellNames.push_back(cellName);
				}
			}
			
			// create space for new entry and copy cellRecord pointer
			debugstream.str(""); debugstream.clear();
			debugstream << "Collecting CellExport Data: i=[" << i << "] ";
            CellExportData *exportData = new CellExportData;
            exportData->cellRecordPtr = cellRecordPtr;

            // translate to Oblivion coords, then calculate block and subblock
            int baseX = cellRecordPtr->get().mData.mX*2;
            int baseY = cellRecordPtr->get().mData.mY*2;
			debugstream << "X,Y[" << baseX << "," << baseY << "] ";

			// assign formID
//			uint32_t formID = mState.crossRefCellXY(baseX, baseY);
			uint32_t formID = 0;

			// generate new EDID:formID pair based on ESM4 cell coords
			int x, y;
			for (y=0; y < 2; y++)
			{
				for (x=0; x < 2; x++)
				{
					std::ostringstream generatedSubCellID;
					generatedSubCellID << "#" << baseX+x << " " << baseY+y;
					int tempID = writer.crossRefStringID(generatedSubCellID.str(), "CELL", false, true);
					if (tempID == 0)
					{
						tempID = writer.reserveFormID(0, generatedSubCellID.str(), "CELL", false);
//						std::ostringstream debug2;
//						debug2 << "DEBUG: reserving formID: " << generatedSubCellID.str() << " [" << std::hex << tempID << "]" << std::endl;
//						std::cout << debug2.str();
//						OutputDebugString(debug2.str().c_str());
					}
					if (formID == 0 && (x==0 && y==0))
					{
						formID = tempID;
					}
				}
			}
			
			exportData->formID = formID;
			debugstream << "formID[" << formID << "] ";

			// calculate block and subblock coordinates
			// Thanks to Zilav's forum post for the Oblivion/Fallout Grid algorithm
			subblockX = baseX/8;
            if ((baseX < 0) && ((baseX % 8) != 0))
            {
				subblockX--;
            }
			blockX = subblockX/4;
			if ((subblockX < 0) && ((subblockX % 4) != 0))
			{
				blockX--;
			}

			subblockY = baseY/8;
			if ((baseY < 0) && ((baseY % 8) != 0))
			{
				subblockY--;
			}
			blockY = subblockY/4;
			if ((subblockY < 0) && ((subblockY % 4) != 0))
			{
				blockY--;
			}

			debugstream << "Block=[" << blockX << "," << blockY << "] ";
			debugstream << "Subblock=[" << subblockX << "," << subblockY << "] ";

			mCellExportList.push_back(*exportData);

			// retrieve the block from the worldgrid
			if (WorldGrid.find(blockX) == WorldGrid.end() || WorldGrid[blockX].find(blockY) == WorldGrid[blockX].end())
			{
				debugstream << "New Block... (" << WorldGrid.size() << ") ";
				WorldGrid[blockX][blockY] = new BlockT;
			}
			BlockT *block = WorldGrid[blockX][blockY];
			// retrieve subblock from block
			if (block->find(subblockX) == block->end() || (*block)[subblockX].find(subblockY) == (*block)[subblockX].end())
			{
				debugstream << "New Sub-Subblock... (" << block->size() << ") ";
				(*block)[subblockX][subblockY] = new SubBlockT;
			}
			SubBlockT *subblock = (*block)[subblockX][subblockY];
			// map cell to coord in subblock
			if ((*subblock)[baseX].find(baseY) != (*subblock)[baseX].end())
			{
				exit(-1);
				throw std::runtime_error("bad programming of subblocks");
			}
			(*subblock)[baseX][baseY] = exportData;

			// record the block and subblock used in a separate list
			GridTrackT *gridTrack = new GridTrackT;
			gridTrack->blockX = blockX;
			gridTrack->blockY = blockY;
			gridTrack->subblockX = subblockX;
			gridTrack->subblockY = subblockY;
			GridTracker.push_back(*gridTrack);
			debugstream << std::endl;

//			OutputDebugString(debugstream.str().c_str());			
//            std::cout << "formID=" << formID << "; i=" << i << "; X,Y=[" << baseX << "," << baseY << "]; ";
//			std::cout << "Block[" << blockX << "," << blockY << "][" << subblockX << "," << subblockY << "]" << std::endl;
		}
	}

	// count number of cells in all blocks to return
	mNumCells = 0;
    mNumCells = mCellExportList.size();
    
	return mNumCells;

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
	//	CSMWorld::Record<CSMWorld::Cell>& cell = mDocument.getData().getCells().getRecord (stage);
    CellExportData *exportData;
    CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr=0;
	int blockX, blockY, subblockX, subblockY;
	uint32_t cellFormID;
	SubBlockT *subblock;
	uint16_t groupid_gridYX[2];
	std::ostringstream debugstream;

	// if stage == 0, then add the group record first
	//*********************START WORLD GROUP HEADER**************************/
	if (stage == 0 && mIsWrldHeaderWritten == false)
	{
		mIsWrldHeaderWritten = true;
		debugstream.str(""); debugstream.clear();
		debugstream << "writing worldspace header..." << std::endl;
		OutputDebugString(debugstream.str().c_str());
		std::cout << debugstream.str();

		std::string sSIG="WRLD";
		writer.startGroupTES4(sSIG, 0); // Top GROUP
		
		// Create WRLD record
//		uint32_t wrldFormID = writer.reserveFormID(0x01380000, "WrldMorrowind");
		uint32_t wrldFormID = writer.crossRefStringID("WrldMorrowind", "WRLD", false, true);
		if (wrldFormID == 0)
			wrldFormID = writer.reserveFormID(0x01380000, "WrldMorrowind", "WRLD", true);
		writer.startRecordTES4("WRLD", 0, wrldFormID, "WrldMorrowind");
		writer.startSubRecordTES4("EDID");
		writer.writeHCString("WrldMorrowind");
		writer.endSubRecordTES4("EDID");
		writer.startSubRecordTES4("FULL");
		writer.writeHCString("Morroblivion");
		writer.endSubRecordTES4("FULL");
		// WNAM (parent world WRLD)
		// CNAM (climate CLMT)
		// NAM2 (water WATR)
		writer.startSubRecordTES4("NAM2");
		writer.writeT<uint32_t>(0x18);
		writer.endSubRecordTES4("NAM2");
		writer.startSubRecordTES4("ICON");
		writer.writeHCString("Menus\\map\\world\\Vvardenfell.dds");
		writer.endSubRecordTES4("ICON");
		// MNAM { 32{X, Y}, 16{ {X,Y}, {X,Y} } }
		writer.startSubRecordTES4("MNAM");
		// usable dimensions
		writer.writeT<int32_t>(2048); // X dimension
		writer.writeT<int32_t>(2048); // Y dimension
		writer.writeT<int16_t>(-88); // NW, X
		writer.writeT<int16_t>(63); // NW, Y
		writer.writeT<int16_t>(103); // SE, X
		writer.writeT<int16_t>(-128); // SE, Y
		writer.endSubRecordTES4("MNAM");
		// DATA (flags)
		uint32_t flags = 0;
		writer.startSubRecordTES4("DATA");
		writer.writeT<uint8_t>(flags);
		writer.endSubRecordTES4("DATA");
		writer.startSubRecordTES4("NAM0");
		writer.writeT<float>(-71 * 4096); // min x
		writer.writeT<float>(-62 * 4096); // min y
		writer.endSubRecordTES4("NAM0");
		writer.startSubRecordTES4("NAM9");
		writer.writeT<float>(100 * 4096); // max x
		writer.writeT<float>(61 * 4096); // max y
		writer.endSubRecordTES4("NAM9");
		writer.endRecordTES4("WRLD");
		
		// Create WRLD Children Top Group
		writer.startGroupTES4(wrldFormID, 1);
		
		// Create persistent CELL dummy record
		flags=0x400;
		uint32_t dummyCellFormID = writer.reserveFormID(0x01380001, "wrldmorrowind-dummycell", "CELL", true);
		writer.startRecordTES4("CELL", flags, dummyCellFormID, "");
		writer.startSubRecordTES4("DATA");
		writer.writeT<uint8_t>(0x02); // flag: has water=0x02
		writer.endSubRecordTES4("DATA");
		writer.startSubRecordTES4("XCLC");
		writer.writeT<int32_t>(0); // X=0
		writer.writeT<int32_t>(0); // Y=0
		writer.endSubRecordTES4("XCLC");
		writer.endRecordTES4("CELL");
		// Create CELL dummy top children group
		writer.startGroupTES4(dummyCellFormID, 6); // top Cell Children Group
		// Create CELL dummy persistent children subgroup
		writer.startGroupTES4(dummyCellFormID, 8); // grouptype=8 (persistent children)

		std::cout << "Exporting persistent world refs";
		// Write out persistent refs here...
		std::deque<int>& worldspacePersistentRefList = mState.mPersistentRefMap["worldspace-dummycell"];
		for (std::deque<int>::const_iterator refindex_iter = worldspacePersistentRefList.begin();
			 refindex_iter != worldspacePersistentRefList.end(); refindex_iter++)
		{
			const CSMWorld::Record<CSMWorld::CellRef>& ref =
			mDocument.getData().getReferences().getRecord (*refindex_iter);
			
			CSMWorld::CellRef refRecord = ref.get();
			
			CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);
			if (baseRefIndex.first != -1)
			{
				sSIG="";
				switch (baseRefIndex.second)
				{
					case CSMWorld::UniversalId::Type::Type_Npc:
						sSIG = "ACHR";
						break;
					case CSMWorld::UniversalId::Type::Type_Creature:
						sSIG = "ACRE";
						break;
					default:
						sSIG = "REFR";
						break;
				}
				std::string strEDID = "";
				std::string compString = Misc::StringUtils::lowerCase(refRecord.mRefID);
				if (writer.mBaseObjToScriptedREFList.find(compString) != writer.mBaseObjToScriptedREFList.end())
				{
					strEDID = writer.generateEDIDTES4(refRecord.mRefID, 0, "PREF");
				}
				else
				{
					strEDID = refRecord.mId;
				}
				uint32_t refFormID = writer.crossRefStringID(refRecord.mId, sSIG, false, true);
				if (refFormID == 0)
				{
					throw std::runtime_error("Exterior Cell Persistent Reference: retrieved invalid formID from *refindex lookup: " + strEDID);
				}
				uint32_t teleportDoorRefID=0;
				ESM::Position returnPosition;
				//**************************TELEPORT DOOR*****************************/
				if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Door &&
					refRecord.mTeleport == true)
				{
					teleportDoorRefID = FindSiblingDoor(mDocument, mState, refRecord, refFormID, returnPosition);
				}
				//**************************TELEPORT DOOR*****************************/
				uint32_t refFlags= 0x0400; // persistent ref on
				if (ref.mState == CSMWorld::RecordBase::State_Deleted)
					refFlags |= 0x800; // DISABLED
				// check for NPCDremora
				if ((Misc::StringUtils::lowerCase(refRecord.mRefID).find("dremora") != std::string::npos &&
					Misc::StringUtils::lowerCase(sSIG) == "acre"))
				{
					if ( Misc::StringUtils::lowerCase(refRecord.mRefID) == "TR_m2_q_4_SorenDremora")
						sSIG = "ACHR";
				}
				// sanity check: try to resolve baseFormID here, if no formID found, then use static XMarker (0x03B)
				std::string baseEDID = writer.generateEDIDTES4(refRecord.mRefID, 0, "");
				uint32_t baseFormID = writer.crossRefStringID(baseEDID, "BASEREF", false);
				if (baseFormID == 0)
				{
					// TODO: write error message	
					sSIG = "REFR";
				}

				// start record
				writer.startRecordTES4(sSIG, refFlags, refFormID, strEDID);
				refRecord.exportTES4 (writer, mDocument, strEDID, teleportDoorRefID, &returnPosition);
				// end record
				writer.endRecordTES4(sSIG);
//				debugstream.str(""); debugstream.clear();
//				debugstream << "writing persistent record: type=[" << sSIG << "] baseEDID=[" << refRecord.mRefID << "] formID=[" << baseRefID << "]" << std::endl;
//				OutputDebugString(debugstream.str().c_str());
//				std::cout << debugstream.str();
			}
			std::cout << ".";
		} // foreach... worldspacePersistentRefList
		std::cout << "persistent world refs done." << std::endl;
		writer.endGroupTES4(0); // (8) dummy cell's persistent children subgroup
		writer.endGroupTES4(0); // (6) dummy cell's top children group		
	}	
	//*********************END WORLD GROUP HEADER**************************/

	//********************BEGIN PROCESSING EXTERIOR CELLS*****************/
	//***************BEGIN CELL SELECTION********************/
	// Cells must be written to file in sets of contigous blocks and subblocks
	if (stage == 0)
	{
		std::cout << "Now exporting cells and refs";
		GridTrackT gridTrack = GridTracker[0];
		blockX = gridTrack.blockX;
		blockY = gridTrack.blockY;
		subblockX = gridTrack.subblockX;
		subblockY = gridTrack.subblockY;
		oldBlockX = blockX;
		oldBlockY = blockY;
		oldSubblockX = subblockX;
		oldSubblockY = subblockY;
		cellCount = 0;

		// initialize first exterior Cell Block, grouptype=4; label=0xYYYYXXXX
		groupid_gridYX[0] = blockY;
		groupid_gridYX[1] = blockX;
		writer.startGroupTES4( *((uint32_t *)groupid_gridYX), 4);
		// initialize first exterior Cell SubBlock, grouptype=5; label=0xYYYYXXXX
		groupid_gridYX[0] = subblockY;
		groupid_gridYX[1] = subblockX;
		writer.startGroupTES4( *((uint32_t *)groupid_gridYX), 5);
	}

	blockX = oldBlockX;
	blockY = oldBlockY;
	subblockX = oldSubblockX;
	subblockY = oldSubblockY;
	
	bool cellFound=false;
	while (cellFound == false)
	{
		// retrieve block and subblock groups from the current coordinates
		BlockT *block = WorldGrid[blockX][blockY];
		subblock = (*block)[subblockX][subblockY];
		// check if any cells are left
		if (subblock->begin() == subblock->end())
		{
			// if no more subblocks in block, check the gridtrack
			cellCount = 0;
			// if no more cells in subblock, search block for more subblocks
			// start with the first subblock and iterate down
			BlockT::iterator blockBranchX;
			std::map<int, SubBlockT* >::iterator blockBranchY;
			bool siblingSubBlockFound=false;
			for (blockBranchX = block->begin();
				 blockBranchX != block->end() && siblingSubBlockFound != true;
				 blockBranchX++)
			{
				for (blockBranchY = blockBranchX->second.begin();
					 blockBranchY != blockBranchX->second.end() && siblingSubBlockFound != true;
					 blockBranchY++)
				{
					SubBlockT *tempSubBlock = blockBranchY->second;
					if (tempSubBlock->size() > 0 )
					{
						subblockX = blockBranchX->first;
						subblockY = blockBranchY->first;
						siblingSubBlockFound = true;
					}
				}
			}
			if (siblingSubBlockFound == true)
				continue;
			// remove gridtrack and try again
			debugstream.str(""); debugstream.clear();
			debugstream << "no more cells in subblock, getting next GridTracker... ";
			if (GridTracker.begin() == GridTracker.end())
				throw std::runtime_error("exterior cell exporter broke!");
			// retrieve the next gridtrack from the tracker and process a cell
			GridTrackT gridTrack = GridTracker[1];
			blockX = gridTrack.blockX;
			blockY = gridTrack.blockY;
			subblockX = gridTrack.subblockX;
			subblockY = gridTrack.subblockY;
			debugstream << "trying block[" << blockX << "," << blockY << "] subblock[" << subblockX << "," << subblockY << "]" << std::endl;
			GridTracker.erase(GridTracker.begin());
		}
		else if (subblock->begin()->second.begin() != subblock->begin()->second.end())
		{
			cellFound = true;
			debugstream << "get the first subblock" << std::endl;
//			OutputDebugString(debugstream.str().c_str());
			exportData = subblock->begin()->second.begin()->second;
		}
		else
		{
			// delete prior column(row?) of cells and start the next
			subblock->erase(subblock->begin());
		}
	}	
    cellRecordPtr = exportData->cellRecordPtr;
    cellFormID = exportData->formID;
	//******************* END CELL SELECTION ***********************/

	debugstream.str(""); debugstream.clear();
	debugstream << "Processing exterior cell: BLOCK[" << blockX << "," << blockY << "] SUBBLOCK[" << subblockX << "," << subblockY << "] ";
	debugstream << "X,Y[" << cellRecordPtr->get().mData.mX*2 << "," << cellRecordPtr->get().mData.mY*2 << "] ";
	debugstream << "CellCount=[" << ++cellCount << "]" << std::endl;
//	OutputDebugString(debugstream.str().c_str());
//	std::cout << debugstream.str();

	if (cellRecordPtr == 0)
	{
		// throw exception
		throw std::runtime_error ("export error: cellRecordPtr uninitialized");
	}

	//*****************CHECK FOR END OF BLOCK/SUBBLOCK***********/
	// check to see if group is initialized
    if ((subblockX != oldSubblockX) || (subblockY != oldSubblockY))
	{
		// close previous subblock
		debugstream.str(""); debugstream.clear();
		debugstream << "Closing Subblock [" << oldSubblockX << "," << oldSubblockY << "]...";
		writer.endGroupTES4(0);
        if ((blockX != oldBlockX) || (blockY != oldBlockY))
		{
			debugstream << "Closing Block [" << oldBlockX << "," << oldBlockY << "]";
			writer.endGroupTES4(0);
			// start the next block if prior block was closed
			groupid_gridYX[0] = blockY;
			groupid_gridYX[1] = blockX;
			writer.startGroupTES4(*((uint32_t *)groupid_gridYX), 4);
		}
		debugstream << std::endl;
//		OutputDebugString(debugstream.str().c_str());
		// start new subblock
		groupid_gridYX[0] = subblockY;
		groupid_gridYX[1] = subblockX;
		writer.startGroupTES4(*((uint32_t *)groupid_gridYX), 5);
	}
    oldBlockX = blockX; oldBlockY = blockY;
    oldSubblockX = subblockX; oldSubblockY = subblockY;

	//============================================================================================================

	if (cellFormID == 0)
	{
		throw std::runtime_error ("export: cellFormID is 0");
	}

	uint32_t flags=0;
	// ************* EXPORT EXTERIOR CELL RECORD TIMES 4 SUBCELLS **********************
	int baseX=cellRecordPtr->get().mData.mX * 2;
	int baseY=cellRecordPtr->get().mData.mY * 2;
	for (int y=0, subCell=0; y < 2; y++)
	{
		for (int x=0; x < 2; x++)
		{
			// load the pre-generated SubRecord list of references for this subcell
			std::ostringstream generatedSubCellID;
			generatedSubCellID << "#" << baseX+x << " " << baseY+y;

			debugstream.str(""); debugstream.clear();
			debugstream << "Processing OblivionCell[" << generatedSubCellID.str() << "]:(" << cellFormID << ")";

			std::map<std::string, std::deque<int> >::const_iterator references =
				mState.getSubRecords().find (Misc::StringUtils::lowerCase (generatedSubCellID.str()));

			bool bTempRefsPresent = false, bLandscapePresent = false;
			// check if child records are present (temp refs)
			if ( references != mState.getSubRecords().end() && references->second.empty() != true ) 
			{
				debugstream << "refs found...";
				bTempRefsPresent = true;
			}
			// landscape record present
			std::ostringstream landID;
			landID << "#" << (baseX/2) << " " << (baseY/2);
			if (mDocument.getData().getLand().searchId(landID.str()) != -1 )
			{
				debugstream << "landscape data found...";
				int landIndex = mDocument.getData().getLand().getIndex(landID.str());
				auto landRecord = mDocument.getData().getLand().getRecord(landIndex);
				if (landRecord.isModified())
					bLandscapePresent = true;
			}
			debugstream << std::endl;
//			OutputDebugString(debugstream.str().c_str());
//			std::cout << debugstream.str();
			debugstream.str(""); debugstream.clear();

			if ( (cellRecordPtr->isModified() || cellRecordPtr->isDeleted()) &&
				(bTempRefsPresent || bLandscapePresent) )
			{
				// TODO: Fix ReferenceCounter?
				// count new references and adjust RefNumCount accordingsly
				int newRefNum = cellRecordPtr->get().mRefNumCounter;
				if (references!=mState.getSubRecords().end())
				{
					for (std::deque<int>::const_iterator iter (references->second.begin());
						iter!=references->second.end(); ++iter)
					{
						const CSMWorld::Record<CSMWorld::CellRef>& ref =
							mDocument.getData().getReferences().getRecord (*iter);
						if (ref.get().mNew)
							++(cellRecordPtr->get().mRefNumCounter);
					}
				}

//				if (subCell > 0)
				if (true) // debug testing for CellFormID crossRef Bug
				{
					cellFormID = mState.crossRefCellXY(baseX+x, baseY+y);
					if (cellFormID == 0)
					{
						// formID was generated & reserved during setup phase
						cellFormID = writer.crossRefStringID(generatedSubCellID.str(), "CELL", false, false);
						if (cellFormID == 0)
						{
							throw std::runtime_error("ERROR: cellFormID was not found for: " + generatedSubCellID.str());
						}
					}
				}


/**************** STOP! THIS IS BREAKING STUFF!!!!!!!!!!!!!!!!!*********************
				// add EDID:formID association in StringMap for named exterior city cells
				if (cellRecordPtr->get().mName != "")
				{
					std::string namedCellEDID = writer.generateEDIDTES4(cellRecordPtr->get().mName, 1);
					writer.mStringIDMap.insert(std::make_pair(namedCellEDID, cellFormID));
					writer.mStringTypeMap.insert(std::make_pair(namedCellEDID, "CELL"));
				}
***********************************************************************************/


				// ********************EXPORT SUBCELL HERE **********************
				flags = 0;
				if (cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted)
					flags |= 0x800; // DO NOT USE DELETED FLAG, USE DISABLED INSTEAD
				writer.startRecordTES4 (cellRecordPtr->get().sRecordId, flags, cellFormID, generatedSubCellID.str());
				cellRecordPtr->get().exportSubCellTES4 (writer, baseX+x, baseY+y, subCell++);

				// Cell record ends before creation of child records (which are full records and not subrecords)
				writer.endRecordTES4 (cellRecordPtr->get().sRecordId);

				// display a period for progress feedback
				std::cout << ".";

				// TODO: cross-reference Region string with formID

				// Create Cell children group
				writer.startGroupTES4(cellFormID, 6); // top Cell Children Group
				// TODO: create VWD group, add VWD refs

				//******************TEMPORARY CHILDREN*****************************
				writer.startGroupTES4(cellFormID, 9); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children

//				debugstream.str(""); debugstream.clear();
//				debugstream << "retrieving landID=[" << landID.str() << "] ...";

				//******************EXPORT LANDSCAPE*****************/
				if (bLandscapePresent)
				{
					int landIndex = mDocument.getData().getLand().getIndex(landID.str());
//					debugstream << "ID retrieved.  exporting land ... ";
					std::ostringstream ssLandscapeEDID;
					ssLandscapeEDID << "#" << baseX + x << " " << baseY + y << "-landscape";
					uint32_t landFormID = mState.crossRefLandXY(baseX + x, baseY + y);
					if (landFormID == 0)
					{
						landFormID = writer.crossRefStringID(ssLandscapeEDID.str(), "LAND", false, false);
//						std::cout << "DEBUG: crossRef found " << ssLandscapeEDID.str() << " [" << std::hex << landFormID << "]" << std::endl;
					}
					writer.CompressNextRecord();
					writer.startRecordTES4("LAND", 0, landFormID, ssLandscapeEDID.str());
					mDocument.getData().getLand().getRecord(landIndex).get().exportSubCellTES4(writer, x, y);

					// VTEX, LTEX formIDs (each morroblivion subcell maps to 8x8 portion of the original 16x16 morrowind cell vtex grid)
					// each Subcell is further divided into quadrants containing a 4x4 portion of the original 16x16 morrowind cell vtex grid
					const ESM::Land::LandData *landData = mDocument.getData().getLand().getRecord(landIndex).get().getLandData(ESM::Land::DATA_VTEX);
					if (landData != 0)
					{
						uint32_t texformID;
						int i, j, u, v, quadVal = 0;
						for (v = 0; v < 2; v++)
						{
							for (u = 0; u < 2; u++)
							{
								createPreBlendMap(writer, (baseX / 2), (baseY / 2), x, y, u, v);
								gatherPreBlendTextureList();
								if (mPreBlendTextureList.size() > 0)
								{
									exportLTEX_basetexture(writer, quadVal);
									exportLTEX_interpolatelayers(writer, quadVal);
								}
								quadVal++;
							}
						}
					}
					writer.endRecordTES4("LAND");
					debugstream << "done." << std::endl;
				}
				else
				{
//					debugstream << "no landscape data." << std::endl;
				}
//				OutputDebugString(debugstream.str().c_str());
				//**********END EXPORT LANDSCAPE************************/
				
				//****************EXPORT PATHGRID*****************/
				int pathgridIndex = mDocument.getData().getPathgrids().searchId(cellRecordPtr->get().mId);
				if (pathgridIndex != -1 && bLandscapePresent && cellRecordPtr->isModified() )
				{
					const CSMWorld::Record<CSMWorld::Pathgrid>& pathgrid
						= mDocument.getData().getPathgrids().getRecord (pathgridIndex);
					// check for over-riding and deleting and stuff
					std::ostringstream ssPathgridEDID;
					ssPathgridEDID << "#" << baseX+x << " " << baseY+y << "-pathgrid";
					uint32_t pathgridFormID = mState.crossRefPathgridXY(baseX+x, baseY+y);
					if (pathgridFormID == 0)
					{
						pathgridFormID = writer.crossRefStringID(ssPathgridEDID.str(), "PGRD", false, false);
//						std::cout << "DEBUG: crossRef found " << ssPathgridEDID.str() << " [" << std::hex << pathgridFormID << "]" << std::endl;
					}
					writer.CompressNextRecord();
					writer.startRecordTES4("PGRD", 0, pathgridFormID, ssPathgridEDID.str());
					pathgrid.get().exportSubCellTES4(writer, baseX+x, baseY+y);
					writer.endRecordTES4("PGRD");
				}

				// export Refs (ACRE, REFR)
				if (bTempRefsPresent)
				{
//					debugstream.str(""); debugstream.clear();
//					debugstream << "Exporting Temporary Refs (ACRE,REFR): ";
					for (std::deque<int>::const_reverse_iterator iter(references->second.rbegin());
						iter != references->second.rend(); ++iter)
					{
						const CSMWorld::Record<CSMWorld::CellRef>& ref =
							mDocument.getData().getReferences().getRecord (*iter);
						CSMWorld::CellRef refRecord = ref.get();
						std::string refStringID = refRecord.mId;
						uint32_t refFormID = writer.crossRefStringID(refStringID, "REFR", false);

						if ( (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted) )
						{

							// Check for uninitialized content file
							if (!refRecord.mRefNum.hasContentFile())
								refRecord.mRefNum.mContentFile = 0;

							// recalculate the ref's cell location
	//						std::ostringstream stream;

							// refnumcounter is broken by subCell structure
							// tally new record count to update cellRecordPtr mRefNumCounter
							if (refRecord.mNew)
								refRecord.mRefNum.mIndex = newRefNum++;

							// reserve formID
//							uint32_t refFormID = writer.getNextAvailableFormID();
//							refFormID = writer.reserveFormID(refFormID, refRecord.mId);
							CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);

							uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID, "BASEREF");
							if (baseRefID != 0)
							{
								std::string sSIG;
								switch (baseRefIndex.second)
								{
								case CSMWorld::UniversalId::Type::Type_Npc:
									sSIG = "ACHR";
									break;
								case CSMWorld::UniversalId::Type::Type_Creature:
									sSIG = "ACRE";
									break;
								default:
									sSIG = "REFR";
									break;
								}
								uint32_t refFlags=0;
								if (ref.mState == CSMWorld::RecordBase::State_Deleted)
									refFlags |= 0x800; // DO NOT USE DELETED FLAG, USE DISABLED INSTEAD
								// check for NPCDremora
								if ((Misc::StringUtils::lowerCase(refRecord.mRefID).find("dremora") != std::string::npos &&
									Misc::StringUtils::lowerCase(sSIG) == "acre"))
								{
									sSIG = "ACHR";
								}
								// sanity check: try to resolve baseFormID here, if no formID found, then use static XMarker (0x03B)
								std::string baseEDID = writer.generateEDIDTES4(refRecord.mRefID, 0, "");
								uint32_t baseFormID = writer.crossRefStringID(baseEDID, "BASEREF", false);
								if (baseFormID == 0)
								{
									// TODO: write error message	
									sSIG = "REFR";
								}

								// start record
								writer.startRecordTES4(sSIG, refFlags, refFormID, refStringID);
//								refRecord.exportTES4(writer, mDocument, "");
								refRecord.exportTES4(writer, mDocument, refStringID);
								// end record
								writer.endRecordTES4(sSIG);
								debugstream << "(" << sSIG << ")[" << refFormID << "] ";
							}

						} // end: if ( (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted) ...
					} // end: for (std::deque<int>::const_reverse_iterator iter(references->second.rbegin()) ...
//					debugstream << std::endl;
//					OutputDebugString(debugstream.str().c_str());

				}  // end: if (bTempRefsPresent)
				// end: export Refs (ACRE, REFR)

				// close cell children group
				writer.endGroupTES4(cellFormID); // (9) cell temporary children subgroup
				writer.endGroupTES4(cellFormID); // (6) top cell children group

			} // if subcell is modified | deleted | not-empty
			else 
			{
				// IF CELL IS NOT (modified | deleted | not-empty)
//				debugstream.str(""); debugstream.clear();
//				debugstream << "Cell X,Y[" << cellRecordPtr->get().mData.mX*2 << "," << cellRecordPtr->get().mData.mY*2 << "] ";
//				debugstream << "is identical to master, skipping. " << std::endl;
//				OutputDebugString(debugstream.str().c_str());
			}

		} // foreach subcell... x
	} // foreach subcell... y

	if (stage == (mNumCells-1))
	{
		// two for the final block-subblock
		writer.endGroupTES4(0);
		writer.endGroupTES4(0);
		// WRLD children Top Group
		writer.endGroupTES4(0);
		// third one is the WRLD Top Group
		writer.endGroupTES4("WRLD");
	}
//	std::cout << "Erase the current sub-block" << std::endl;
	subblock->begin()->second.erase( subblock->begin()->second.begin() );
	mCellExportList.pop_back();
	
}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::exportLTEX_interpolatelayers(ESM::ESMWriter& writer, int quadVal)
{
	std::ostringstream debugstream;
	int layer = -1;
	uint32_t texformID;
	auto tex_iter = mPreBlendTextureList.begin();

	// iterate through the remaining textures, exporting each as a separate layer
	for (tex_iter++; tex_iter != mPreBlendTextureList.end(); tex_iter++)
	{
		layer++;
		texformID = *tex_iter;
		writer.startSubRecordTES4("ATXT");
		writer.writeT<uint32_t>(texformID); // formID
		writer.writeT<uint8_t>(quadVal); // quadrant
		writer.writeT<uint8_t>(0); // unused
		writer.writeT<int16_t>(layer); // 16bit layer
		writer.endSubRecordTES4("ATXT");

		debugstream.str(""); debugstream.clear();
		debugstream << "[" << layer << "]Making OpacityMap: texformID=" << texformID << " ";
		// create an opacity map for this layer, then export it as VTXT record
		float opacityMap[17][17];
		int map_x, map_y;
		for (map_y = 0; map_y < 17; map_y++)
		for (map_x = 0; map_x < 17; map_x++)
		{
			// bilinear interopolate texture
			float map_u, map_v;
			float neighbors[2][2];
			map_u = ((map_x)/17.0); // (UV coords 0-1)
			map_v = ((map_y)/17.0);
			// assign neighbors (1.0 vs 0.0 if texture is present)
			// find nearest neighbor coordinate (on 6x6 preblendmap)
			int v_floor = floor(map_v * 4)+1;
			int u_floor = floor(map_u * 4)+1;
			int v_ceil = v_floor+1;
			int u_ceil = u_floor+1;

			if ((u_ceil >= 6) || (v_ceil >= 6))
				throw std::runtime_error("ERROR: Landtex bilerp - preblend offsets out of bounds"); 

			// get neighbor's values
			neighbors[0][0] = (mPreBlendMap[u_floor][v_floor] == texformID) ? 1.0f : 0.0f;
			neighbors[1][0] = (mPreBlendMap[u_ceil][v_floor] == texformID) ? 1.0f : 0.0f;
			neighbors[0][1] = (mPreBlendMap[u_floor][v_ceil] == texformID) ? 1.0f : 0.0f;
			neighbors[1][1] = (mPreBlendMap[u_ceil][v_ceil] == texformID) ? 1.0f : 0.0f;

			// now do the actual interpolation
			// first along x * 2
			float x_lerp1, x_lerp2;
			float wt_u = ((map_u*4)+1) - u_floor;
			float wt_v = ((map_v*4)+1) - v_floor;
			// calculate weights based on u==0 as texture A and u==1 as texture B
			x_lerp1 = (neighbors[0][0] * (1-wt_u)) + (neighbors[1][0] * (wt_u));
			x_lerp2 = (neighbors[0][1] * (1-wt_u)) + (neighbors[1][1] * (wt_u));
			// then along y of the 2 prior results
			float y_lerp;
			y_lerp = (x_lerp1 * (1-wt_v)) + (x_lerp2 * (wt_v));
//			if (y_lerp != 0)
//				debugstream << "pos=[" << map_x << "," << map_y << "] uv=(" << map_u << "," << map_v << ") X1=" << x_lerp1 << " X2=" << x_lerp2 << " Y=" << y_lerp << "; ";

			// now put it in the postmap
			opacityMap[map_x][map_y] = y_lerp;
		}
		debugstream << std::endl;
//		OutputDebugString(debugstream.str().c_str());

		int u, v;
		uint16_t position=0;
		float opacity=0;
		writer.startSubRecordTES4("VTXT");
		for (v=0; v < 17; v++)
		for (u=0; u < 17; u++)
		{
			opacity = opacityMap[u][v];
			if (opacity > 0.0f)
			{
				writer.writeT<uint16_t>(position); // offset into 17x17 grid
				writer.writeT<uint8_t>(0); // unused
				writer.writeT<uint8_t>(0); // unused
				writer.writeT<float>(opacity); // float opacity
			}
			position++;
		}
		writer.endSubRecordTES4("VTXT");
	}

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::exportLTEX_basetexture(ESM::ESMWriter& writer, int quadVal)
{
	int texformID;

	// export first texture as the base layer
	int16_t layer = -1;
	auto tex_iter = mPreBlendTextureList.begin();
	texformID = *tex_iter;
	writer.startSubRecordTES4("BTXT");
	writer.writeT<uint32_t>(texformID); // formID
	writer.writeT<uint8_t>(quadVal); // quadrant
	writer.writeT<uint8_t>(0); // unused
	writer.writeT<int16_t>(layer); // 16bit layer
	writer.endSubRecordTES4("BTXT");

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::gatherPreBlendTextureList()
{
	int x, y;
	uint32_t formID;
	mPreBlendTextureList.clear();

	// gather central 4x4
	for (y=1; y < 5; y++)
		for (x=1; x < 5; x++)
		{
			formID = mPreBlendMap[x][y];
			if (formID == 0)
				continue;
			auto tex_iter = mPreBlendTextureList.begin();
			bool matchFound = false;
			for (; tex_iter != mPreBlendTextureList.end(); tex_iter++)
			{
				if (*tex_iter == formID)
				{
					matchFound = true;
					break;
				}
			}
			if (matchFound == false)
			{
				mPreBlendTextureList.push_back(formID);
			}
		}
	// gather borders areas
	for (y=0; y < 6; y++)
		for (x=0; x < 6; x++)
		{
			if (x == 0 || x == 5 || y == 0 || y == 5) 
			{
				formID = mPreBlendMap[x][y];
				if (formID == 0)
					continue;
				auto tex_iter = mPreBlendTextureList.begin();
				bool matchFound = false;
				for (; tex_iter != mPreBlendTextureList.end(); tex_iter++)
				{
					if (*tex_iter == formID)
					{
						matchFound = true;
						break;
					}
				}
				if (matchFound == false)
				{
					mPreBlendTextureList.push_back(formID);
				}
			}
		}

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::createPreBlendMap(ESM::ESMWriter& writer, int origX, int origY, int subCX, int subCY, int quadX, int quadY)
{
	std::ostringstream landID, debugstream;
	const ESM::Land::LandData *landData;
	int landIndex, plugindex;

	// get central 4x4 grid
	if (getLandDataFromXY(origX, origY, plugindex, landData) == true)
	{
		int x, y;
		for (y=0; y < 4; y++)
			for (x=0; x < 4; x++)
				drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, x, y, x+1, y+1);

		// get 4 sides...
		// get side X-1
		drawLeftBorder(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);
		drawRightBorder(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);
		drawTopBorder(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);
		drawBottomBorder(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);

		// get 4 corners
		drawTopLeftCorner(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);
		drawTopRightCorner(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);
		drawBottomLeftCorner(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);
		drawBottomRightCorner(origX, origY, subCX, subCY, quadX, quadY, plugindex, landData);

	}
	else
	{
		debugstream.str(""); debugstream.clear();
		debugstream << "ERROR: createPreBlendMap-getLandDataFromXY: no landData found" << std::endl;
//		OutputDebugString(debugstream.str().c_str());
	}

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawTopLeftCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	int origX2=origX, origY2=origY;
	int subCX2=subCX, quadX2=quadX;
	int subCY2=subCY, quadY2=quadY;

	// move to top left corner quadrant
	// start with quadX-1 and quadY+1
	quadX2 = quadX - 1;
	quadY2 = quadY + 1;
	// evaluate boundaries and move subcells or cells as needed
	// if quadX2 < 0, then decrement subCX-1 and set quadX2=1
	if (quadX2 < 0)
	{
		subCX2 = subCX - 1;
		quadX2 = 1;
		// now re-evaluate new boundaries
		if (subCX2 < 0)
		{
			origX2 = origX - 1;
			subCX2 = 1;
			quadX2 = 1;
		}
	}
	if (quadY2 > 1)
	{
		subCY2 = subCY + 1;
		quadY2 = 0;
		// re-evaluate
		if (subCY2 > 1)
		{
			origY2 = origY + 1;
			subCY2 = 0;
			quadY2 = 0;
		}
	}
	const ESM::Land::LandData* tempData;
	int plugindex2;
	if (getLandDataFromXY (origX2, origY2, plugindex2, tempData) == true)
	{
		drawPreBlendMapXY(tempData, plugindex2, subCX2, subCY2, quadX2, quadY2, 3, 0, 0, 5);
	}
	else
	{
		// feather to blank or make hard edge
		drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, 0, 3, 0, 5);
	}

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawTopRightCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	int origX2=origX, origY2=origY;
	int subCX2=subCX, quadX2=quadX;
	int subCY2=subCY, quadY2=quadY;

	// move to top left corner quadrant
	// start with quadX-1 and quadY+1
	quadX2 = quadX + 1;
	quadY2 = quadY + 1;
	// evaluate boundaries and move subcells or cells as needed
	// if quadX2 < 0, then decrement subCX-1 and set quadX2=1
	if (quadX2 > 1)
	{
		subCX2 = subCX + 1;
		quadX2 = 0;
		// now re-evaluate new boundaries
		if (subCX2 > 1)
		{
			origX2 = origX + 1;
			subCX2 = 0;
			quadX2 = 0;
		}
	}
	if (quadY2 > 1)
	{
		subCY2 = subCY + 1;
		quadY2 = 0;
		// re-evaluate
		if (subCY2 > 1)
		{
			origY2 = origY + 1;
			subCY2 = 0;
			quadY2 = 0;
		}
	}
	const ESM::Land::LandData* tempData;
	int plugindex2;
	if (getLandDataFromXY (origX2, origY2, plugindex2, tempData) == true)
	{
		drawPreBlendMapXY(tempData, plugindex2, subCX2, subCY2, quadX2, quadY2, 0, 0, 5, 5);
	}
	else
	{
		// feather to blank or make hard edge
		drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, 3, 3, 5, 5);
	}

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawBottomLeftCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	int origX2=origX, origY2=origY;
	int subCX2=subCX, quadX2=quadX;
	int subCY2=subCY, quadY2=quadY;

	// move to top left corner quadrant
	// start with quadX-1 and quadY+1
	quadX2 = quadX - 1;
	quadY2 = quadY - 1;
	// evaluate boundaries and move subcells or cells as needed
	// if quadX2 < 0, then decrement subCX-1 and set quadX2=1
	if (quadX2 < 0)
	{
		subCX2 = subCX - 1;
		quadX2 = 1;
		// now re-evaluate new boundaries
		if (subCX2 < 0)
		{
			origX2 = origX - 1;
			subCX2 = 1;
			quadX2 = 1;
		}
	}
	if (quadY2 < 0)
	{
		subCY2 = subCY - 1;
		quadY2 = 1;
		// re-evaluate
		if (subCY2 < 0)
		{
			origY2 = origY - 1;
			subCY2 = 1;
			quadY2 = 1;
		}
	}
	const ESM::Land::LandData* tempData;
	int plugindex2;
	if (getLandDataFromXY (origX2, origY2, plugindex2, tempData) == true)
	{
		drawPreBlendMapXY(tempData, plugindex2, subCX2, subCY2, quadX2, quadY2, 3, 3, 0, 0);
	}
	else
	{
		// feather to blank or make hard edge
		drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, 0, 0, 0, 0);
	}

}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawBottomRightCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	int origX2=origX, origY2=origY;
	int subCX2=subCX, quadX2=quadX;
	int subCY2=subCY, quadY2=quadY;

	// move to top left corner quadrant
	// start with quadX-1 and quadY+1
	quadX2 = quadX + 1;
	quadY2 = quadY - 1;
	// evaluate boundaries and move subcells or cells as needed
	// if quadX2 < 0, then decrement subCX-1 and set quadX2=1
	if (quadX2 > 1)
	{
		subCX2 = subCX + 1;
		quadX2 = 0;
		// now re-evaluate new boundaries
		if (subCX2 > 1)
		{
			origX2 = origX + 1;
			subCX2 = 0;
			quadX2 = 0;
		}
	}
	if (quadY2 < 0)
	{
		subCY2 = subCY - 1;
		quadY2 = 1;
		// re-evaluate
		if (subCY2 < 0)
		{
			origY2 = origY - 1;
			subCY2 = 1;
			quadY2 = 1;
		}
	}
	const ESM::Land::LandData* tempData;
	int plugindex2;
	if (getLandDataFromXY (origX2, origY2, plugindex2, tempData) == true)
	{
		drawPreBlendMapXY(tempData, plugindex2, subCX2, subCY2, quadX2, quadY2, 0, 3, 5, 0);
	}
	else
	{
		// feather to blank or make hard edge
		drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, 3, 0, 5, 0);
	}

}


void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawLeftBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	// get side X-1
	int subCX2=subCX, quadX2=quadX;
	if (quadX == 1)
	{
		// retrieve from quadX=0;
		quadX2=0;
		int y;
		for (y=0; y < 4; y++)
			drawPreBlendMapXY(landData, plugindex, subCX2, subCY, quadX2, quadY, 3, y, 0, y+1);
	}
	else if (subCX == 1)
	{
		// retrieve from subCX=0, quadX=1;
		subCX2=0;
		quadX2=1;
		int y;
		for (y=0; y < 4; y++)
			drawPreBlendMapXY(landData, plugindex, subCX2, subCY, quadX2, quadY, 3, y, 0, y+1);
	}
	else
	{
		// retrieve from origX-1, subCX=1, quadX=1
		subCX2=1;
		quadX2=1;
		const ESM::Land::LandData* tempData;
		int plugindex2;
		if (getLandDataFromXY (origX-1, origY, plugindex2, tempData) == true)
		{
			int y;
			for (y=0; y < 4; y++)
				drawPreBlendMapXY(tempData, plugindex2, subCX2, subCY, quadX2, quadY, 3, y, 0, y+1);
		}
		else
		{
			// feather to blank or make hard edge
			for (int y=0; y < 4; y++)
				drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, 0, y, 0, y+1);
		}
	}
}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawRightBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	// get side X+1
	int subCX2=subCX, quadX2=quadX;
	if (quadX == 0)
	{
		// retrieve from quadX=0;
		quadX2=1;
		for (int y=0; y < 4; y++)
			drawPreBlendMapXY(landData, plugindex, subCX2, subCY, quadX2, quadY, 0, y, 5, y+1);
	}
	else if (subCX == 0)
	{
		// retrieve from subCX=1, quadX=0;
		subCX2=1;
		quadX2=0;
		for (int y=0; y < 4; y++)
			drawPreBlendMapXY(landData, plugindex, subCX2, subCY, quadX2, quadY, 0, y, 5, y+1);
	}
	else
	{
		// retrieve from origX+1, subCX=1, quadX=1
		subCX2=0;
		quadX2=0;
		const ESM::Land::LandData* tempData;
		int plugindex2;
		if (getLandDataFromXY (origX+1, origY, plugindex2, tempData) == true)
		{
			for (int y=0; y < 4; y++)
				drawPreBlendMapXY(tempData, plugindex2, subCX2, subCY, quadX2, quadY, 0, y, 5, y+1);
		}
		else
		{
			// feather to blank or make hard edge
			for (int y=0; y < 4; y++)
				drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, 3, y, 5, y+1);
		}
	}
}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawTopBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	// get side Y+1
	int subCY2=subCY, quadY2=quadY;
	if (quadY == 0)
	{
		// retrieve from quadX=0;
		quadY2=1;
		for (int x=0; x < 4; x++)
			drawPreBlendMapXY(landData, plugindex, subCX, subCY2, quadX, quadY2, x, 0, x+1, 5);
	}
	else if (subCY == 0)
	{
		// retrieve from subCX=1, quadX=0;
		subCY2=1;
		quadY2=0;
		for (int x=0; x < 4; x++)
			drawPreBlendMapXY(landData, plugindex, subCX, subCY2, quadX, quadY2, x, 0, x+1, 5);
	}
	else
	{
		// retrieve from origX+1, subCX=1, quadX=1
		subCY2=0;
		quadY2=0;
		const ESM::Land::LandData* tempData;
		int plugindex2;
		if (getLandDataFromXY (origX, origY+1, plugindex2, tempData) == true)
		{
			for (int x=0; x < 4; x++)
				drawPreBlendMapXY(tempData, plugindex2, subCX, subCY2, quadX, quadY2, x, 0, x+1, 5);
		}
		else
		{
			// feather to blank or make hard edge
			for (int x=0; x < 4; x++)
				drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, x, 3, x+1, 5);
		}
	}
}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawBottomBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData)
{
	// get side Y-1
	int subCY2=subCY, quadY2=quadY;
	if (quadY == 1)
	{
		// retrieve from quadX=0;
		quadY2=0;
		for (int x=0; x < 4; x++)
			drawPreBlendMapXY(landData, plugindex, subCX, subCY2, quadX, quadY2, x, 3, x+1, 0);
	}
	else if (subCY == 1)
	{
		// retrieve from subCY=0, quadY=1;
		subCY2=0;
		quadY2=1;
		for (int x=0; x < 4; x++)
			drawPreBlendMapXY(landData, plugindex, subCX, subCY2, quadX, quadY2, x, 3, x+1, 0);
	}
	else
	{
		// retrieve from origY-1, subCX=1, quadX=1
		subCY2=1;
		quadY2=1;
		const ESM::Land::LandData* tempData;
		int plugindex2;
		if (getLandDataFromXY (origX, origY-1, plugindex2, tempData) == true)
		{
			for (int x=0; x < 4; x++)
				drawPreBlendMapXY(tempData, plugindex, subCX, subCY2, quadX, quadY2, x, 3, x+1, 0);
		}
		else
		{
			// feather to blank or make hard edge
			for (int x=0; x < 4; x++)
				drawPreBlendMapXY(landData, plugindex, subCX, subCY, quadX, quadY, x, 0, x+1, 0);
		}
	}
}

bool CSMDoc::ExportExteriorCellCollectionTES4Stage::getLandDataFromXY(int origX, int origY, int& plugindex, const ESM::Land::LandData*& landData)
{
	std::ostringstream landID, debugstream;
	int landIndex;

	landID.str(""); landID.clear();
	landID << "#" << origX << " " << origY;
	if (mDocument.getData().getLand().searchId(landID.str()) != -1)
	{
		landIndex = mDocument.getData().getLand().getIndex(landID.str());
		int plugtemp = mDocument.getData().getLand().getRecord(landIndex).get().mPlugin;
//		if (plugtemp == 0)
//			plugtemp = mDocument.getData().getLand().getRecord(landIndex).getBase().mPlugin;
		plugindex = plugtemp;
		landData = mDocument.getData().getLand().getRecord(landIndex).get().getLandData(ESM::Land::DATA_VTEX);
		if (landData != 0)
			return true;
		else
		{
			debugstream << "WARNING: getLandDataFromXY[" << origX << "," << origY << "] - no vtex data" << std::endl;
//			OutputDebugString(debugstream.str().c_str());
//			throw std::runtime_error("ERROR: getLandDataFromXY - no vtex data");
		}
	}
	else
	{
		// This is an expected outcome to detect border regions.. not necessarily an error
		debugstream << "WARNING: getLandDataFromXY[" << origX << "," << origY << "] - couldn't find landID" << std::endl;
//		OutputDebugString(debugstream.str().c_str());
	}

	return false;
}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::drawPreBlendMapXY(const ESM::Land::LandData *landData, int plugindex, int subCX, int subCY, int quadX, int quadY, int inputX, int inputY, int outputX, int outputY)
{
	uint32_t defaultTex = 0x8C0; // TerrainHDDirt01dds
	std::ostringstream debugstream;

	int yoffset = (subCY*8) + (quadY*4) + inputY;
	int xoffset = (subCX*8) + (quadX*4) + inputX;
	if (landData == 0)
	{
		debugstream << "ERROR: drawPreBlendMapXY - landData is null!" << std::endl;
//		OutputDebugString(debugstream.str().c_str());
		throw std::runtime_error(debugstream.str().c_str());
	}
	int texindex = landData->mTextures[(yoffset*16)+xoffset]-1;
	if (texindex == -1)
	{
		// using locally defined defaultTex above
		mPreBlendMap[outputX][outputY] = defaultTex;
		return; 
	}
	int current_plugindex = plugindex;
	auto lookup = mState.mLandTexLookup_Plugin_Index[plugindex].find(texindex);
	if (lookup == mState.mLandTexLookup_Plugin_Index[plugindex].end())
	{
//		mPreBlendMap[outputX][outputY] = defaultTex;
//		return;
		debugstream.str("");
		debugstream << "WARNING: drawPreBlendMapXY - couldn't resolve texindex=" << texindex << ", (plugin=" << plugindex << "), attempting to search other plugins..." << "\n";
//		OutputDebugString(debugstream.str().c_str());
		auto pluginMap_it = mState.mLandTexLookup_Plugin_Index.begin();
		current_plugindex = pluginMap_it->first;
		lookup = pluginMap_it->second.find(texindex);
		while (lookup == pluginMap_it->second.end())
		{
			pluginMap_it++;
			if (pluginMap_it != mState.mLandTexLookup_Plugin_Index.end())
			{
				current_plugindex = pluginMap_it->first;
				lookup = pluginMap_it->second.find(texindex);
			}
			else
			{
				debugstream.str("");
				debugstream << "ERROR: drawPreBlendMapXY - couldn't resolve texindex=" << texindex << ", (plugin=" << plugindex << "), using defaultTexture." << std::endl;
//				OutputDebugString(debugstream.str().c_str());
				// just use defaultTex to continue gracefully instead of crashing
				mPreBlendMap[outputX][outputY] = defaultTex;
				return;
			}
		}
	}
	if (plugindex != current_plugindex)
	{
		debugstream.str("");
		debugstream << "DEBUG: drawPreBlendMapXY - couldn't resolve texindex=" << texindex << " (plugin=" << plugindex << "), using plugin=" << current_plugindex << " instead.\n";
//		OutputDebugString(debugstream.str().c_str());
//		std::cout << "X";
	}
	else
	{
		debugstream.str("");
		debugstream << "DEBUG: drawPreBlendMapXY - successfully resolved texindex=" << texindex << ", (plugin=" << plugindex << "), using plugin=" << current_plugindex << ".\n";
//		OutputDebugString(debugstream.str().c_str());
//		std::cout << "+";
	}
	uint32_t texformID = lookup->second;
	mPreBlendMap[outputX][outputY] = texformID;
}

// iterate through the 4x4 grid for this quadrant and add index=formID pair to map if not already there
void CSMDoc::ExportExteriorCellCollectionTES4Stage::gatherSubCellQuadrantLTEX(int subCX, int subCY, int quadX, int quadY, const ESM::Land::LandData *landData, int plugindex)
{
	std::ostringstream debugstream;
	mSubCellQuadTexList.clear();

	debugstream << "Collecting SubCell-Quadrant Textures List: (plugin=" << plugindex << ") ";
	int texindex=0;
	uint32_t formID;
	int gridpos=0;
	for (int i=0; i < 4; i++)
	{
		for (int j=0; j < 4; j++)
		{
			int yoffset = (subCY*8) + (quadY*4) + i;
			int xoffset = (subCX*8) + (quadX*4) + j;
			texindex = landData->mTextures[(yoffset*16)+xoffset]-1;
			debugstream << "[" << gridpos << "] texindex=" << texindex << " ";
			gridpos++;
			if (texindex == -1)
			{
				debugstream << "; ";
				continue; // todo: figure out what the default texture is
			}

			auto lookup = mState.mLandTexLookup_Plugin_Index[plugindex].find(texindex);
			if (lookup == mState.mLandTexLookup_Plugin_Index[plugindex].end())
			{
				debugstream << "<<not found, plugin=0...>> ";
				// try again with plugindex==0
				lookup = mState.mLandTexLookup_Plugin_Index[0].find(texindex);
				if (lookup == mState.mLandTexLookup_Plugin_Index[0].end())
				{
					debugstream << "ERROR: couldn't resolve texindex=" << texindex << std::endl;
					OutputDebugString(debugstream.str().c_str());
					throw std::runtime_error("ERROR: gatherSubCellQuadrantLTEX - couldn't resolve texindex");
				}
				std::stringstream err;
				err << "WARNING: gatherSubCellQuadrantLTEX(): could not resolve texindex=" << texindex << " (plugin=" << plugindex << "), using plugin=0\n";
				OutputDebugString(err.str().c_str());
			}

			formID = lookup->second;
			debugstream << "formID=[" << formID << "] ";
			auto tex_iter = mSubCellQuadTexList.begin();
			for (; tex_iter != mSubCellQuadTexList.end(); tex_iter++)
			{
				if (*tex_iter == formID)
				{
					break;
				}
			}
			if (tex_iter == mSubCellQuadTexList.end())
			{
				debugstream << "ADDED";
				mSubCellQuadTexList.push_back(formID);
			}
			debugstream << ";";
		}
	}
	debugstream << std::endl;
//	OutputDebugString(debugstream.str().c_str());

}

// create a 17x17 opacity map for specified layerID
void CSMDoc::ExportExteriorCellCollectionTES4Stage::calculateTexLayerOpacityMap(int subCX, int subCY, int quadX, int quadY, const ESM::Land::LandData *landData, int plugindex, int layerID)
{
	std::ostringstream debugstream;
	mTexLayerOpacityMap.clear();

	debugstream << "Create Opacity Map: layerID=[" << layerID << "] ";
	int texindex=0;
//	int quadIndex=0;
	uint32_t formID;
	int gridpos=0;
	for (int i=0; i < 4; i++)
	{
		for (int j=0; j < 4; j++)
		{
			int yoffset = (subCY*8) + (quadX*4) + i;
			int xoffset = (subCX*8) + (quadY*4) + j;
			texindex = landData->mTextures[(yoffset*16)+xoffset]-1;
			gridpos++;
			if (texindex == -1)
			{
				continue; // todo: figure out what the default texture is
			}

			auto lookup = mState.mLandTexLookup_Plugin_Index[plugindex].find(texindex);
			if (lookup == mState.mLandTexLookup_Plugin_Index[plugindex].end())
			{
				lookup = mState.mLandTexLookup_Plugin_Index[0].find(texindex);
				if (lookup == mState.mLandTexLookup_Plugin_Index[0].end())
				{
					throw std::runtime_error("ERROR: calculateTexLayerOpacityMap - couldn't resolve texindex");
				}
				else
				{
					std::stringstream err;
					err << "WARNING: calculateTexLayerOpacityMap(): could not resolve texindex=" << texindex << " (plugin=" << plugindex << "), using plugin=0\n";
					OutputDebugString(err.str().c_str());
				}
			}

			formID = lookup->second;
//			float opacity = (1.0f / mSubCellQuadTexList.size());
			if (layerID == formID)
			{
				debugstream << "[" << gridpos << "] pos=";
				// calculate positions
				int map_x, map_y;
				map_x = j * 17.0 / 4.0;
				map_y = i * 17.0 / 4.0;

				for (int y=0; y < 4; y++)
				for (int x=0; x < 4; x++)
				{
					uint16_t map_pos = ((map_y+y)*17)+(map_x+x);
					if (x > 0 && x < 4 && y > 0 && y < 4)
						mTexLayerOpacityMap[map_pos] = 1.0f;
					else
						mTexLayerOpacityMap[map_pos] = 0.8f;
					debugstream << map_pos << " ";
				}
				debugstream << "; ";
			}
		}
	}
	debugstream << std::endl;
//	OutputDebugString(debugstream.str().c_str());

}

CSMDoc::ExportPathgridCollectionTES4Stage::ExportPathgridCollectionTES4Stage (Document& document,
	SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportPathgridCollectionTES4Stage::setup()
{
	return mDocument.getData().getPathgrids().getSize();
}

void CSMDoc::ExportPathgridCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<CSMWorld::Pathgrid>& pathgrid =
		mDocument.getData().getPathgrids().getRecord (stage);

	if (pathgrid.isModified() || pathgrid.mState == CSMWorld::RecordBase::State_Deleted)
	{
		CSMWorld::Pathgrid record = pathgrid.get();

		if (record.mId.substr (0, 1)=="#")
		{
			std::istringstream stream (record.mId.c_str());
			char ignore;
			stream >> ignore >> record.mData.mX >> record.mData.mY;
		}
		else
			record.mCell = record.mId;

		writer.startRecord (record.sRecordId);
		record.save (writer, pathgrid.mState == CSMWorld::RecordBase::State_Deleted);
		writer.endRecord (record.sRecordId);
	}
}

//************EXPORT LAND STAGE NOT NECESSARY -- COMBINED WITH EXPORT CELL**********/
CSMDoc::ExportLandCollectionTES4Stage::ExportLandCollectionTES4Stage (Document& document,
	SavingState& state)
	: mDocument (document), mState (state)
{}
int CSMDoc::ExportLandCollectionTES4Stage::setup()
{
	return mDocument.getData().getLand().getSize();
}
void CSMDoc::ExportLandCollectionTES4Stage::perform (int stage, Messages& messages)
{
}


CSMDoc::ExportLandTextureCollectionTES4Stage::ExportLandTextureCollectionTES4Stage (Document& document,
	SavingState& state, bool skipMaster, bool doExport)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMaster;
	mSetupOnly = !doExport;
}

int CSMDoc::ExportLandTextureCollectionTES4Stage::setup()
{
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getLandTextures().getSize();

	for (int i = 0; i < collectionSize; i++)
	{
		const CSMWorld::Record<CSMWorld::LandTexture>& landTexture = mDocument.getData().getLandTextures().getRecord(i);
		CSMWorld::LandTexture record = landTexture.get();

		bool bExportRecord = false;
		if (mSkipMasterRecords)
			bExportRecord = landTexture.isModified() || landTexture.isDeleted();
		else
			bExportRecord = true;

		// create separate lookup tables for each plugin loaded
		std::string strEDID = writer.generateEDIDTES4(record.mId, 0, "LTEX");
		uint32_t formID = writer.crossRefStringID(strEDID, "LTEX", false, true);
		if (formID == 0)
		{
			formID = writer.getNextAvailableFormID();
			formID = writer.reserveFormID(formID, strEDID, "LTEX");
			if (formID == 0)
			{
				std::cout << "SETUP PHASE: LandTexture Collection ERROR! unable to allocate formID for texture: " << strEDID << "\n";
				continue;
			}
		}
		int plugindex = landTexture.get().mPluginIndex;
//		if (plugindex == 0)
//			plugindex = landTexture.getBase().mPluginIndex;

//		if (mState.mLandTexLookup_Plugin_Index.empty())
//			mState.mLandTexLookup_Plugin_Index.insert(std::make_pair(plugindex, std::map<int, uint32_t>()));
		mState.mLandTexLookup_Plugin_Index[plugindex][record.mIndex] = formID;

		//	debugstream << "INDEXED: (plugin=" << record.mPluginIndex << ") texindex=" << record.mIndex << " formid=[" << formID << "] mID=" << record.mId << std::endl;
		//	OutputDebugString(debugstream.str().c_str());

		if (writer.mUniqueIDcheck.find(formID) != writer.mUniqueIDcheck.end())
		{
			// formID already used in another record, don't worry this may be a one(Morroblivion)-to-many(Morrowind) mapping
			// just go ahead and skip to mapping index
			bExportRecord = false;
		}

		if (mSkipMasterRecords)
			bExportRecord &= ((formID & 0xFF000000) > 0x01000000);

		if (bExportRecord && !mSetupOnly)
		{
			mActiveRecords.push_back(i);
		}

	}

	if (!mSetupOnly)
		return mActiveRecords.size();
	else return 1;
//	mActiveRefCount =  mDocument.getData().getLandTextures().getSize();
//	return mActiveRefCount;
}

void CSMDoc::ExportLandTextureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::ostringstream debugstream;
	ESM::ESMWriter& writer = mState.getWriter();

	if (mSetupOnly)
		return;

	// LTEX GRUP
	if (stage == 0)
	{
		writer.startGroupTES4("LTEX", 0);
	}

	int recordIndex = mActiveRecords.at(stage);
	const CSMWorld::Record<CSMWorld::LandTexture>& landTexture = mDocument.getData().getLandTextures().getRecord(recordIndex);

	CSMWorld::LandTexture record = landTexture.get();

	// create separate lookup tables for each plugin loaded
	std::string strEDID = writer.generateEDIDTES4(record.mId, 0, "LTEX");
	uint32_t formID = writer.crossRefStringID(strEDID, "LTEX", false, true);

	uint32_t flags=0;
	if (landTexture.mState == CSMWorld::RecordBase::State_Deleted)
		flags |= 0x800; // DO NOT USE DELETED, USE DISABLED

	// avoid creating duplicate ltex records
	if ( (formID != 0) && (writer.mUniqueIDcheck.find(formID)!=writer.mUniqueIDcheck.end()) )
	{
		// do not write, issue warning
		std::cout << "ESMWRITER WARNING: duplicate LTEX record detectd, will skip: (" << strEDID << ") [" << std::hex << formID << "]" << std::endl;
	}
	else
	{
		writer.startRecordTES4("LTEX", flags, formID, strEDID);
		record.exportTESx(writer, true, 4);
		writer.endRecordTES4("LTEX");
	}

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4("LTEX");
	}

}

CSMDoc::ExportScriptTES4Stage::ExportScriptTES4Stage(Document& document, SavingState& state, bool skipMasters)
	: mDocument(document), mState(state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportScriptTES4Stage::setup()
{
	std::string sSIG = "SCPT";

	ESM::ESMWriter& writer = mState.getWriter();
	
	for (int i = 0; i < mDocument.getData().getScripts().getSize(); i++)
	{
		auto record = mDocument.getData().getScripts().getRecord(i);
		ESM::Script script = record.get();
		std::string strEDID = writer.generateEDIDTES4(script.mId, 0, sSIG);
		uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

		bool bExportRecord = false;
		if (mSkipMasterRecords)
		{
			bExportRecord = false;
			bExportRecord |= record.isModified();
			bExportRecord |= record.isDeleted();
		}
		else
		{
			bExportRecord = true;
		}

		if (bExportRecord)
		{
			mActiveRecords.push_back(i);
		}
	}

	return mActiveRecords.size();
}
void CSMDoc::ExportScriptTES4Stage::perform(int stage, Messages& messages)
{
	std::string sSIG = "SCPT";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	int recordIndex = mActiveRecords[stage];
	auto record = mDocument.getData().getScripts().getRecord(recordIndex);
	ESM::Script script = record.get();
	std::string strEDID = writer.generateEDIDTES4(script.mId, 0, sSIG);
	uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);

	bool bExportRecord = true;
	if (formID != 0 && writer.mUniqueIDcheck.find(formID) != writer.mUniqueIDcheck.end())
	{
		// formID already used in another record, don't worry this may be a one(Morroblivion)-to-many(Morrowind) mapping
		// just go ahead and skip to mapping index
		bExportRecord = false;
		std::cout << "record already written, skipping ahead: (" << sSIG << ") " << strEDID << std::endl;
	}

	if (bExportRecord)
	{
		writer.startRecordTES4(sSIG, 0, formID, strEDID);
		script.exportTESx(mDocument, writer);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRecords.size()-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::CloseExportTES4Stage::CloseExportTES4Stage (SavingState& state)
	: mState (state)
{}

int CSMDoc::CloseExportTES4Stage::setup()
{
	return 1;
}

void CSMDoc::CloseExportTES4Stage::perform (int stage, Messages& messages)
{
	mState.getWriter().updateTES4();
	mState.getStream().close();

	if (!mState.getStream())
		throw std::runtime_error ("saving failed");
}


CSMDoc::FinalizeExportTES4Stage::FinalizeExportTES4Stage (Document& document, SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::FinalizeExportTES4Stage::setup()
{
	return 1;
}

void CSMDoc::FinalizeExportTES4Stage::MakeBatchNIFFiles(ESM::ESMWriter& esm)
{
	std::cout << std::endl << "Creating batch file for NIF conversions...";

#ifdef _WIN32
    std::string outputRoot = "";
	std::string oblivionOutput = "C:/Oblivion.output/";
#else
    std::string outputRoot = getenv("HOME");
    outputRoot += "/";
	std::string oblivionOutput = outputRoot + "Oblivion.output/";
#endif
	outputRoot += "nifconv_bats/";

	// FarNifAutoGen support
	bool bFarNifAutoGen = true;
	if (esm.mConversionOptions.find("#farnifautogen") != std::string::npos)
	{
		bFarNifAutoGen = true;
	}

	bool bLegacyNifConv = true;
	if (esm.mConversionOptions.find("#oldnifconv") != std::string::npos)
	{
		bLegacyNifConv = true;
	}

	bool bBlenderOutput = false;
	if (esm.mConversionOptions.find("#blender") != std::string::npos)
		bBlenderOutput = true;
	bool bBlenderVWD = false;
	if (esm.mConversionOptions.find("#blendervwd") != std::string::npos)
		bBlenderVWD = true;
	
	bool bDoClothing = false;
    if (esm.mConversionOptions.find("#clothing") != std::string::npos ||
        esm.mConversionOptions.find("#armor") != std::string::npos)
        bDoClothing = true;

	std::string modStem = mDocument.getSavePath().filename().stem().string();
	std::string batchFileStem = "ModExporter_NIFConv_" + modStem;
	std::ofstream batchFileNIFConv;
	std::ofstream batchFileNIFConv_helper1;
	//	std::ofstream batchFileNIFConv_helper2;
	std::ofstream batchFileLODNIFConv;

	// new Blender List
	std::ofstream blenderOutList;
	std::ofstream blenderOutList_fullres;
	std::ofstream blenderOutList_far;
	std::ofstream blenderOutList_clothing;
	int jobnumber=0, fullres_jobnumber=0, far_jobnumber=0, clothing_jobnumber=0;
	char jobstring[6];
	int stringlen=0;

	std::string jobstem = oblivionOutput + "jobs/ModExporter_blender_" + modStem;
	std::string fullres_jobstem = jobstem + "_fullres_collision";
	std::string far_jobstem = jobstem + "_far";
	std::string clothing_jobstem = jobstem + "_clothing";

	bool bFullResCollision = true;
	if (esm.mConversionOptions.find("#qhull") != std::string::npos)
		bFullResCollision = false;

	// FarNifAutoGen support
	std::ofstream farNifAutoGen_nifjoblist;
	if (bFarNifAutoGen)
	{
		if (boost::filesystem::exists(oblivionOutput) == false)
		{
			boost::filesystem::create_directories(oblivionOutput);
		}
		std::string nifjoblist_filename = oblivionOutput + "FarNifAutoGen_nifjoblist_" + modStem;
		farNifAutoGen_nifjoblist.open(nifjoblist_filename + ".job", std::ios_base::out | std::ios_base::trunc);
	}

    if (bBlenderOutput)
    {
        if (boost::filesystem::exists(oblivionOutput + "jobs/") == false)
        {
            boost::filesystem::create_directories(oblivionOutput + "jobs/");
        }

        stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", clothing_jobnumber);
        jobstring[stringlen] = '\0';
        blenderOutList_clothing.open(clothing_jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);

        stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", fullres_jobnumber);
        jobstring[stringlen] = '\0';
        blenderOutList_fullres.open(fullres_jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);

        stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", far_jobnumber);
        jobstring[stringlen] = '\0';
        blenderOutList_far.open(far_jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);

        stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", jobnumber);
        jobstring[stringlen] = '\0';
        blenderOutList.open(jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);
    }

	if (bLegacyNifConv)
	{
        if (boost::filesystem::exists(outputRoot) == false)
        {
            boost::filesystem::create_directories(outputRoot);
        }

		batchFileNIFConv.open(outputRoot + batchFileStem + ".bat", std::ios_base::out | std::ios_base::trunc);
		batchFileNIFConv_helper1.open(outputRoot + batchFileStem + "_helper.dat", std::ios_base::out | std::ios_base::trunc);
		//	batchFileNIFConv_helper2.open(outputRoot + batchFileStem + "_helper2.dat", std::ios_base::out | std::ios_base::trunc);
		batchFileLODNIFConv.open(outputRoot + batchFileStem + "_LOD.bat", std::ios_base::out | std::ios_base::trunc);

		// set up header code for spawning
		batchFileNIFConv << "@echo off\n";
		batchFileNIFConv << "REM ModExporter_NIFConv batch file converter for " << modStem << " created with ModExporter" << "\n";
		batchFileNIFConv << "rename \"" << batchFileStem << "_helper.dat\" \"" << batchFileStem << "_helper.bat\"\n";
		//	batchFileNIFConv << "rename " << batchFileStem << "_helper2.dat " << batchFileStem << "_helper2.bat\n";
		batchFileNIFConv << "start cmd /c \"" << batchFileStem << "_helper.bat\"\n";
		//	batchFileNIFConv << "start cmd /c " << batchFileStem << "_herlper2.bat\n";
        batchFileNIFConv << "cd ..\n";

		batchFileNIFConv_helper1 << "@echo off\n";
		batchFileNIFConv_helper1 << "cd ..\n";
		//	batchFileNIFConv_helper2 << "@echo off\n";
		batchFileLODNIFConv << "@echo off\n";
		batchFileLODNIFConv << "cd ..\n";
	}

	int linecount = 0, far_linecount=0;
	int nSpawnCount = 0;
	for (auto nifConvItem = esm.mModelsToExportList.begin(); nifConvItem != esm.mModelsToExportList.end(); nifConvItem++)
	{
		std::string rawFilename = nifConvItem->first;
		std::string nifInputName = "";
		std::string nifOutputName = "";
		nifInputName = Misc::ResourceHelpers::correctActorModelPath(rawFilename, mDocument.getVFS());
		nifOutputName = nifConvItem->second.first;
		if (mDocument.getVFS()->exists("meshes/" + nifInputName) != true)
		{
			continue;
		}
        std::string archiveName = Misc::StringUtils::lowerCase( mDocument.getVFS()->lookupArchive("meshes/" + nifInputName) );
        if (archiveName.find("morrowind.bsa") != std::string::npos ||
            archiveName.find("tribunal.bsa") != std::string::npos  ||
            archiveName.find("bloodmoon.bsa") != std::string::npos)
        {
            if (nifConvItem->second.second != 4)
                continue;
        }
        std::cout << ".";

		if (bLegacyNifConv)
		{
			std::string cmdFlags;
			if (nifConvItem->second.second == 1) // door
			{
				cmdFlags = " -l 2 -s 6 -q 2";
			}
			else if (nifConvItem->second.second == 2) // weapon
			{
				cmdFlags = " -m 5 -l 5 -s 4 -q 3";
			}
			else if (nifConvItem->second.second == 3) // clutter
			{
				cmdFlags = " -l 4 -s 4 -q 3";
			}
			else if (nifConvItem->second.second == 0) // generic
			{
				cmdFlags = "";
			}
			else
			{
				// 4 is skipped, anything else is currently unhandled
			}

			if (nifConvItem->second.second == 4) // VWD/LOD -- skip here, process below
			{
				// skip
			}
			else
			{
				//int nSpawnNow = (nSpawnCount++ % 3);
				int nSpawnNow = (nSpawnCount++ % 2);
				if (nSpawnNow == 0)
				{
					batchFileNIFConv << "NIF_Conv.exe " << nifInputName << cmdFlags << " -d " << nifOutputName << "\n";
				}
				//else if (nSpawnNow == 1)
				else
				{
					batchFileNIFConv_helper1 << "NIF_Conv.exe " << nifInputName << cmdFlags << " -d " << nifOutputName << "\n";
				}
				//else
				//{
				//	batchFileNIFConv_helper2 << "NIF_Conv.exe " << nifInputName << " -d " << nifConvItem->second << "\n";
				//}
			}
		}

		// create LOD batch file
		if (nifConvItem->second.second == 4)
		{
			if (bLegacyNifConv)
			{
//				std::string lodFileName = nifConvItem->second.first.substr(0, nifConvItem->second.first.length() - 4) + "_far.nif";
                std::string lodFileName = nifConvItem->second.first;
				batchFileLODNIFConv << "NIF_Conv.exe " << nifInputName << " -l 15 -s 0 -q 0 -f -c " << " -d " << lodFileName << "\n";
			}

			if (bBlenderVWD)
			{
				// create New BlenderOutList
//				blenderOutList_far << Misc::ResourceHelpers::getNormalizedPath(nifOutputName).substr(0, nifOutputName.length() - 4) + "_far.nif" << "\n";
                blenderOutList_far << Misc::ResourceHelpers::getNormalizedPath(nifOutputName) << "\n";
				if (++far_linecount > 100)
				{
					far_linecount = 0;
					blenderOutList_far.close();
					stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", ++far_jobnumber);
					jobstring[stringlen] = '\0';
					blenderOutList_far.open(far_jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);
				}
			}

			if (bFarNifAutoGen)
			{
				// create FarNifAutoGen Nif_Job_List
				farNifAutoGen_nifjoblist << Misc::ResourceHelpers::getNormalizedPath(nifOutputName).substr(0, nifOutputName.length() - 8) + ".nif" << ",9999.0\n";
//				farNifAutoGen_nifjoblist << Misc::ResourceHelpers::getNormalizedPath(nifOutputName) << ",1.0\n";
			}

		}

		if (bBlenderOutput)
		{
			std::string cmdFlags;
			if (nifConvItem->second.second == 1) // door
			{
				cmdFlags = " -l 2 -s 6 -q 2";
			}
			else if (nifConvItem->second.second == 2) // weapon
			{
				cmdFlags = " -m 5 -l 5 -s 4 -q 3";
			}
			else if (nifConvItem->second.second == 3) // clutter
			{
				cmdFlags = " -l 4 -s 4 -q 3";
			}
			else if (nifConvItem->second.second == 0) // generic
			{
				cmdFlags = "";
			}

			// create New BlenderOutList (currently only types 1 & 2 supported
//			if (nifConvItem->second.second == 0 ||
//				nifConvItem->second.second == 1)
			if (true)
			{
				if (bFullResCollision)
					blenderOutList_fullres << Misc::ResourceHelpers::getNormalizedPath(nifOutputName) << ":" << nifInputName << ":" << cmdFlags << "\n";
				else
					blenderOutList << Misc::ResourceHelpers::getNormalizedPath(nifOutputName) << ":" << nifInputName << ":" << cmdFlags << "\n";
				if (++linecount > 100)
				{
					linecount = 0;
					if (bFullResCollision)
					{
						blenderOutList_fullres.close();
						stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", ++fullres_jobnumber);
						jobstring[stringlen] = '\0';
						blenderOutList_fullres.open(fullres_jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);
					}
					else
					{
						blenderOutList.close();
						stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", ++jobnumber);
						jobstring[stringlen] = '\0';
						blenderOutList.open(jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);
					}
				}
			}

		}

	}
	if (bLegacyNifConv)
	{
		batchFileNIFConv << "echo ----------------------\n";
		batchFileNIFConv << "echo Conversion of " << modStem << " is complete.  Press any key to close this window.\n";
		batchFileNIFConv << "pause\n";
        batchFileNIFConv << "cd nifconv_bats\n";
		batchFileNIFConv << "rename \"" << batchFileStem << "_helper.bat\" \"" << batchFileStem << "_helper.dat\"\n";
		//	batchFileNIFConv << "rename " << batchFileStem << "_helper2.bat " << batchFileStem << "_helper2.dat\n";

		batchFileNIFConv_helper1 << "echo ----------------------\n";
		batchFileNIFConv_helper1 << "\n\necho Helper thread: Conversion of " << modStem << " is complete.  Press any key to close this window.\n";
		batchFileNIFConv_helper1 << "pause\n";

		//	batchFileNIFConv_helper2 << "echo ----------------------\n";
		//	batchFileNIFConv_helper2 << "\n\necho Conversion of " << modStem << " is complete.  You may close this window.\n";
		//	batchFileNIFConv_helper2 << "pause\n";

		batchFileLODNIFConv << "echo ----------------------\n";
		batchFileLODNIFConv << "echo LOD mesh generation complete.\n";
		batchFileLODNIFConv << "pause\n";

		batchFileNIFConv.close();
		batchFileNIFConv_helper1.close();
		//	batchFileNIFConv_helper2.close();
		batchFileLODNIFConv.close();
	}

	if (bFarNifAutoGen)
	{
		farNifAutoGen_nifjoblist.close();
	}

    if (bBlenderOutput)
    {
        blenderOutList_fullres.close();
        blenderOutList.close();
        blenderOutList_far.close();
    }

	// ****************
	// ARMOR conversion
	// ****************
	std::ofstream batchFileArmorConv;
	if (bLegacyNifConv)
	{
		batchFileArmorConv.open(outputRoot + batchFileStem + "_ARMO.bat", std::ios_base::out | std::ios_base::trunc);
		batchFileArmorConv << "@echo off\n";
		batchFileArmorConv << "cd ..\n";
	}
	linecount = 0;
	for (auto nifConvItem = esm.mArmorToExportList.begin(); nifConvItem != esm.mArmorToExportList.end(); nifConvItem++)
	{
		std::cout << ".";
		std::string rawFilename = nifConvItem->first;
		std::string nifInputName = "";
		std::string nifOutputName = "";
		nifInputName = Misc::ResourceHelpers::correctActorModelPath(rawFilename, mDocument.getVFS());
		nifOutputName = nifConvItem->second.first;

/*
        if (mDocument.getVFS()->exists("meshes/" + nifInputName) != true)
        {
            continue;
        }
        std::string archiveName = Misc::StringUtils::lowerCase( mDocument.getVFS()->lookupArchive("meshes/" + nifInputName) );
        if (archiveName.find("morrowind.bsa") != std::string::npos ||
            archiveName.find("tribunal.bsa") != std::string::npos  ||
            archiveName.find("bloodmoon.bsa") != std::string::npos)
        {
            continue;
        }
*/
        std::cout << ".";

		std::string cmdFlags = "";
		cmdFlags = nifInputName;

		if (bLegacyNifConv)
		{
			batchFileArmorConv << "echo ----------------------\n";
			batchFileArmorConv << "echo NIF_Conv.exe " << cmdFlags << " -d " << nifOutputName << "\n";
			batchFileArmorConv << "NIF_Conv.exe " << cmdFlags << " -d " << nifOutputName << "\n";
		}

		if (bBlenderOutput && bDoClothing)
		{
			blenderOutList_clothing << Misc::ResourceHelpers::getNormalizedPath(nifOutputName) << ":" << nifInputName << ":" << " " << "\n";
			if (++linecount > 100)
			{
				linecount = 0;
				blenderOutList_clothing.close();
				stringlen = snprintf(jobstring, sizeof(jobstring), "_%04d", ++clothing_jobnumber);
				jobstring[stringlen] = '\0';
				blenderOutList_clothing.open(clothing_jobstem + jobstring + ".job", std::ios_base::out | std::ios_base::trunc);
			}

		}

	}

    if (bBlenderOutput)
    {
        blenderOutList_clothing.close();
    }

	if (bLegacyNifConv)
	{
		batchFileArmorConv << "echo ----------------------\n";
		batchFileArmorConv << "echo Armor mesh generation complete.\n";
		batchFileArmorConv << "pause\n";
		batchFileArmorConv.close();
	}
	std::cout << ". done.\n";

}

void CSMDoc::FinalizeExportTES4Stage::ExportDDSFiles(ESM::ESMWriter & esm)
{
	std::cout << std::endl << "Exporting DDS textures...";

	std::string modStem = mDocument.getSavePath().filename().stem().string();
	std::string logFileStem = "Exported_TextureList_" + modStem;
	std::ofstream logFileDDSConv;

#ifdef _WIN32
	std::string outputRoot = "C:/";
    std::string logRoot = "";
#else
	std::string outputRoot = getenv("HOME");
    outputRoot += "/";
    std::string logRoot = outputRoot;
#endif
    logRoot += "modexporter_logs/";
	// Log File is initialized in savingstate.cpp::InitializeSubstitutions()
	logFileDDSConv.open(logRoot + logFileStem + ".csv", std::ios_base::out | std::ios_base::app);
//	logFileDDSConv << "Original texture,Exported texture,Export result\n";
	boost::filesystem::path rootDir(outputRoot + "Oblivion.output/Data/Textures");

	if (boost::filesystem::exists(rootDir) == false)
	{
		boost::filesystem::create_directories(rootDir);
	}
	for (auto ddsConvItem = esm.mDDSToExportList.begin(); ddsConvItem != esm.mDDSToExportList.end(); ddsConvItem++)
	{
		std::string mw_filename = "";
		std::string ob_filename = "";
//		mw_filename = get_normalized_path(ddsConvItem->first);
//		ob_filename = get_normalized_path(ddsConvItem->second.first);
        mw_filename = Misc::ResourceHelpers::getNormalizedPath(ddsConvItem->first);
        ob_filename = Misc::ResourceHelpers::getNormalizedPath(ddsConvItem->second.first);

		int mode = ddsConvItem->second.second;
		std::string inputFilepath, outputFilepath;
		if (mode == 0) // landscape == 0
		{
			inputFilepath = Misc::ResourceHelpers::correctTexturePath(mw_filename, mDocument.getVFS());
//            Misc::ResourceHelpers::getNormalizedPath(inputFilepath);
			outputFilepath = "Textures/Landscape/" + ob_filename;
		}
		else if (mode == 1) // icons == 1
		{
			inputFilepath = Misc::ResourceHelpers::correctIconPath(mw_filename, mDocument.getVFS());
			outputFilepath = "Textures/Menus/Icons/" + ob_filename;
		}
		else if (mode == 2) // bookart == 2
		{
			inputFilepath = Misc::ResourceHelpers::correctBookartPath(mw_filename, mDocument.getVFS());
			outputFilepath = "Textures/Menus/" + ob_filename;
		}
		else
		{
			inputFilepath = Misc::ResourceHelpers::correctTexturePath(mw_filename, mDocument.getVFS());
			outputFilepath = "Textures/" + ob_filename;
		}
		if (mDocument.getVFS()->exists(inputFilepath) != true )
		{
			continue;
		}
        std::string archiveName = Misc::StringUtils::lowerCase( mDocument.getVFS()->lookupArchive(inputFilepath) );
        if (archiveName.find("morrowind.bsa") != std::string::npos ||
            archiveName.find("tribunal.bsa") != std::string::npos  ||
            archiveName.find("bloodmoon.bsa") != std::string::npos)
        {
            continue;
        }
        std::cout << ".";

		logFileDDSConv << inputFilepath << "," << outputFilepath << ",";

		try 
		{
			auto fileStream = mDocument.getVFS()->get(inputFilepath);
			std::ofstream newDDSFile;
			// create output subdirectories
			boost::filesystem::path p(outputRoot + "Oblivion.output/Data/" + outputFilepath);
			if (boost::filesystem::exists(p.parent_path()) == false)
			{
				boost::filesystem::create_directories(p.parent_path());
			}
			// DO NOT COPY IF FILE EXISTS
			if (boost::filesystem::exists(p.string()) == true)
			{
				continue;
			}
			newDDSFile.open(p.string(), std::ofstream::out | std::ofstream::binary );

/*
			osgDB::ReaderWriter* imageReader = osgDB::Registry::instance()->getReaderWriterForExtension(inputFilepath.substr(inputFilepath.find_last_of(".")+1));
//			osgDB::ReaderWriter* tgaWriter = osgDB::Registry::instance()->getReaderWriterForExtension("tga");
			osgDB::ReaderWriter* ddsWriter = osgDB::Registry::instance()->getReaderWriterForExtension("dds");
			auto result = imageReader->readImage(*fileStream);
			osg::Image *tempImage = result.getImage();
			if (tempImage != NULL)
			{
				if (mode == 1)
				{
					if (tempImage->getInternalTextureFormat() == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
						tempImage->getInternalTextureFormat() == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
						tempImage->getInternalTextureFormat() == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
						tempImage->getInternalTextureFormat() == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
					{
//						std::cout << " - S3TC detected.\n";
					}
					else
					{
//						icon->scaleImage(64, 64, 1);
					}
				}
//				ddsWriter->writeImage(*tempImage, newDDSFile);
//				imageReader->writeImage(*tempImage, newDDSFile);				
			}
			else
			{
				std::cout << "Failed to load: " << p.string() << "\n";
			}
*/
			int len = 0;
			char buffer[1024];
			while (fileStream->eof() == false)
			{
				fileStream->read(buffer, sizeof(buffer));
				len = fileStream->gcount();
				newDDSFile.write(buffer, len);
			}

			newDDSFile.close();

/*
			// if icon, scale x2
			if (mode == -1)
			{

				// open image from first file

				osg::Image *tempIcon = osgDB::readImageFile(p.string());
				if (tempIcon != NULL)
				{
					if (tempIcon->s() == 64)
					{
						std::string errorMesg = "[" + p.string() + "]: icon width already 64";
						throw (std::runtime_error(errorMesg.c_str()));
					}
					tempIcon->scaleImage(64, 64, 1);
					if (tempIcon->s() == 64)
					{
						// write out final file
						if (osgDB::writeImageFile((*tempIcon), p.string()) == true)
						{
							std::cout << "scaling success: " << p.string() << "\n";
						}
					}
					else
					{
						std::string errorMesg = "[" + p.string() + "]: unable scale to 64";
						throw (std::runtime_error(errorMesg.c_str()));
					}
				}

			}
*/
			logFileDDSConv << "export success\n";
		}
		catch (std::runtime_error e)
		{
			std::cout << "Error: (" << inputFilepath << ") " << e.what() << "\n";
			logFileDDSConv << "export error\n";
		}
		catch (...)
		{
			std::cout << "ERROR: something else bad happened!\n";
		}
	}
	std::cout << ". done.\n";

	logFileDDSConv.close();
}

void CSMDoc::FinalizeExportTES4Stage::perform (int stage, Messages& messages)
{
	if (mState.hasError())
	{
		mState.getWriter().updateTES4();
		mState.getWriter().close();
		mState.getStream().close();

		if (boost::filesystem::exists (mState.getTmpPath()))
			boost::filesystem::remove (mState.getTmpPath());

		return;
	}
	else if (!mState.isProjectFile())
	{
		if (boost::filesystem::exists (mState.getPath()))
			boost::filesystem::remove (mState.getPath());

		boost::filesystem::rename (mState.getTmpPath(), mState.getPath());

//		mDocument.getUndoStack().setClean();
	}

	ESM::ESMWriter& esm = mState.getWriter();

	MakeBatchNIFFiles(esm);
	ExportDDSFiles(esm);

	std::cout << std::endl << "Now writing out CSV log files..";

#ifdef _WIN32
    std::string outputRoot = "";
#else
    std::string outputRoot = getenv("HOME");
    outputRoot += "/";
#endif
    outputRoot += "modexporter_logs/";

	if (boost::filesystem::exists(outputRoot) == false)
	{
		boost::filesystem::create_directories(outputRoot);
	}

	std::string modStem = mDocument.getSavePath().filename().stem().string();
	// write unmatched EDIDs
	std::ofstream unmatchedCSVFile;
	unmatchedCSVFile.open(outputRoot + "UnresolvedEDIDlist_" + modStem + ".csv", std::ios_base::out | std::ios_base::trunc);
	// write header
	unmatchedCSVFile << "Record Types" << "," << "Mod File" << "," << "EDID" << "," << "Ref Count" << "," << "Put FormID Here" << "," << "Put Comments Here" << "," << "Position Offset" << "," << "Rotation Offset" << ", " << "Scale" << std::endl;

	for (auto edidItem = esm.unMatchedEDIDmap.begin();
		edidItem != esm.unMatchedEDIDmap.end();
		edidItem++)
	{
		// skip items that were created new
		if (esm.crossRefStringID(edidItem->first, edidItem->second.first, false, true) != 0)
		{
			// skip
		}
		else
		{
			// get sSIG from record
			std::string sSIG = "UNKN";
			auto recordType = esm.mStringTypeMap.find(Misc::StringUtils::lowerCase(edidItem->first));
			if (recordType != esm.mStringTypeMap.end() && recordType->second != "")
				sSIG = recordType->second;
			// list only items which were lost during conversion
			unmatchedCSVFile << sSIG << "," << mDocument.getSavePath().filename().stem().string() << ",\"" << edidItem->first << "\"," << edidItem->second.second << "," << "0x00" << "," << edidItem->second.first << "," << "" << "," << "" << "," << "" << std::endl;
	//		std::cout << "[" << count++ << "] " << edidItem->first << " - " << edidItem->second << std::endl;
		}
	}
	unmatchedCSVFile.close();

	// Write EDIDmap for exported records
	std::ofstream exportedEDIDCSVFile;
	exportedEDIDCSVFile.open(outputRoot + "ExportedEDIDlist_" + modStem + ".csv", std::ios_base::out | std::ios_base::trunc);
	// write header
	int index=0;
	exportedEDIDCSVFile << "Record Type" << "," << "Mod File" << "," << "EDID" << "," << "blank" << "," << "FormID" << "," << "Comments" << "," << "Position Offset" << "," << "Rotation Offset" << "," << "Scale" << "," << "Extra Tags" << std::endl;

	// generate EDID map for questscript lookup
	std::map<std::string, int> questScriptEDIDLookup;
	for (auto scriptRec = esm.mScriptToQuestList.begin(); scriptRec != esm.mScriptToQuestList.end(); scriptRec++)
	{
		int startscript = 0;
		std::string scriptEDID = esm.generateEDIDTES4(scriptRec->second.first, 0, "SCPT");
		if (esm.mAutoStartScriptEDID.find(Misc::StringUtils::lowerCase(scriptEDID)) != esm.mAutoStartScriptEDID.end())
		{
			startscript = 1;
		}
		questScriptEDIDLookup[Misc::StringUtils::lowerCase(scriptEDID)] = startscript;
	}


/*
	for (auto exportItem = esm.mStringIDMap.begin();
		exportItem != esm.mStringIDMap.end();
		exportItem++)
	{
		// skip items which have a different ESM index
		if ((exportItem->second & esm.mESMoffset) == esm.mESMoffset)
		{
			// get sSIG from record
			// if refEDID contains ref#, then check baseRecord for scriptedRef
			std::string sSIG = "";
			std::string sTAGS = "";
			auto recordType = esm.mStringTypeMap.find(Misc::StringUtils::lowerCase(exportItem->first));
			if (recordType != esm.mStringTypeMap.end() && recordType->second != "")
				sSIG = recordType->second;
			if (exportItem->first.find("ref#") != std::string::npos)
			{
				int tempIndex = mDocument.getData().getReferences().searchId(exportItem->first);
				auto record = mDocument.getData().getReferences().getRecord(tempIndex).get();
				std::string baseRecordID = Misc::StringUtils::lowerCase(record.mRefID);
				if (esm.mBaseObjToScriptedREFList.find(baseRecordID) != esm.mBaseObjToScriptedREFList.end())
				{
					if (sTAGS != "") sTAGS += " ";
					sTAGS = "#PREF" + esm.generateEDIDTES4(record.mRefID, 0, "PREF");
				}
			}
			if (Misc::StringUtils::lowerCase(sSIG) == "scpt")
			{
				if (questScriptEDIDLookup.find(Misc::StringUtils::lowerCase(exportItem->first)) != questScriptEDIDLookup.end())
				{
					if (sTAGS != "") sTAGS += " ";
					sTAGS += "#QUEST";
					if (questScriptEDIDLookup[exportItem->first] == 1)
					{
						sTAGS += " #START";
					}
				}
			}
			sTAGS = "\""+ sTAGS +"\"";
			exportedEDIDCSVFile << sSIG << "," << mDocument.getSavePath().filename().stem().string() << ",\"" << exportItem->first << "\"," << "" << "," << "0x" << std::hex << exportItem->second << "," << std::dec << index++ << "," << "," << "," << "," << sTAGS << std::endl;
		}
	}
	exportedEDIDCSVFile.close();
*/
	for (auto exportItem = esm.mFormIDMap.begin();
		exportItem != esm.mFormIDMap.end();
		exportItem++)
	{
		// skip items which have a different ESM index
		if ((exportItem->first & esm.mESMoffset) == esm.mESMoffset)
		{
			// get sSIG from record
			// if refEDID contains ref#, then check baseRecord for scriptedRef
			std::string sSIG = "";
			std::string sTAGS = "";
			auto recordType = esm.mStringTypeMap.find(Misc::StringUtils::lowerCase(exportItem->second));
			if (recordType != esm.mStringTypeMap.end() && recordType->second != "")
				sSIG = recordType->second;
			if (exportItem->second.find("ref#") != std::string::npos)
			{
				int tempIndex = mDocument.getData().getReferences().searchId(exportItem->second);
				auto record = mDocument.getData().getReferences().getRecord(tempIndex).get();
				std::string baseRecordID = Misc::StringUtils::lowerCase(record.mRefID);
				if (esm.mBaseObjToScriptedREFList.find(baseRecordID) != esm.mBaseObjToScriptedREFList.end())
				{
					if (sTAGS != "") sTAGS += " ";
					sTAGS = "#PREF" + esm.generateEDIDTES4(record.mRefID, 0, "PREF");
				}
			}
			if (Misc::StringUtils::lowerCase(sSIG) == "scpt")
			{
				if (questScriptEDIDLookup.find(Misc::StringUtils::lowerCase(exportItem->second)) != questScriptEDIDLookup.end())
				{
					if (sTAGS != "") sTAGS += " ";
					sTAGS += "#QUEST";
					if (questScriptEDIDLookup[exportItem->second] == 1)
					{
						sTAGS += " #START";
					}
				}
			}
			sTAGS = "\"" + sTAGS + "\"";
			exportedEDIDCSVFile << sSIG << "," << mDocument.getSavePath().filename().stem().string() << ",\"" << exportItem->second << "\"," << "" << "," << "0x" << std::hex << exportItem->first << "," << std::dec << index++ << "," << "," << "," << "," << sTAGS << std::endl;
		}
	}
	exportedEDIDCSVFile.close();


	// write unresolved local vars
	std::ofstream unresolvedLocalVarStream;
	unresolvedLocalVarStream.open(outputRoot + "UnresolvedLocalVars_" + modStem + ".csv", std::ios_base::out | std::ios_base::trunc);
	unresolvedLocalVarStream << "Local Var" << "," << "QuestVar Index" << "," << "Occurences" << std::endl;
	for (auto localVarItem = esm.mUnresolvedLocalVars.begin();
		localVarItem != esm.mUnresolvedLocalVars.end();
		localVarItem++)
	{
		unresolvedLocalVarStream << "short " << localVarItem->first << "," << "" << "," << localVarItem->second << std::endl;
	}
	unresolvedLocalVarStream.close();

	std::cout << "..Log writing done.\n\n[" << modStem << "] EXPORT COMPLETED! You may now close the ModExporter, or open another ESP." << std::endl;
}

uint32_t CSMDoc::FindSiblingDoor(Document& mDocument, SavingState& mState, CSMWorld::CellRef& refRecord, uint32_t refFormID, ESM::Position& returnPosition)
{
	uint32_t teleportDoorRefID = 0;

	ESM::ESMWriter& writer = mState.getWriter();

	// find teleport door (reference) near the doordest location...
	// add this reference + teleportDoorRefID ESM file offset to stack for second pass
	auto matchResult = mState.mReferenceToReferenceMap.find(refFormID);
	if (matchResult != mState.mReferenceToReferenceMap.end())
	{
		return teleportDoorRefID = matchResult->second;
	}

	std::map<std::string, std::deque<int>>::iterator destCellPersistentRefList;

	if (refRecord.mDestCell.length() != 0)
		// interior cell persistent ref list
		destCellPersistentRefList = mState.mPersistentRefMap.find(Misc::StringUtils::lowerCase (refRecord.mDestCell) );
	else
		// exterior cell persistent ref list
		destCellPersistentRefList = mState.mPersistentRefMap.find("worldspace-dummycell");

	if (destCellPersistentRefList == mState.mPersistentRefMap.end() )
	{
		// WORKAROUND: check the temp references for a door reference
		destCellPersistentRefList = mState.getSubRecords().find( Misc::StringUtils::lowerCase (refRecord.mDestCell) );
		if (destCellPersistentRefList == mState.getSubRecords().end() )
		{
			destCellPersistentRefList = mState.mBaseGameDoorList.find(Misc::StringUtils::lowerCase (refRecord.mDestCell));
			if (destCellPersistentRefList == mState.mBaseGameDoorList.end() )
			{
//				throw std::runtime_error("Find Sibling Door: ERROR: Destination Cell not found.");
				return 0;
			}
		}
	}

	for (auto dest_refIndex = destCellPersistentRefList->second.begin();
		dest_refIndex != destCellPersistentRefList->second.end(); dest_refIndex++)
	{
		auto destRef = mDocument.getData().getReferences().getRecord(*dest_refIndex);
		auto destRefBaseRecord = mDocument.getData().getReferenceables().getDataSet().searchId(destRef.get().mRefID);
		if (destRefBaseRecord.second == CSMWorld::UniversalId::Type_Door)
		{
			float distanceDoorToDest = writer.calcDistance(refRecord.mDoorDest, destRef.get().mPos);
			if (distanceDoorToDest > 300) 
			{
				continue;
			}
//			std::ostringstream destRefStringID;
//			destRefStringID << "*refindex" << *dest_refIndex;
//			std::string strEDID = writer.generateEDIDTES4(destRef.get().mId);
			std::string strEDID = destRef.get().mId;
			teleportDoorRefID = writer.crossRefStringID(strEDID, "REFR", false, false);
			if (teleportDoorRefID == 0)
			{
//				throw std::runtime_error("Find Sibling Door: ERROR: Destination door does not have a formID.");
				continue;
			}
			// WORKAROUND: check if this door is already registered in map, if so, look for another door
			if (mState.mReferenceToReferenceMap.find(teleportDoorRefID) != mState.mReferenceToReferenceMap.end() ||
				teleportDoorRefID == refFormID)
			{
				continue;
			}
			else
			{
				// add matching door ID as index to retrieve current Reference Door ID
				mState.mReferenceToReferenceMap.insert(std::make_pair(teleportDoorRefID, refFormID));
				for (int i=0; i < 3; i++)
				{				
					returnPosition.pos[i] = destRef.get().mPos.pos[i] - refRecord.mDoorDest.pos[i];
					returnPosition.rot[i] = refRecord.mDoorDest.rot[i];
				}
				break;
			}
		}
	}

	return teleportDoorRefID;
}

void CSMDoc::StartModRecord(const std::string& sSIG,  const std::string& mId, ESM::ESMWriter& esm, const CSMWorld::RecordBase::State& state)
{
	std::string strEDID = esm.generateEDIDTES4(mId, 0, sSIG);
	uint32_t formID = esm.crossRefStringID(strEDID, sSIG, false, true);
	uint32_t flags=0;
	if (state == CSMWorld::RecordBase::State_Deleted)
		flags |= 0x800; // DISABLED
	esm.startRecordTES4(sSIG, flags, formID, strEDID);
}

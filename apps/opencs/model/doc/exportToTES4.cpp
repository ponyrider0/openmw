#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#else
void inline OutputDebugString(char *c_string) { std::cout << c_string; };
void inline OutputDebugString(const char *c_string) { std::cout << c_string; };
#endif

#include <components/misc/stringops.hpp>
#include <boost/filesystem.hpp>
#include <QUndoStack>

#include "exportToTES4.hpp"
#include "document.hpp"

CSMDoc::ExportToTES4::ExportToTES4() : ExportToBase()
{
    std::cout << "TES4 Exporter Initialized " << std::endl;
}

void CSMDoc::ExportToTES4::defineExportOperation(Document& currentDoc, SavingState& currentSave)
{
	currentSave.getWriter().clearReservedFormIDs();
	currentSave.initializeSubstitutions();

	// Export to ESM file
	appendStage (new OpenExportTES4Stage (currentDoc, currentSave, true));

	appendStage (new ExportHeaderTES4Stage (currentDoc, currentSave, false));

/*
	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Global> >(mDocument.getData().getGlobals(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::GameSetting> >
		(mDocument.getData().getGmsts(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Script> >
		(mDocument.getData().getScripts(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BirthSign> >
		(mDocument.getData().getBirthsigns(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BodyPart> >
		(mDocument.getData().getBodyParts(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::SoundGenerator> >
		(mDocument.getData().getSoundGens(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::MagicEffect> >
		(mDocument.getData().getMagicEffects(), currentSave));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::StartScript> >
		(mDocument.getData().getStartScripts(), currentSave));
*/
	// Dialogue can reference objects and cells so must be written after these records for vanilla-compatible files

//	mExportOperation->appendStage (new ExportDialogueCollectionTES4Stage (mDocument, currentSave, false));

//	mExportOperation->appendStage (new ExportDialogueCollectionTES4Stage (mDocument, currentSave, true));

//	mExportOperation->appendStage (new ExportPathgridCollectionTES4Stage (mDocument, currentSave));

	bool skipMasterRecords = true;

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Spell> >
		(currentDoc.getData().getSpells(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

//	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Skill> >
//		(currentDoc.getData().getSkills(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Race> >
		(currentDoc.getData().getRaces(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Sound> >
		(currentDoc.getData().getSounds(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Class> >
		(currentDoc.getData().getClasses(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Faction> >
		(currentDoc.getData().getFactions(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Enchantment> >
		(currentDoc.getData().getEnchantments(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));

	appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Region> >
		(currentDoc.getData().getRegions(), currentSave, CSMWorld::Scope_Content, skipMasterRecords));
//	appendStage (new ExportClimateCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

	appendStage (new ExportLandTextureCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

// Separate Landscape export stage unneccessary -- now combined with export cell
//	mExportOperation->appendStage (new ExportLandCollectionTES4Stage (mDocument, currentSave));

	appendStage (new ExportWeaponCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportAmmoCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportMiscCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportKeyCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportSoulgemCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportLightCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportIngredientCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportClothingCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportBookCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportArmorCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportApparatusCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportPotionCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportLeveledItemCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

	appendStage (new ExportActivatorCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportSTATCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportFurnitureCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

	appendStage (new ExportDoorCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportContainerCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportFloraCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

	appendStage (new ExportNPCCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportCreatureCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));
	appendStage (new ExportLeveledCreatureCollectionTES4Stage (currentDoc, currentSave, skipMasterRecords));

	appendStage (new ExportReferenceCollectionTES4Stage (currentDoc, currentSave));
	appendStage (new ExportExteriorCellCollectionTES4Stage (currentDoc, currentSave));
	appendStage (new ExportInteriorCellCollectionTES4Stage (currentDoc, currentSave));

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
		throw std::runtime_error ("failed to open stream for saving");
}


CSMDoc::ExportHeaderTES4Stage::ExportHeaderTES4Stage (Document& document, SavingState& state, bool simple)
	: mDocument (document), mState (state), mSimple (simple)
{}

int CSMDoc::ExportHeaderTES4Stage::setup()
{
	mState.getWriter().setVersion();

	mState.getWriter().clearMaster();

	if (mSimple)
	{
		mState.getWriter().setAuthor ("");
		mState.getWriter().setDescription ("");
		mState.getWriter().setRecordCount (0);
		mState.getWriter().setFormat (ESM::Header::CurrentFormat);
	}
	else
	{
		mDocument.getData().getMetaData().save (mState.getWriter());
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
		mState.getWriter().addMaster ("Morrowind_ob - UCWUS.esp", 0);
		mState.getWriter().addMaster ("morroblivion-fixes.esp", 0);

	}

	return 1;
}

void CSMDoc::ExportHeaderTES4Stage::perform (int stage, Messages& messages)
{
	mState.getWriter().exportTES4 (mState.getStream());
}


CSMDoc::ExportDialogueCollectionTES4Stage::ExportDialogueCollectionTES4Stage (Document& document,
	SavingState& state, bool journal)
	: mState (state),
	mTopics (journal ? document.getData().getJournals() : document.getData().getTopics()),
	mInfos (journal ? document.getData().getJournalInfos() : document.getData().getTopicInfos())
{}

int CSMDoc::ExportDialogueCollectionTES4Stage::setup()
{
	return mTopics.getSize();
}

void CSMDoc::ExportDialogueCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<ESM::Dialogue>& topic = mTopics.getRecord (stage);

	if (topic.mState == CSMWorld::RecordBase::State_Deleted)
	{
		// if the topic is deleted, we do not need to bother with INFO records.
		ESM::Dialogue dialogue = topic.get();
		writer.startRecord(dialogue.sRecordId);
		dialogue.save(writer, true);
		writer.endRecord(dialogue.sRecordId);
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

	if (topic.isModified() || infoModified)
	{
		if (infoModified && topic.mState != CSMWorld::RecordBase::State_Modified
			&& topic.mState != CSMWorld::RecordBase::State_ModifiedOnly)
		{
			mState.getWriter().startRecord (topic.mBase.sRecordId);
			topic.mBase.save (mState.getWriter(), topic.mState == CSMWorld::RecordBase::State_Deleted);
			mState.getWriter().endRecord (topic.mBase.sRecordId);
		}
		else
		{
			mState.getWriter().startRecord (topic.mModified.sRecordId);
			topic.mModified.save (mState.getWriter(), topic.mState == CSMWorld::RecordBase::State_Deleted);
			mState.getWriter().endRecord (topic.mModified.sRecordId);
		}

		// write modified selected info records
		for (CSMWorld::InfoCollection::RecordConstIterator iter (range.first); iter!=range.second; ++iter)
		{
			if (iter->isModified() || iter->mState == CSMWorld::RecordBase::State_Deleted)
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

				writer.startRecord (info.sRecordId);
				info.save (writer, iter->mState == CSMWorld::RecordBase::State_Deleted);
				writer.endRecord (info.sRecordId);
			}
		}
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

CSMDoc::ExportAmmoCollectionTES4Stage::ExportAmmoCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportAmmoCollectionTES4Stage::setup()
{
	mActiveRefCount = 1;
	return mActiveRefCount;
}
void CSMDoc::ExportAmmoCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "AMMO";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	//	mDocument.getData().getReferenceables().getDataSet().getWeapons().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	for (auto ammoIndex = mState.mAmmoFromWeaponList.begin(); ammoIndex != mState.mAmmoFromWeaponList.end(); ammoIndex++)
	{
		const CSMWorld::Record<ESM::Weapon> weaponRec = mDocument.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(*ammoIndex);
		std::string strEDID = writer.generateEDIDTES4(weaponRec.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);

		bool exportOrSkip=false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = weaponRec.isModified() || weaponRec.isDeleted() ||
				 (formID == 0) || ((formID & 0xFF000000) > 0x01000000);
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
			weaponRec.get().exportAmmoTESx(writer, 4);
			writer.endRecordTES4(sSIG);
		}
	}

	if (stage == mActiveRefCount-1)
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
	ESM::ESMWriter& writer = mState.getWriter();

	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getWeapons().getSize();
	for (int i=0; i < mActiveRefCount; i++)
	{
		const CSMWorld::Record<ESM::Weapon>& record = mDocument.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(i);
		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);
		if (formID == 0)
		{
			formID = writer.getNextAvailableFormID();
			writer.reserveFormID(formID, strEDID);
		}
	}

	return mActiveRefCount;
}
void CSMDoc::ExportWeaponCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "WEAP";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getWeapons().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	const CSMWorld::Record<ESM::Weapon> weaponRec = mDocument.getData().getReferenceables().getDataSet().getWeapons().mContainer.at(stage);
	std::string strEDID = writer.generateEDIDTES4(weaponRec.get().mId);
	uint32_t formID = writer.crossRefStringID(strEDID, false);

	bool exportOrSkip=false;
	if (mSkipMasterRecords)
	{
		exportOrSkip = weaponRec.isModified() || weaponRec.isDeleted() ||
			 (formID == 0) || ((formID & 0xFF000000) > 0x01000000);
	}
	else
	{
		exportOrSkip = true;
	}

	// Move ammunition (arrows, bolts) to ammo collection
	if (exportOrSkip)
	{
		if ( (weaponRec.get().mData.mType == ESM::Weapon::Type::Arrow) || 
			(weaponRec.get().mData.mType == ESM::Weapon::Type::Bolt) )
		{
			mState.mAmmoFromWeaponList.push_back(stage);
			exportOrSkip = false;
		}
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

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount=1;
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
		debugstream << "Exporting furniture: size=" << mActiveRefCount << "... ";
//		OutputDebugString(debugstream.str().c_str());
		writer.startGroupTES4(sSIG, 0);
	}

	for (auto furnIndex = mState.mFurnitureFromActivatorList.begin(); furnIndex != mState.mFurnitureFromActivatorList.end(); furnIndex++)
	{
		//	mDocument.getData().getReferenceables().getDataSet().getActivators().exportTESx (index, mState.getWriter(), 4);
		const CSMWorld::Record<ESM::Activator> activatorRec = mDocument.getData().getReferenceables().getDataSet().getActivators().mContainer.at(*furnIndex);
		std::string strEDID = writer.generateEDIDTES4(activatorRec.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);

		bool exportOrSkip=false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = activatorRec.isModified() || activatorRec.isDeleted() || 
				(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
/*
			std::string strEDID = writer.generateEDIDTES4(activatorRec.get().mId);
			uint32_t formID = writer.crossRefStringID(strEDID);
			uint32_t flags=0;
			if (activatorRec.mState == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x800; // DISABLED
			writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
			StartModRecord(sSIG, activatorRec.get().mId, writer, activatorRec.mState);
			activatorRec.get().exportTESx(writer, 4);
			// MNAM
			writer.startSubRecordTES4("MNAM");
			writer.writeT<uint8_t>(0);
			writer.endSubRecordTES4("MNAM");
			writer.endRecordTES4(sSIG);
//			debugstream.str(""); debugstream.clear();
//			debugstream << "(" << *furnIndex << ")[" << formID << "] ";
//			OutputDebugString(debugstream.str().c_str());
		}
	}

	for (auto furnIndex = mState.mFurnitureFromStaticList.begin(); furnIndex != mState.mFurnitureFromStaticList.end(); furnIndex++)
	{
		const CSMWorld::Record<ESM::Static> staticRec = mDocument.getData().getReferenceables().getDataSet().getStatics().mContainer.at(*furnIndex);
		std::string strEDID = writer.generateEDIDTES4(staticRec.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);

		bool exportOrSkip=false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = staticRec.isModified() || staticRec.isDeleted() || 
				(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
/*
			std::string strEDID = writer.generateEDIDTES4(staticRec.get().mId);
			uint32_t formID = writer.crossRefStringID(strEDID);
			uint32_t flags=0;
			if (staticRec.mState == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x800; // DISABLED
			writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
			StartModRecord(sSIG, staticRec.get().mId, writer, staticRec.mState);
			staticRec.get().exportTESx(writer, 4);
			// MNAM
			writer.startSubRecordTES4("MNAM");
			writer.writeT<uint8_t>(0);
			writer.endSubRecordTES4("MNAM");
			writer.endRecordTES4(sSIG);
			debugstream.str(""); debugstream.clear();
//			debugstream << "(" << *furnIndex << ")[" << formID << "] ";
//			OutputDebugString(debugstream.str().c_str());
		}
	}


	if (stage == mActiveRefCount-1)
	{
		debugstream.str(""); debugstream.clear();
		debugstream << "complete." << std::endl;
//		OutputDebugString(debugstream.str().c_str());
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

	// GRUP
	if (stage == 0)
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

	if (stage == mActiveRefCount-1)
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

	// GRUP
	if (stage == 0)
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

	if (stage == mActiveRefCount-1)
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
	ESM::ESMWriter& writer = mState.getWriter();

	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().getSize();
	for (int i=0; i < mActiveRefCount; i++)
	{
		const CSMWorld::Record<ESM::Miscellaneous>& record = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(i);
		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);
		if (formID == 0)
		{
			formID = writer.getNextAvailableFormID();
			writer.reserveFormID(formID, strEDID);
		}
	}
	return mActiveRefCount;
}
void CSMDoc::ExportMiscCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "MISC";
	std::string searchString;
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	const CSMWorld::Record<ESM::Miscellaneous> miscRecord = mDocument.getData().getReferenceables().getDataSet().getMiscellaneous().mContainer.at(stage);

	std::string strEDID = writer.generateEDIDTES4(miscRecord.get().mId);
	uint32_t formID = writer.crossRefStringID(strEDID, false);

	bool exportOrSkip=false;
	if (mSkipMasterRecords)
	{
		exportOrSkip = miscRecord.isModified() || miscRecord.isDeleted() || 
			(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
	}
	else
	{
		exportOrSkip = true;
	}

	// Put Keys in separate collection
	if (exportOrSkip)
	{
		bool bIsKey = miscRecord.get().mData.mIsKey;
		if (bIsKey == false)
		{
			searchString = Misc::StringUtils::lowerCase(miscRecord.get().mId);
			if (searchString.find("key", 0) != searchString.npos)
			{
				bIsKey = true;
			}
			else
			{
				searchString = Misc::StringUtils::lowerCase(miscRecord.get().mName);
				if (searchString.find("key", 0) != searchString.npos)
					bIsKey = true;
			}
		}

		if (bIsKey)
		{
			// add to KeyCollection, and skip
			mState.mKeyFromMiscList.push_back(stage);
			exportOrSkip = false;
		}
	}
	// Put SoulGems in separate collection too
	if (exportOrSkip)
	{
		bool bIsSoulGem = false;
		searchString = Misc::StringUtils::lowerCase(miscRecord.get().mId);
		if (searchString.find("soulgem", 0) != searchString.npos)
		{
			bIsSoulGem = true;
		}
		else
		{
			searchString = Misc::StringUtils::lowerCase(miscRecord.get().mName);
			if (searchString.find("soulgem", 0) != searchString.npos)
				bIsSoulGem = true;
		}

		if (bIsSoulGem)
		{
			// add to KeyCollection, and skip
			mState.mSoulgemFromMiscList.push_back(stage);
			exportOrSkip = false;
		}
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

	if (stage == mActiveRefCount-1)
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
	ESM::ESMWriter& writer = mState.getWriter();

	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getLights().getSize();
	for (int i=0; i < mActiveRefCount; i++)
	{
		const CSMWorld::Record<ESM::Light>& record =  mDocument.getData().getReferenceables().getDataSet().getLights().mContainer.at(i);
		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);
		if (formID == 0)
		{
			formID = writer.getNextAvailableFormID();
			writer.reserveFormID(formID, strEDID);
		}
	}

	return mActiveRefCount;
}
void CSMDoc::ExportLightCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "LIGH";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getLights().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().getSize();
	ESM::ESMWriter& writer = mState.getWriter();
	int formID=0;

	for (int i=0; i < mActiveRefCount; i++)
	{
//		formID = writer.reserveFormID(formID, mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().mContainer.at(i).get().mId);
		const CSMWorld::Record<ESM::ItemLevList>& record = mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().mContainer.at(i);
		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);
		if (formID == 0)
		{
			formID = writer.getNextAvailableFormID();
			writer.reserveFormID(formID, strEDID);
		}
	}

	return mActiveRefCount;
}
void CSMDoc::ExportLeveledItemCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "LVLI";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getItemLevelledList().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getIngredients().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportIngredientCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "INGR";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getIngredients().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
	{
		writer.endGroupTES4(sSIG);
	}
}

CSMDoc::ExportFloraCollectionTES4Stage::ExportFloraCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportFloraCollectionTES4Stage::setup()
{
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getContainers().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportFloraCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "FLOR";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getContainers().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	const CSMWorld::Record<ESM::Container> containerRecord = mDocument.getData().getReferenceables().getDataSet().getContainers().mContainer.at(stage);
	if ((containerRecord.get().mFlags & ESM::Container::Flags::Organic) != 0)
	{
		std::string strEDID = writer.generateEDIDTES4(containerRecord.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);

		bool exportOrSkip=false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = containerRecord.isModified() || containerRecord.isDeleted() || 
				(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
/*
			std::string strEDID = writer.generateEDIDTES4(containerRecord.get().mId);
			uint32_t formID = writer.crossRefStringID(strEDID);
			uint32_t flags=0;
			if (containerRecord.mState == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x800; // DISABLED
			writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
			StartModRecord(sSIG, containerRecord.get().mId, writer, containerRecord.mState);
			containerRecord.get().exportTESx(writer, 4);
			writer.endRecordTES4(sSIG);
		}
	}

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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getContainers().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportContainerCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "CONT";
	ESM::ESMWriter& writer = mState.getWriter();

	// GRUP
	if (stage == 0)
	{
		writer.startGroupTES4(sSIG, 0);
	}

//	mDocument.getData().getReferenceables().getDataSet().getContainers().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	const CSMWorld::Record<ESM::Container> containerRecord = mDocument.getData().getReferenceables().getDataSet().getContainers().mContainer.at(stage);
	if ((containerRecord.get().mFlags & ESM::Container::Flags::Organic) == 0)
	{
		std::string strEDID = writer.generateEDIDTES4(containerRecord.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);
		bool exportOrSkip=false;
		if (mSkipMasterRecords)
		{
			exportOrSkip = containerRecord.isModified() || containerRecord.isDeleted() || 
				(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
		}
		else
		{
			exportOrSkip = true;
		}

		if (exportOrSkip)
		{
/*
			std::string strEDID = writer.generateEDIDTES4(containerRecord.get().mId);
			uint32_t formID = writer.crossRefStringID(strEDID);
			uint32_t flags=0;
			if (containerRecord.mState == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x800; // DISABLED
			writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
			StartModRecord(sSIG, containerRecord.get().mId, writer, containerRecord.mState);
			containerRecord.get().exportTESx(writer, 4);
			writer.endRecordTES4(sSIG);
		}
	}

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getClothing().getSize();
	return mActiveRefCount;
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

//	mDocument.getData().getReferenceables().getDataSet().getClothing().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	bool exportOrSkip=false;
	const CSMWorld::Record<ESM::Clothing> clothingRecord = mDocument.getData().getReferenceables().getDataSet().getClothing().mContainer.at(stage);

	if (mSkipMasterRecords == true)
	{
		// check for modified / deleted state, otherwise skip
		exportOrSkip = clothingRecord.isModified() || clothingRecord.isDeleted();
	}
	else {
		// no skipping, export all
		exportOrSkip=true;
	}

	// filter out left glove
	if ((exportOrSkip) && (clothingRecord.get().mData.mType == ESM::Clothing::Type::LGlove))
		exportOrSkip = false;

	if (exportOrSkip)
	{
/*
		std::string strEDID = writer.generateEDIDTES4(clothingRecord.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID);
		uint32_t flags=0;
		if (clothingRecord.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, strEDID);
*/
		StartModRecord(sSIG, clothingRecord.get().mId, writer, clothingRecord.mState);
		clothingRecord.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1)
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
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getBooks().getSize();
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

//	mDocument.getData().getReferenceables().getDataSet().getBooks().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	ESM::Region region = mDocument.getData().getRegions().getNthRecord(stage).get();
	std::string strEDID = writer.generateEDIDTES4(region.mId) + "Clmt";
	uint32_t formID = writer.crossRefStringID(strEDID, false);
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getBooks().getSize();
	return mActiveRefCount;
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

	mDocument.getData().getReferenceables().getDataSet().getBooks().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getArmors().getSize();
	return mActiveRefCount;
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

//	mDocument.getData().getReferenceables().getDataSet().getArmors().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	bool exportOrSkip=false;
	const CSMWorld::Record<ESM::Armor> armorRecord = mDocument.getData().getReferenceables().getDataSet().getArmors().mContainer.at(stage);

	if (mSkipMasterRecords == true)
	{
		// check for modified / deleted state, otherwise skip
		exportOrSkip = armorRecord.isModified() || armorRecord.isDeleted();
	}
	else {
		// no skipping, export all
		exportOrSkip=true;
	}

	if (exportOrSkip)
	{
		switch (armorRecord.get().mData.mType)
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
/*
		uint32_t formID = writer.crossRefStringID(armorRecord.get().mId);
		uint32_t flags=0;
		if (armorRecord.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, armorRecord.get().mId);
*/
		StartModRecord(sSIG, armorRecord.get().mId, writer, armorRecord.mState);
		armorRecord.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getApparati().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportApparatusCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "APPA";

	// GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4(sSIG, 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getApparati().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getPotions().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportPotionCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::string sSIG = "ALCH";

	// GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4(sSIG, 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getPotions().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getActivators().getSize();
	return mActiveRefCount;
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

//	mDocument.getData().getReferenceables().getDataSet().getActivators().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	bool exportOrSkip=false;
	const CSMWorld::Record<ESM::Activator> activatorRec = mDocument.getData().getReferenceables().getDataSet().getActivators().mContainer.at(stage);
	std::string strEDID = writer.generateEDIDTES4(activatorRec.get().mId);
	uint32_t formID = writer.crossRefStringID(strEDID, false);

	if (mSkipMasterRecords == true)
	{
		// check for modified / deleted state, otherwise skip
		exportOrSkip = activatorRec.isModified() || activatorRec.isDeleted() || 
			(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
	}
	else {
		// no skipping, export all
		exportOrSkip=true;
	}

	if (exportOrSkip)
	{
		// if mName contains Bed, Chair, Bench, Hammock then add to furnList and skip
		std::string tempStr ;
		if (activatorRec.get().mName.size() > 0)
			tempStr = Misc::StringUtils::lowerCase(activatorRec.get().mName);
		else
			tempStr = Misc::StringUtils::lowerCase(activatorRec.get().mId);

		if ( (tempStr.find("bed",0) != tempStr.npos) ||
			(tempStr.find("chair", 0) != tempStr.npos) ||
			(tempStr.find("bench", 0) != tempStr.npos) ||
			(tempStr.find("hammock", 0) != tempStr.npos) ||
			(tempStr.find("stool", 0) != tempStr.npos) 
			)
		{
			exportOrSkip = false;
			mState.mFurnitureFromActivatorList.push_back(stage);
			debugstream << "Moving activator(" << stage << ") [" << tempStr << "] to furn collection (size=" << mState.mFurnitureFromActivatorList.size() << ")" << std::endl;
//			OutputDebugString(debugstream.str().c_str());
		}
	}

	if (exportOrSkip)
	{
/*
		uint32_t formID = writer.crossRefStringID(activatorRec.get().mId);
		uint32_t flags=0;
		if (activatorRec.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, activatorRec.get().mId);
*/
		StartModRecord(sSIG, activatorRec.get().mId, writer, activatorRec.mState);
		activatorRec.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().getSize();
	ESM::ESMWriter& writer = mState.getWriter();
	int formID=0;

	for (int i=0; i < mActiveRefCount; i++)
	{
//		formID = writer.reserveFormID(formID, mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i).get().mId );
//		std::cout << "export: LVLC[" << i << "] " << mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i).get().mId << " = " << formID << std::endl;
		const CSMWorld::Record<ESM::CreatureLevList>& record = mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i);
		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
		uint32_t formID = writer.crossRefStringID(strEDID, false);
		if (formID == 0)
		{
			formID = writer.getNextAvailableFormID();
			writer.reserveFormID(formID, strEDID);
		}
	}

	return mActiveRefCount;
}
void CSMDoc::ExportLeveledCreatureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// LVLC GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("LVLC", 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatures().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportCreatureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// CREA GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("CREA", 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getCreatures().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("CREA");
	}
}

CSMDoc::ExportDoorCollectionTES4Stage::ExportDoorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportDoorCollectionTES4Stage::setup()
{
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getDoors().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportDoorCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// DOOR GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("DOOR", 0);
	}

//	std::cout << "Door [" << stage << "]" << std::endl;
	mDocument.getData().getReferenceables().getDataSet().getDoors().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("DOOR");
	}
}

CSMDoc::ExportSTATCollectionTES4Stage::ExportSTATCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMasters;
}
int CSMDoc::ExportSTATCollectionTES4Stage::setup()
{
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getStatics().getSize();
	return mActiveRefCount;
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

//	mDocument.getData().getReferenceables().getDataSet().getStatics().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);
	bool exportOrSkip=false;
	const CSMWorld::Record<ESM::Static> staticRec = mDocument.getData().getReferenceables().getDataSet().getStatics().mContainer.at(stage);
	std::string strEDID = writer.generateEDIDTES4(staticRec.get().mId);
	uint32_t formID = writer.crossRefStringID(strEDID, false);

	if (mSkipMasterRecords == true)
	{
		// check for modified / deleted state, otherwise skip
		exportOrSkip = staticRec.isModified() || staticRec.isDeleted() || 
			(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
	}
	else {
		// no skipping, export all
		exportOrSkip=true;
	}

	if (exportOrSkip)
	{
		// if mName contains Bed, Chair, Bench, Hammock then add to furnList and skip
		std::string tempStr;
		tempStr = Misc::StringUtils::lowerCase(staticRec.get().mId);

		if ((tempStr.find("bed", 0) != tempStr.npos) ||
			(tempStr.find("chair", 0) != tempStr.npos) ||
			(tempStr.find("bench", 0) != tempStr.npos) ||
			(tempStr.find("hammock", 0) != tempStr.npos) ||
			(tempStr.find("stool", 0) != tempStr.npos)
			)
		{
			exportOrSkip = false;
			mState.mFurnitureFromStaticList.push_back(stage);
			debugstream << "Moving static(" << stage << ") [" << tempStr << "] to furn collection (size=" << mState.mFurnitureFromStaticList.size() << ")" << std::endl;
//			OutputDebugString(debugstream.str().c_str());
		}
	}

	if (exportOrSkip)
	{
/*
		uint32_t formID = writer.crossRefStringID(staticRec.get().mId);
		uint32_t flags=0;
		if (staticRec.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DISABLED
		writer.startRecordTES4(sSIG, flags, formID, staticRec.get().mId);
*/
		StartModRecord(sSIG, staticRec.get().mId, writer, staticRec.mState);
		staticRec.get().exportTESx(writer, 4);
		writer.endRecordTES4(sSIG);
	}

	if (stage == mActiveRefCount-1)
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
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getNPCs().getSize();
	return mActiveRefCount;
}
void CSMDoc::ExportNPCCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// CREA GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("NPC_", 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getNPCs().exportTESx (stage, mState.getWriter(), mSkipMasterRecords, 4);

	if (stage == mActiveRefCount-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("NPC_");
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
//		OutputDebugString(debugstream.str().c_str());
	}

	// process a batch of 100 references in each stage
	for (int i=stage*100; i<stage*100+100 && i<size; ++i)
	{
		const CSMWorld::Record<CSMWorld::CellRef>& record =
			mDocument.getData().getReferences().getRecord (i);
//		std::string strEDID = writer.generateEDIDTES4(record.get().mId);
		std::string strEDID = record.get().mId;
		uint32_t formID = writer.crossRefStringID(strEDID, false);

		if ( record.isModified() || record.mState == CSMWorld::RecordBase::State_Deleted || 
			(formID == 0) || ((formID & 0xFF000000) > 0x01000000) )
		{
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

				tempStream << "#" << cellX << "," << cellY;
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

			bool persistentRef=false;
			CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(record.get().mRefID);

			std::string tempStr = Misc::StringUtils::lowerCase(record.get().mRefID);
			if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Npc)
				persistentRef = true;
			else if (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature)
				persistentRef = true;
			else if ((baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Door) && (record.get().mTeleport == true))
				persistentRef = true;
			else if (tempStr.find("marker") != std::string::npos)
				persistentRef = true;
	
			if (persistentRef == true)
			{
				debugstream.str(""); debugstream.clear();
				debugstream << "Collecting Persistent reference: BaseID=[" << record.get().mRefID << "] cellID=[" << cellId << "]" << std::endl;
//				OutputDebugString(debugstream.str().c_str());
				std::string tempCellReference = cellId;
				if (!interior)
					tempCellReference = "worldspace-dummycell";
				std::deque<int>& persistentRefListOfCurrentCell = mState.mPersistentRefMap[Misc::StringUtils::lowerCase (tempCellReference)];
				// reserve FormID here, then push back the pair
//				std::ostringstream refEDIDstr;
//				uint32_t formID = mState.getWriter().getNextAvailableFormID();
//				refEDIDstr << "*refindex" << i;
				if (formID == 0)
				{
					formID = writer.getNextAvailableFormID();
					formID = writer.reserveFormID(formID, strEDID);
				}
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

					tempStream << "#" << cellX << "," << cellY;
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

	if (stage == size-1)
	{
		debugstream << "Reference Lists creation complete." << std::endl;
//		OutputDebugString(debugstream.str().c_str());
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

	// look through all cells and add interior cells to appropriate subblock
	for (int i=0; i < collectionSize; i++)
	{
		CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr = &mDocument.getData().getCells().getNthRecord(i);
		bool interior = cellRecordPtr->get().mId.substr (0, 1)!="#";

		if (interior == true)
		{	
			// add to one of 100 subblocks
			std::string strEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId, 1);
			uint32_t formID = writer.crossRefStringID(strEDID, false);
//			uint32_t formID = writer.getNextAvailableFormID();
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				writer.reserveFormID(formID, strEDID);
			}
			int block = formID % 100;
			int subblock = block % 10;
			block -= subblock;
			block /= 10;
			if (block < 0 || block >= 10 || subblock < 0 || subblock >= 10)
				throw std::runtime_error ("export error: block/subblock calculation produced out of bounds index");
			Blocks[block][subblock].push_back(std::pair<uint32_t, CSMWorld::Record<CSMWorld::Cell>*>(formID, cellRecordPtr) );
//			std::pair<uint32_t, CSMWorld::Record<CSMWorld::Cell>*> newPair(formID, cellRecordPtr);
//			Blocks[block][subblock].push_back(newPair);
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
	uint32_t cellFormID;

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
		if (cellRecordPtr != 0)
			break;
	}
	if (cellRecordPtr == 0)
	{
		// throw exception
		throw std::runtime_error ("export error: cellRecordPtr uninitialized");
	}

	// check if this is the first record, to write first GRUP record
	if (stage == 0)
	{
		std::string sSIG;
		for (int i=0; i<4; ++i)
			sSIG += reinterpret_cast<const char *>(&cellRecordPtr->get().sRecordId)[i];
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
            return;
        }

        // prepare record flags
		std::string strEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId, 1);
		std::cout << "Exporting interior cell [" << strEDID << "]" << std::endl;

        uint32_t flags=0;
        if (cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted)
            flags |= 0x800; // DISABLED

        //***********EXPORT INTERIOR CELL*****************************/
        writer.startRecordTES4 (cellRecordPtr->get().sRecordId, flags, cellFormID, strEDID);
        cellRecordPtr->get().exportTES4 (writer);
        // write supplemental records requiring cross-referenced data...
        // Owner formID subrecord: XOWN
        
        // Faction rank long subrecord: XRNK (if owner is a faction)
        
        // Water formID subrecord: XCWT (if included as flag in record flags and present in cell record)
        
        // Region formID subrecord: XCLR (if present in cell record)
        
        // Cell record ends before creation of child records (which are full records and not subrecords)
        writer.endRecordTES4 (cellRecordPtr->get().sRecordId);

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

//						std::ostringstream refEDIDstr;
//						refEDIDstr << "*refindex" << *refindex_iter;
//						std::string strEDID = writer.generateEDIDTES4(refRecord.mId);
						std::string strEDID = refRecord.mId;
						uint32_t refFormID = writer.crossRefStringID(strEDID, false);
						if (refFormID == 0)
						{
							throw std::runtime_error ("Interior Cell Persistent Reference: retrieved invalid formID from *refindex lookup");
						}
						uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID);
						CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);
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
							case CSMWorld::UniversalId::Type::Type_Door:
							default:
//								if (refRecord.mTeleport != true)
//									continue;
								sSIG = "REFR";
								break;
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
							// start record
							writer.startRecordTES4(sSIG, refFlags, refFormID, strEDID);
							refRecord.exportTES4 (writer, teleportDoorRefID, &returnPosition);
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
				for (std::deque<int>::const_reverse_iterator iter(references->second.rbegin());
					iter != references->second.rend(); ++iter)
				{
					const CSMWorld::Record<CSMWorld::CellRef>& ref =
						mDocument.getData().getReferences().getRecord (*iter);

					CSMWorld::CellRef refRecord = ref.get();
	//				std::string strEDID = writer.generateEDIDTES4(refRecord.mId);
					std::string strEDID = refRecord.mId;
					uint32_t refFormID = writer.crossRefStringID(strEDID, false);
					if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
					{
	//                    uint32_t refFormID = writer.getNextAvailableFormID();
	//                    refFormID = writer.reserveFormID(refFormID, refRecord.mId);
						uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID);
						CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);	

						if (baseRefID != 0)
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
							uint32_t refFlags=0;
							if (ref.mState == CSMWorld::RecordBase::State_Deleted)
								refFlags |= 0x800; // DISABLED
							// start record
                        
							writer.startRecordTES4(sSIG, refFlags, refFormID, strEDID);
							refRecord.exportTES4 (writer);
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
			std::string strEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId, 1);
			uint32_t formID = mState.crossRefCellXY(baseX, baseY);
			if (formID == 0)
			{
				formID = writer.getNextAvailableFormID();
				formID = writer.reserveFormID(formID, strEDID);
			}
			exportData->formID = formID;
			debugstream << "formID[" << formID << "] ";

			// calculate block and subblock coordinates
			// Thanks to Zilav's forum posting for the Oblivion/Fallout Grid algorithm
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
		uint32_t wrldFormID = writer.crossRefStringID("WrldMorrowind", false);
		if (wrldFormID == 0)
			wrldFormID = writer.reserveFormID(0x01380000, "WrldMorrowind", true);
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
		uint32_t dummyCellFormID = writer.reserveFormID(0x01380001, "wrldmorrowind-dummycell", true);
		writer.startRecordTES4("CELL", flags, dummyCellFormID, "");
		writer.startSubRecordTES4("DATA");
		writer.writeT<uint8_t>(0x02); // flag: has water=0x02
		writer.endSubRecordTES4("DATA");
		writer.startSubRecordTES4("XCLC");
		writer.writeT<long>(0); // X=0
		writer.writeT<long>(0); // Y=0
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
			
//			std::ostringstream refEDIDstr;
//			refEDIDstr << "*refindex" << *refindex_iter;
//			std::string strEDID = writer.generateEDIDTES4(refRecord.mId);
			std::string strEDID = refRecord.mId;
			uint32_t refFormID = writer.crossRefStringID(strEDID, false);
			if (refFormID == 0)
			{
				throw std::runtime_error ("Exterior Cell Persistent Reference: retrieved invalid formID from *refindex lookup: " + strEDID);
			}
//			uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID);
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
						continue;
					default:
						sSIG = "REFR";
						break;
						continue;
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
				// start record
				writer.startRecordTES4(sSIG, refFlags, refFormID, strEDID);
				refRecord.exportTES4 (writer, teleportDoorRefID, &returnPosition);
				// end record
				writer.endRecordTES4(sSIG);
//				debugstream.str(""); debugstream.clear();
//				debugstream << "writing persistent record: type=[" << sSIG << "] baseEDID=[" << refRecord.mRefID << "] formID=[" << baseRefID << "]" << std::endl;
//				OutputDebugString(debugstream.str().c_str());
//				std::cout << debugstream.str();
			}
			std::cout << ".";
		} // foreach... worldspacePersistentRefList
		std::cout << "persistent world refs done..." << std::endl;
		writer.endGroupTES4(0); // (8) dummy cell's persistent children subgroup
		writer.endGroupTES4(0); // (6) dummy cell's top children group		
	}	
	//*********************END WORLD GROUP HEADER**************************/

	//********************BEGIN PROCESSING EXTERIOR CELLS*****************/
	//***************BEGIN CELL SELECTION********************/
	// Cells must be written to file in sets of contigous blocks and subblocks
	if (stage == 0)
	{
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
	std::cout << debugstream.str();

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
			generatedSubCellID << "#" << baseX+x << "," << baseY+y;

//			debugstream.str(""); debugstream.clear();
//			debugstream << "Processing OblivionCell[" << generatedSubCellID.str() << "]: ";

			std::map<std::string, std::deque<int> >::const_iterator references =
				mState.getSubRecords().find (Misc::StringUtils::lowerCase (generatedSubCellID.str()));

			bool bTempRefsPresent = false, bLandscapePresent = false;
			// check if child records are present (temp refs)
			if ( references != mState.getSubRecords().end() && references->second.empty() != true ) 
			{
//				debugstream << "refs found...";
				bTempRefsPresent = true;
			}
			// landscape record present
			std::ostringstream landID;
			landID << "#" << (baseX/2) << " " << (baseY/2);
			if (mDocument.getData().getLand().searchId(landID.str()) != -1 )
			{
//				debugstream << "landscape data found...";
				bLandscapePresent = true;
			}
//			debugstream << std::endl;
//			OutputDebugString(debugstream.str().c_str());
//			std::cout << debugstream.str();
//			debugstream.str(""); debugstream.clear();

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

				std::string strEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId, 1);
				if (subCell > 0)
				{
					// request new formID, so subcell siblings don't share a single formID
//					cellFormID = writer.reserveFormID(cellFormID, cellRecordPtr->get().mId);
//					std::string strEDID = writer.generateEDIDTES4(cellRecordPtr->get().mId);
//					cellFormID = writer.getNextAvailableFormID();
					cellFormID = mState.crossRefCellXY(baseX+x, baseY+y);
					if (cellFormID == 0)
					{
						cellFormID = writer.reserveFormID(cellFormID, strEDID);
					}
				}
				// ********************EXPORT SUBCELL HERE **********************
				flags = 0;
				if (cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted)
					flags |= 0x800; // DO NOT USE DELETED FLAG, USE DISABLED INSTEAD
				writer.startRecordTES4 (cellRecordPtr->get().sRecordId, flags, cellFormID, strEDID);
				cellRecordPtr->get().exportSubCellTES4 (writer, baseX+x, baseY+y, subCell++);

				// crossRef Region stringID to formID to make XCLR subrecord
				std::string strREGN = writer.generateEDIDTES4(cellRecordPtr->get().mRegion, 2);
				uint32_t regnID = writer.crossRefStringID(strREGN, false);
				if (regnID == 0)
				{
					strREGN = writer.generateEDIDTES4(cellRecordPtr->get().mRegion, 0);
					regnID = writer.crossRefStringID(strREGN, false);
				}
				if (regnID != 0)
				{
					writer.startSubRecordTES4("XCLR");
					writer.writeT<uint32_t>(regnID);
					writer.endSubRecordTES4("XCLR");
				}
				// Cell record ends before creation of child records (which are full records and not subrecords)
				writer.endRecordTES4 (cellRecordPtr->get().sRecordId);

				// TODO: cross-reference Region string with formID

				// Create Cell children group
				writer.startGroupTES4(cellFormID, 6); // top Cell Children Group
				// TODO: create VWD group, add VWD refs

				//******************TEMPORARY CHILDREN*****************************
				writer.startGroupTES4(cellFormID, 9); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children

//				debugstream.str(""); debugstream.clear();
//				debugstream << "retrieving landID=[" << landID.str() << "] ...";
				//******************EXPORT LANDSCAPE*****************/
				if (bLandscapePresent && cellRecordPtr->isModified())
				{
					int landIndex = mDocument.getData().getLand().getIndex(landID.str());
//					debugstream << "ID retrieved.  exporting land ... ";
					uint32_t landFormID = mState.crossRefLandXY(baseX+x, baseY+y);
					writer.startRecordTES4("LAND", 0, landFormID, "");
					mDocument.getData().getLand().getRecord(landIndex).get().exportSubCellTES4(writer, x, y);
	//					int plugindex = mDocument.getData().getLand().getRecord(landIndex).get().mPlugin;

					// VTEX, LTEX formIDs (each morroblivion subcell maps to 8x8 portion of the original 16x16 morrowind cell vtex grid)
					// each Subcell is further divided into quadrants containing a 4x4 portion of the original 16x16 morrowind cell vtex grid
					const ESM::Land::LandData *landData = mDocument.getData().getLand().getRecord(landIndex).get().getLandData(ESM::Land::DATA_VTEX);
					if (landData != 0)
					{
						uint32_t texformID;
						int i, j, u, v, quadVal=0;
						for (v=0; v < 2; v++)
						{
							for (u=0; u < 2; u++)
							{

								// After exporting landscape heightmaps...
								// 1. Create BlendMap
								createPreBlendMap(writer, (baseX/2), (baseY/2), x, y, u, v);
								// 2. Create Layer List (from all textures in Blendmap)
								gatherPreBlendTextureList();
								if (mPreBlendTextureList.size() > 0)
								{
									// 3. Choose Base Texture
									doStuff4(writer, quadVal);
									// 4. Run interpolation algorithm to generate layers
									doStuff5(writer, quadVal);
								}

								//*******************************************************/
	/*
								// create texture list for subcell quadrant
								gatherSubCellQuadrantLTEX(x, y, u, v, landData, plugindex);
								if (mSubCellQuadTexList.size() == 0)
									continue;

								// export first texture as the base layer
								int16_t layer = -1;
								auto tex_iter = mSubCellQuadTexList.begin();
								texformID = *tex_iter;
								writer.startSubRecordTES4("BTXT");
								writer.writeT<uint32_t>(texformID); // formID
								writer.writeT<uint8_t>(quadVal); // quadrant
								writer.writeT<uint8_t>(0); // unused
								writer.writeT<int16_t>(layer); // 16bit layer
								writer.endSubRecordTES4("BTXT");
	//								debugstream << "writing ref-LTEX=[" << texformID << "]... ";

								// iterate through the remaining textures, exporting each as a separate layer
								for (tex_iter++; tex_iter !=  mSubCellQuadTexList.end(); tex_iter++)
								{
									layer++;
									texformID = *tex_iter;
									writer.startSubRecordTES4("ATXT");
									writer.writeT<uint32_t>(texformID); // formID
									writer.writeT<uint8_t>(quadVal); // quadrant
									writer.writeT<uint8_t>(0); // unused
									writer.writeT<int16_t>(layer); // 16bit layer
									writer.endSubRecordTES4("ATXT");

									// create an opacity map for this layer, then export it as VTXT record
									uint16_t position=0;
									float opacity=1.0f;
									writer.startSubRecordTES4("VTXT");
									calculateTexLayerOpacityMap(x, y, u, v, landData, plugindex, texformID);
									for (auto map_pos = mTexLayerOpacityMap.begin(); map_pos != mTexLayerOpacityMap.end(); map_pos++)
									{
										position = map_pos->first;
										opacity = map_pos->second;
										writer.writeT<uint16_t>(position); // offset into 17x17 grid
										writer.writeT<uint8_t>(0); // unused
										writer.writeT<uint8_t>(0); // unused
										writer.writeT<float>(opacity); // float opacity
									}
									writer.endSubRecordTES4("VTXT");
								}
	*/
								// ************************************************************/
								// update the quadrant number for next pass
								quadVal++;

	/*
								writer.startSubRecordTES4("VTEX");
								// multiple formIDs
								writer.writeT<uint32_t>(texformID); // formID
								writer.endSubRecordTES4("VTEX");
	*/

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
				
				// TODO: export PATHGRID
				
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
//						std::string strEDID = writer.generateEDIDTES4(refRecord.mId);
						std::string strEDID = refRecord.mId;
						uint32_t refFormID = writer.crossRefStringID(strEDID, false);

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
							uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID);
							CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);
	/*
							if ((baseRefID != 0) && ( (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_CreatureLevelledList) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Npc) ) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Static) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Door) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Activator) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Potion) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Apparatus) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Armor) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Book) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Clothing) ||
								(baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Container) )
	*/
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
								// start record
								writer.startRecordTES4(sSIG, refFlags, refFormID, strEDID);
								refRecord.exportTES4 (writer);
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

void CSMDoc::ExportExteriorCellCollectionTES4Stage::doStuff5(ESM::ESMWriter& writer, int quadVal)
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

void CSMDoc::ExportExteriorCellCollectionTES4Stage::doStuff4(ESM::ESMWriter& writer, int quadVal)
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

	} // if (getLandDataFromXY(origX, origY, plugindex, landData) == true)
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
		plugindex = mDocument.getData().getLand().getRecord(landIndex).get().mPlugin;
		landData = mDocument.getData().getLand().getRecord(landIndex).get().getLandData(ESM::Land::DATA_VTEX);
		if (landData != 0)
			return true;
		else
		{
			debugstream << "ERROR: getLandDataFromXY - no vtex data" << std::endl;
//			OutputDebugString(debugstream.str().c_str());
//			throw std::runtime_error("ERROR: getLandDataFromXY - no vtex data");
		}
	}
	else
	{
		debugstream << "ERROR: getLandDataFromXY - couldn't find landID" << std::endl;
//		OutputDebugString(debugstream.str().c_str());
//		throw std::runtime_error("ERROR: getLandDataFromXY - couldn't find landID");
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
		OutputDebugString(debugstream.str().c_str());
		throw std::runtime_error(debugstream.str().c_str());
	}
	int texindex = landData->mTextures[(yoffset*16)+xoffset]-1;
	if (texindex == -1)
	{
		// using locally defined defaultTex above
		mPreBlendMap[outputX][outputY] = defaultTex;
		return; 
	}
	auto lookup = mState.mLandTexLookup_Plugin_Index[plugindex].find(texindex);
	if (lookup == mState.mLandTexLookup_Plugin_Index[plugindex].end())
	{
		// try again with plugindex==0
		lookup = mState.mLandTexLookup_Plugin_Index[0].find(texindex);
		if (lookup == mState.mLandTexLookup_Plugin_Index[0].end())
		{
			debugstream << "ERROR: drawPreBlendMapXY - couldn't resolve texindex=" << texindex << ", using defaultTexture." << std::endl;
			OutputDebugString(debugstream.str().c_str());
			// just use defaultTex to continue gracefully instead of crashing
			mPreBlendMap[outputX][outputY] = defaultTex;
			return;
//			throw std::runtime_error("ERROR: drawPreBlendMapXY - couldn't resolve texindex");
		}
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
					throw std::runtime_error("ERROR: calculateTexLayerOpacityMap - couldn't resolve texindex");
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
	SavingState& state, bool skipMaster)
	: mDocument (document), mState (state)
{
	mSkipMasterRecords = skipMaster;
}

int CSMDoc::ExportLandTextureCollectionTES4Stage::setup()
{
	mActiveRefCount =  mDocument.getData().getLandTextures().getSize();

	return mActiveRefCount;
}

void CSMDoc::ExportLandTextureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	std::ostringstream debugstream;
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<CSMWorld::LandTexture>& landTexture =
		mDocument.getData().getLandTextures().getRecord (stage);

	// LTEX GRUP
	if (stage == 0)
	{
		writer.startGroupTES4("LTEX", 0);
	}

	CSMWorld::LandTexture record = landTexture.get();

	// create separate lookup tables for each plugin loaded
	std::string strEDID = writer.generateEDIDTES4(record.mId);
	// TODO: substitute Morroblivion EDID
	strEDID = writer.substituteMorroblivionEDID(strEDID, ESM::REC_LTEX);
	uint32_t formID = writer.crossRefStringID(strEDID, false);

	bool bExportRecord = false;
	if (mSkipMasterRecords)
	{
//		bExportRecord = landTexture.isModified() || landTexture.isDeleted() ||
//			(formID == 0) || ((formID & 0xFF000000) > 0x01000000);
		bExportRecord = false;
//		bExportRecord |= landTexture.isModified();
		bExportRecord |= landTexture.isDeleted();
		bExportRecord |= (formID == 0);
		bExportRecord |= ( (formID & 0xFF000000) > 0x01000000 );
		std::cout << "LTEX:" << strEDID << "[" << std::hex << formID << "] bExportRecord=" << std::boolalpha << bExportRecord << std::endl;
	}
	else
	{
		bExportRecord = true;
	}

	if (formID != 0 && writer.mUniqueIDcheck.find(formID) != writer.mUniqueIDcheck.end())
	{
		// formID already used in another record, don't worry this may be a one(Morroblivion)-to-many(Morrowind) mapping
		// just go ahead and skip to mapping index
		bExportRecord = false;
		std::cout << "record already written, skipping ahead." << std::endl;
	}

	if (bExportRecord)
	{
		if (formID == 0)
		{
			// reserve new formID
			formID = writer.getNextAvailableFormID();
			formID = writer.reserveFormID(formID, strEDID);
		}

		uint32_t flags=0;
		if (landTexture.mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x800; // DO NOT USE DELETED, USE DISABLED
		writer.startRecordTES4("LTEX", flags, formID, strEDID);
		record.exportTESx (writer, true, 4);
		writer.endRecordTES4 ("LTEX");
	}

	if (formID != 0)
	{
		// create lookup table for TextureIndex
		mState.mLandTexLookup_Plugin_Index[record.mPluginIndex][record.mIndex] = formID;
		debugstream << "INDEXED: (plugin=" << record.mPluginIndex << ") texindex=" << record.mIndex << " formid=[" << formID << "] mID=" << record.mId << std::endl;
		//		OutputDebugString(debugstream.str().c_str());
	}

	if (stage == mActiveRefCount-1)
	{
		writer.endGroupTES4("LTEX");
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

void CSMDoc::FinalizeExportTES4Stage::perform (int stage, Messages& messages)
{
	if (mState.hasError())
	{
		mState.getWriter().updateTES4();
		mState.getWriter().close();
		mState.getStream().close();

		if (boost::filesystem::exists (mState.getTmpPath()))
			boost::filesystem::remove (mState.getTmpPath());
	}
	else if (!mState.isProjectFile())
	{
		if (boost::filesystem::exists (mState.getPath()))
			boost::filesystem::remove (mState.getPath());

		boost::filesystem::rename (mState.getTmpPath(), mState.getPath());

		mDocument.getUndoStack().setClean();
	}

	std::cout << std::endl << "Export Complete!" << std::endl;
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
			teleportDoorRefID = writer.crossRefStringID(strEDID, false);
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
	std::string strEDID = esm.generateEDIDTES4(mId);
	uint32_t formID = esm.crossRefStringID(strEDID, false);
	uint32_t flags=0;
	if (state == CSMWorld::RecordBase::State_Deleted)
		flags |= 0x800; // DISABLED
	esm.startRecordTES4(sSIG, flags, formID, strEDID);
}

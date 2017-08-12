#include <iostream>
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

	// Export to ESM file
	appendStage (new OpenExportTES4Stage (currentDoc, currentSave, true));

	appendStage (new ExportHeaderTES4Stage (currentDoc, currentSave, false));

//	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Global> >(mDocument.getData().getGlobals(), currentSave));

/*
	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::GameSetting> >
		(mDocument.getData().getGmsts(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Skill> >
		(mDocument.getData().getSkills(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Class> >
		(mDocument.getData().getClasses(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Faction> >
		(mDocument.getData().getFactions(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Race> >
		(mDocument.getData().getRaces(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Sound> >
		(mDocument.getData().getSounds(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Script> >
		(mDocument.getData().getScripts(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Region> >
		(mDocument.getData().getRegions(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BirthSign> >
		(mDocument.getData().getBirthsigns(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Spell> >
		(mDocument.getData().getSpells(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Enchantment> >
		(mDocument.getData().getEnchantments(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BodyPart> >
		(mDocument.getData().getBodyParts(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::SoundGenerator> >
		(mDocument.getData().getSoundGens(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::MagicEffect> >
		(mDocument.getData().getMagicEffects(), currentSave));

	mExportOperation->appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::StartScript> >
		(mDocument.getData().getStartScripts(), currentSave));

	// Dialogue can reference objects and cells so must be written after these records for vanilla-compatible files

	mExportOperation->appendStage (new ExportDialogueCollectionTES4Stage (mDocument, currentSave, false));

	mExportOperation->appendStage (new ExportDialogueCollectionTES4Stage (mDocument, currentSave, true));

	mExportOperation->appendStage (new ExportPathgridCollectionTES4Stage (mDocument, currentSave));

	mExportOperation->appendStage (new ExportLandTextureCollectionTES4Stage (mDocument, currentSave));

	// references Land Textures
	mExportOperation->appendStage (new ExportLandCollectionTES4Stage (mDocument, currentSave));
*/

	appendStage (new ExportNPCCollectionTES4Stage (currentDoc, currentSave));
	appendStage (new ExportCreaturesCollectionTES4Stage (currentDoc, currentSave));
	appendStage (new ExportLeveledCreaturesCollectionTES4Stage (currentDoc, currentSave));

	appendStage (new ExportReferenceCollectionTES4Stage (currentDoc, currentSave));
	appendStage (new ExportInteriorCellCollectionTES4Stage (currentDoc, currentSave));

	appendStage (new ExportExteriorCellCollectionTES4Stage (currentDoc, currentSave));

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
	mState.getWriter().clearReservedFormIDs();
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

		for (std::vector<boost::filesystem::path>::const_iterator iter (dependencies.begin());
			iter!=end; ++iter)
		{
			std::string name = iter->filename().string();
			uint64_t size = boost::filesystem::file_size (*iter);

//			mState.getWriter().addMaster (name, size);
		}
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
	// HACK: hardcoded to add Creatures
//	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatures().getSize();
	return mActiveRefCount;
}

void CSMDoc::ExportRefIdCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// HACK: hardcoded for LVLC
	// LVLC GRUP
	if (stage == 0)
	{
//		ESM::ESMWriter& writer = mState.getWriter();
//		writer.startGroupTES4("LVLC", 0);
	}

	mDocument.getData().getReferenceables().exportTESx (stage, mState.getWriter(), 4);
//	mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().exportTESx (stage, mState.getWriter(), 4);

	if (stage == mActiveRefCount-1)
	{
//		ESM::ESMWriter& writer = mState.getWriter();
//		writer.endGroupTES4("LVLC");
	}
}

CSMDoc::ExportLeveledCreaturesCollectionTES4Stage::ExportLeveledCreaturesCollectionTES4Stage (Document& document, SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportLeveledCreaturesCollectionTES4Stage::setup()
{
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().getSize();
	ESM::ESMWriter& writer = mState.getWriter();
	int formID=0;

	for (int i=0; i < mActiveRefCount; i++)
	{
		formID = writer.reserveFormID(formID, mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i).get().mId );
//		std::cout << "export: LVLC[" << i << "] " << mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().mContainer.at(i).get().mId << " = " << formID << std::endl;
	}

	return mActiveRefCount;
}

void CSMDoc::ExportLeveledCreaturesCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// LVLC GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("LVLC", 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().exportTESx (stage, mState.getWriter(), false, 4);

	if (stage == mActiveRefCount-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("LVLC");
	}
}

CSMDoc::ExportCreaturesCollectionTES4Stage::ExportCreaturesCollectionTES4Stage (Document& document, SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportCreaturesCollectionTES4Stage::setup()
{
	mActiveRefCount = mDocument.getData().getReferenceables().getDataSet().getCreatures().getSize();
	return mActiveRefCount;
}

void CSMDoc::ExportCreaturesCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// CREA GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("CREA", 0);
	}

	mDocument.getData().getReferenceables().getDataSet().getCreatures().exportTESx (stage, mState.getWriter(), false, 4);

	if (stage == mActiveRefCount-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("CREA");
	}
}

CSMDoc::ExportNPCCollectionTES4Stage::ExportNPCCollectionTES4Stage (Document& document, SavingState& state)
	: mDocument (document), mState (state)
{}

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

	mDocument.getData().getReferenceables().getDataSet().getNPCs().exportTESx (stage, mState.getWriter(), false, 4);

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

	int steps = size/100;
	if (size%100) ++steps;

	return steps;
}

void CSMDoc::ExportReferenceCollectionTES4Stage::perform (int stage, Messages& messages)
{
	int size = mDocument.getData().getReferences().getSize();

	for (int i=stage*100; i<stage*100+100 && i<size; ++i)
	{
		const CSMWorld::Record<CSMWorld::CellRef>& record =
			mDocument.getData().getReferences().getRecord (i);

		if (record.isModified() || record.mState == CSMWorld::RecordBase::State_Deleted)
		{
			std::string cellId = record.get().mOriginalCell.empty() ?
				record.get().mCell : record.get().mOriginalCell;

			std::deque<int>& indices =
				mState.getSubRecords()[Misc::StringUtils::lowerCase (cellId)];

			// collect moved references at the end of the container
			bool interior = cellId.substr (0, 1)!="#";
			std::ostringstream stream;
			if (!interior)
			{
				// recalculate the ref's cell location
				std::pair<int, int> index = record.get().getCellIndex();
				stream << "#" << index.first << " " << index.second;
			}

			// An empty mOriginalCell is meant to indicate that it is the same as
			// the current cell.  It is possible that a moved ref is moved again.
			if ((record.get().mOriginalCell.empty() ?
				record.get().mCell : record.get().mOriginalCell) != stream.str() && !interior && record.mState!=CSMWorld::RecordBase::State_ModifiedOnly && !record.get().mNew)
				indices.push_back (i);
			else
				indices.push_front (i);
		}
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
			uint32_t formID = writer.getNextAvailableFormID();
			formID = writer.reserveFormID(formID, cellRecordPtr->get().mId);
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
		writer.startGroupTES4(0, 2);
		// initialize first subblock
		writer.startGroupTES4(0, 3);
		// document creation of first subblock
		blockInitialized[0][0] = true;
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
			writer.startGroupTES4(0, 2);
		}
		// start new subblock
		writer.startGroupTES4(0, 3);
	}
	
//============================================================================================================
// EXPORT AN INTERIOR CELL + CELL'S REFERENCES
//============================================================================================================

	// load the pre-generated SubRecord list of references for this cell
	std::map<std::string, std::deque<int> >::const_iterator references =
		mState.getSubRecords().find (Misc::StringUtils::lowerCase (cellRecordPtr->get().mId));

	if (cellRecordPtr->isModified() ||
        cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted ||
		references != mState.getSubRecords().end())
	{
        // count new references and adjust RefNumCount accordingly
        int newRefNum = cellRecordPtr->get().mRefNumCounter;

        if (references != mState.getSubRecords().end())
        {
            for (std::deque<int>::const_iterator iter (references->second.begin());
                iter != references->second.end(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                if (ref.get().mNew )
                    ++(cellRecordPtr->get().mRefNumCounter);
            }
        }

        // formID was previously generated in setup() method, so throw an error if it's zero
        if (cellFormID == 0)
        {
            throw std::runtime_error ("export: cellFormID is 0");
            return;
        }

        // prepare record flags
        uint32_t flags=0;
        if (cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted)
            flags |= 0x01;

        // write cell record
        writer.startRecordTES4 (cellRecordPtr->get().sRecordId, flags, cellFormID, cellRecordPtr->get().mId);
        cellRecordPtr->get().exportTES4 (writer);
        // write supplemental records requiring cross-referenced data...
        // Owner formID subrecord: XOWN
        
        // Faction rank long subrecord: XRNK (if owner is a faction)
        
        // Water formID subrecord: XCWT (if included as flag in record flags and present in cell record)
        
        // Region formID subrecord: XCLR (if present in cell record)
        
        // Cell record ends before creation of child records (which are full records and not subrecords)
        writer.endRecordTES4 (cellRecordPtr->get().sRecordId);

        // write reference records
        if (references!=mState.getSubRecords().end())
        {
            // Create Cell children group
            writer.startGroupTES4(cellFormID, 6); // top Cell Children Group
            writer.startGroupTES4(cellFormID, 9); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children

            for (std::deque<int>::const_reverse_iterator iter(references->second.rbegin());
                iter != references->second.rend(); ++iter)
            {
                const CSMWorld::Record<CSMWorld::CellRef>& ref =
                    mDocument.getData().getReferences().getRecord (*iter);

                if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
                {
                    CSMWorld::CellRef refRecord = ref.get();

                    // Check for uninitialized content file
                    if (!refRecord.mRefNum.hasContentFile())
                        refRecord.mRefNum.mContentFile = 0;

                    // recalculate the ref's cell location
                    std::ostringstream stream;

                    if (refRecord.mNew)
                        refRecord.mRefNum.mIndex = newRefNum++;

                    // reserve formID

                    uint32_t refFormID = writer.getNextAvailableFormID();
                    refFormID = writer.reserveFormID(refFormID, refRecord.mId);
                    uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID);
                    CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);	
//						if ( (baseRefID != 0) && ( (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_CreatureLevelledList) || (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature) || (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Npc) ) )
                    if ((baseRefID != 0) && ((baseRefIndex.second == CSMWorld::UniversalId::Type::Type_CreatureLevelledList) || (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature)) )
                    {
                        std::string sSIG;
                        switch (baseRefIndex.second)
                        {
//							case CSMWorld::UniversalId::Type::Type_Npc:
//								sSIG = "ACHR";
//								break;
                        case CSMWorld::UniversalId::Type::Type_Creature:
                            sSIG = "ACRE";
                            break;
                        case CSMWorld::UniversalId::Type::Type_CreatureLevelledList:
                        default:
                            sSIG = "REFR";
                            break;
                        }
                        uint32_t refFlags=0;
                        if (ref.mState == CSMWorld::RecordBase::State_Deleted)
                            refFlags |= 0x01;
                        // start record
                        
                        writer.startRecordTES4(sSIG, refFlags, refFormID, refRecord.mId);
                        refRecord.exportTES4 (writer, false, false, ref.mState == CSMWorld::RecordBase::State_Deleted);
                        // end record
                        writer.endRecordTES4(sSIG);
                    }

                }

            }
            // close cell children group
            writer.endGroupTES4(cellFormID); // cell children subgroup
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
	ESM::ESMWriter& writer = mState.getWriter();
	int collectionSize = mDocument.getData().getCells().getSize();

	// look through all cells and add interior cells to appropriate subblock
	for (int i=0; i < collectionSize; i++)
	{
		CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr = &mDocument.getData().getCells().getNthRecord(i);
		bool interior = cellRecordPtr->get().mId.substr (0, 1)!="#";

		if (interior == false)
		{
			// add to one of 100 subblocks
			uint32_t formID = writer.getNextAvailableFormID();
			formID = writer.reserveFormID(formID, cellRecordPtr->get().mId);
			int block = formID % 100;
			int subblock = block % 10;
			block -= subblock;
			block /= 10;
			if (block < 0 || block >= 10 || subblock < 0 || subblock >= 10)
				throw std::runtime_error ("export error: block/subblock calculation produced out of bounds index");
			Blocks[block][subblock].push_back(std::pair<uint32_t, CSMWorld::Record<CSMWorld::Cell>*>(formID, cellRecordPtr));
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

void CSMDoc::ExportExteriorCellCollectionTES4Stage::perform (int stage, Messages& messages)
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

	// if stage == 0, then add the group record first
	if (stage == 0)
	{
		std::string sSIG="WRLD";
		writer.startGroupTES4(sSIG, 0); // Top GROUP

		// Create WRLD record
		writer.reserveFormID(0x01380000, "WrldMorrowind");
		writer.startRecordTES4("WRLD", 0, 0x01380000, "WrldMorrowind");
		writer.startSubRecordTES4("EDID");
		writer.writeHCString("WrldMorrowind");
		writer.endSubRecordTES4("EDID");
		writer.startSubRecordTES4("FULL");
		writer.writeHCString("Morrowind");
		writer.endSubRecordTES4("FULL");
		writer.endRecordTES4("WRLD");
		
		// Create WRLD Children Group
		writer.startGroupTES4(0x01380000, 1);

		// Create CELL dummy record
		uint32_t flags=0x400;
		writer.startRecordTES4("CELL", flags, 0x01380001, "");
		writer.endRecordTES4("CELL");
		// Create CELL dummy top children group
//		writer.startGroupTES4(0x01380001, 6); // top Cell Children Group
		// Create CELL dummy persistent children group
//		writer.startGroupTES4(0x01380001, 8); // grouptype=8 (persistent children)


		// initialize first exterior Cell Block, grouptype=4; label=0xYYYYXXXX
		writer.startGroupTES4(0x00000000, 4);
		// initialize first exterior Cell SubBlock, grouptype=5; label=0xYYYYXXXX
		writer.startGroupTES4(0x00000000, 5);
		// document creation of first subblock
		blockInitialized[0][0] = true;

		// HACK for LVLC -- place all refs in single exterior CELL
		writer.startRecordTES4("CELL", 0, 0x01380002, "");
		writer.endRecordTES4("CELL");
		writer.startGroupTES4(0x01380002, 6); // top Cell Children Group
		writer.startGroupTES4(0x01380002, 9); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children

	}

	// check to see if group is initialized
	if (blockInitialized[block][subblock] == false)
	{
		blockInitialized[block][subblock] = true;
		// close previous subblock
//		writer.endGroupTES4(0);
		// if subblock == 0, then close prior block as well
		if (subblock == 0)
		{
//			writer.endGroupTES4(0);
			// start the next block if prior block was closed
//			writer.startGroupTES4(0, 4);
		}
		// start new subblock
//		writer.startGroupTES4(0, 5);
	}

	//============================================================================================================

	// load the pre-generated SubRecord list of references for this cell
	std::map<std::string, std::deque<int> >::const_iterator references =
		mState.getSubRecords().find (Misc::StringUtils::lowerCase (cellRecordPtr->get().mId));

	if (cellRecordPtr->isModified() ||
		cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted ||
		references!=mState.getSubRecords().end())
	{

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

		if (cellFormID == 0)
		{
			throw std::runtime_error ("export: cellFormID is 0");
		}
/*
		uint32_t flags=0;
		if (cellRecordPtr->mState == CSMWorld::RecordBase::State_Deleted)
			flags |= 0x01;

		writer.startRecordTES4 (cellRecordPtr->get().sRecordId, flags, cellFormID, cellRecordPtr->get().mId);
		cellRecordPtr->get().exportTES4 (writer);

		writer.startSubRecordTES4("XCLC");
		writer.writeT<uint32_t>(cellRecordPtr->get().mData.mX*2);
		writer.writeT<uint32_t>(cellRecordPtr->get().mData.mY*2);
		writer.endSubRecordTES4("XCLC");

		// Cell record ends before creation of child records (which are full records and not subrecords)
		writer.endRecordTES4 (cellRecordPtr->get().sRecordId);
*/
		// write references
		if (references!=mState.getSubRecords().end())
		{
			// Create Cell children group
//			writer.startGroupTES4(cellFormID, 6); // top Cell Children Group
//			writer.startGroupTES4(cellFormID, 9); // Cell Children Subgroup: 8 - persistent children, 9 - temporary children

			for (std::deque<int>::const_reverse_iterator iter(references->second.rbegin());
				iter != references->second.rend(); ++iter)
			{
				const CSMWorld::Record<CSMWorld::CellRef>& ref =
					mDocument.getData().getReferences().getRecord (*iter);

				if (ref.isModified() || ref.mState == CSMWorld::RecordBase::State_Deleted)
				{
					CSMWorld::CellRef refRecord = ref.get();

					// Check for uninitialized content file
					if (!refRecord.mRefNum.hasContentFile())
						refRecord.mRefNum.mContentFile = 0;

					// recalculate the ref's cell location
					std::ostringstream stream;

					if (refRecord.mNew)
					{
						refRecord.mRefNum.mIndex = newRefNum++;
					}

					// reserve formID
					uint32_t refFormID = writer.getNextAvailableFormID();
					refFormID = writer.reserveFormID(refFormID, refRecord.mId);
					uint32_t baseRefID = writer.crossRefStringID(refRecord.mRefID);
					CSMWorld::RefIdData::LocalIndex baseRefIndex = mDocument.getData().getReferenceables().getDataSet().searchId(refRecord.mRefID);
//					if ((baseRefID != 0) && ( (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_CreatureLevelledList) || (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature) || (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Npc) ) )
					if ((baseRefID != 0) && ((baseRefIndex.second == CSMWorld::UniversalId::Type::Type_CreatureLevelledList) || (baseRefIndex.second == CSMWorld::UniversalId::Type::Type_Creature)) )
					{
						std::string sSIG;
						switch (baseRefIndex.second)
						{
//						case CSMWorld::UniversalId::Type::Type_Npc:
//							sSIG = "ACHR";
//							break;
						case CSMWorld::UniversalId::Type::Type_Creature:
							sSIG = "ACRE";
							break;
						case CSMWorld::UniversalId::Type::Type_CreatureLevelledList:
						default:
							sSIG = "REFR";
							break;
						}
						uint32_t refFlags=0;
						if (ref.mState == CSMWorld::RecordBase::State_Deleted)
							refFlags |= 0x01;
						// start record

						writer.startRecordTES4(sSIG, refFlags, refFormID, refRecord.mId);
						refRecord.exportTES4 (writer, false, false, ref.mState == CSMWorld::RecordBase::State_Deleted);
						// end record
						writer.endRecordTES4(sSIG);
					}

				}

			}
			// close cell children group
//			writer.endGroupTES4(cellFormID); // cell children subgroup
//			writer.endGroupTES4(cellFormID); // 6 top cell children group
		}

	}

	if (stage == (mNumCells-1))
	{
		// <***** HACK for LVLC ****
		writer.endGroupTES4(0x01380002); // cell children subgroup
		writer.endGroupTES4(0x01380002); // 6 top cell children group
		// ***** HACK for LVLC ****>

		// two for the block-subblock
		writer.endGroupTES4(0);
		writer.endGroupTES4(0);
		// WRLD children Group
		writer.endGroupTES4(0);
		// third one is the top Group
		writer.endGroupTES4("WRLD");
	}

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
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<CSMWorld::Land>& land =
		mDocument.getData().getLand().getRecord (stage);

	if (land.isModified() || land.mState == CSMWorld::RecordBase::State_Deleted)
	{
		CSMWorld::Land record = land.get();
		writer.startRecord (record.sRecordId);
		record.save (writer, land.mState == CSMWorld::RecordBase::State_Deleted);
		writer.endRecord (record.sRecordId);
	}
}


CSMDoc::ExportLandTextureCollectionTES4Stage::ExportLandTextureCollectionTES4Stage (Document& document,
	SavingState& state)
	: mDocument (document), mState (state)
{}

int CSMDoc::ExportLandTextureCollectionTES4Stage::setup()
{
	return mDocument.getData().getLandTextures().getSize();
}

void CSMDoc::ExportLandTextureCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<CSMWorld::LandTexture>& landTexture =
		mDocument.getData().getLandTextures().getRecord (stage);

	if (landTexture.isModified() || landTexture.mState == CSMWorld::RecordBase::State_Deleted)
	{
		CSMWorld::LandTexture record = landTexture.get();
		writer.startRecord (record.sRecordId);
		record.save (writer, landTexture.mState == CSMWorld::RecordBase::State_Deleted);
		writer.endRecord (record.sRecordId);
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
}

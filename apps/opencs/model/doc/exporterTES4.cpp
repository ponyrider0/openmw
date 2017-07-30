#include <iostream>
#include <boost/filesystem.hpp>
#include <QUndoStack>

#include <components/esm/loaddial.hpp>

#include "../world/infocollection.hpp"
#include "../world/cellcoordinates.hpp"
#include "../world/data.hpp"
#include "../world/idcollection.hpp"

#include "document.hpp"
//#include "savingstages.hpp"
#include "exporterTES4.hpp"

CSMDoc::TES4Exporter::TES4Exporter(Document& document, const boost::filesystem::path exportPath, ToUTF8::FromType encoding)
  : Exporter(document, exportPath, encoding)
{
	mExportPath = exportPath.parent_path() / (exportPath.stem().string() + ".ESM4");
	std::cout << "TES4 Export Path = " << mExportPath << std::endl;
}

void CSMDoc::TES4Exporter::queryExportPath()
{
	if (mStatePtr != 0)
		delete mStatePtr;
	mStatePtr = new SavingState(mExportOperation, mExportPath, mEncoding);
}
void CSMDoc::TES4Exporter::defineExportOperation()
{

	// Export to ESM file
	mExportOperation.appendStage (new OpenExportTES4Stage (mDocument, *mStatePtr, true));

	mExportOperation.appendStage (new ExportHeaderTES4Stage (mDocument, *mStatePtr, false));

//	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Global> >(mDocument.getData().getGlobals(), *mStatePtr));

/*
	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::GameSetting> >
		(mDocument.getData().getGmsts(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Skill> >
		(mDocument.getData().getSkills(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Class> >
		(mDocument.getData().getClasses(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Faction> >
		(mDocument.getData().getFactions(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Race> >
		(mDocument.getData().getRaces(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Sound> >
		(mDocument.getData().getSounds(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Script> >
		(mDocument.getData().getScripts(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Region> >
		(mDocument.getData().getRegions(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BirthSign> >
		(mDocument.getData().getBirthsigns(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Spell> >
		(mDocument.getData().getSpells(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::Enchantment> >
		(mDocument.getData().getEnchantments(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::BodyPart> >
		(mDocument.getData().getBodyParts(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::SoundGenerator> >
		(mDocument.getData().getSoundGens(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::MagicEffect> >
		(mDocument.getData().getMagicEffects(), *mStatePtr));

	mExportOperation.appendStage (new ExportCollectionTES4Stage<CSMWorld::IdCollection<ESM::StartScript> >
		(mDocument.getData().getStartScripts(), *mStatePtr));

	// Dialogue can reference objects and cells so must be written after these records for vanilla-compatible files

	mExportOperation.appendStage (new ExportDialogueCollectionTES4Stage (mDocument, *mStatePtr, false));

	mExportOperation.appendStage (new ExportDialogueCollectionTES4Stage (mDocument, *mStatePtr, true));

	mExportOperation.appendStage (new ExportPathgridCollectionTES4Stage (mDocument, *mStatePtr));

	mExportOperation.appendStage (new ExportLandTextureCollectionTES4Stage (mDocument, *mStatePtr));

	// references Land Textures
	mExportOperation.appendStage (new ExportLandCollectionTES4Stage (mDocument, *mStatePtr));
*/

	mExportOperation.appendStage (new ExportRefIdCollectionTES4Stage (mDocument, *mStatePtr));

	mExportOperation.appendStage (new ExportReferenceCollectionTES4Stage (mDocument, *mStatePtr));

	mExportOperation.appendStage (new ExportInteriorCellCollectionTES4Stage (mDocument, *mStatePtr));

//	mExportOperation.appendStage (new ExportExteriorCellCollectionTES4Stage (mDocument, *mStatePtr));

	// close file and clean up
	mExportOperation.appendStage (new CloseExportTES4Stage (*mStatePtr));

	mExportOperation.appendStage (new FinalizeExportTES4Stage (mDocument, *mStatePtr));

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
	mState.start (mDocument, mProjectFile);

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
	return 1;
}

void CSMDoc::ExportHeaderTES4Stage::perform (int stage, Messages& messages)
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

			mState.getWriter().addMaster (name, size);
		}
	}

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
	// HACK: hardcoded for LVLC
//	return mDocument.getData().getReferenceables().getSize();
	return mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().getSize();
}

void CSMDoc::ExportRefIdCollectionTES4Stage::perform (int stage, Messages& messages)
{
	// HACK: hardcoded for LVLC
	// LVLC GRUP
	if (stage == 0)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.startGroupTES4("LVLC", 0);
	}

//	mDocument.getData().getReferenceables().exportTESx (stage, mState.getWriter(), 4);
	mDocument.getData().getReferenceables().getDataSet().getCreatureLevelledLists().exportTESx (stage, mState.getWriter(), 4);

	if (stage == mDocument.getData().getReferenceables().getSize()-1)
	{
		ESM::ESMWriter& writer = mState.getWriter();
		writer.endGroupTES4("LVLC");
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
	return mDocument.getData().getCells().getSize();
}

void CSMDoc::ExportInteriorCellCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<CSMWorld::Cell>& cell = mDocument.getData().getCells().getRecord (stage);

	std::map<std::string, std::deque<int> >::const_iterator references =
		mState.getSubRecords().find (Misc::StringUtils::lowerCase (cell.get().mId));

	// if stage == 0, then add the group record first
	if (stage == 0)
	{
		std::string sSIG;
		for (int i=0; i<4; ++i)
			sSIG += reinterpret_cast<const char *> (&cell.get().sRecordId)[i];
		writer.startGroupTES4(sSIG, 0); // Top GROUP

		// HACK: create one block-subblock for all cells
		writer.startGroupTES4(0, 2);
		writer.startGroupTES4(0, 3);
	}

	if (cell.isModified() ||
		cell.mState == CSMWorld::RecordBase::State_Deleted ||
		references!=mState.getSubRecords().end())
	{
		CSMWorld::Cell cellRecord = cell.get();
		bool interior = cellRecord.mId.substr (0, 1)!="#";

		if (interior == true)
		{

			// count new references and adjust RefNumCount accordingsly
			int newRefNum = cellRecord.mRefNumCounter;

			if (references!=mState.getSubRecords().end())
			{
				for (std::deque<int>::const_iterator iter (references->second.begin());
					iter!=references->second.end(); ++iter)
				{
					const CSMWorld::Record<CSMWorld::CellRef>& ref =
						mDocument.getData().getReferences().getRecord (*iter);

					if (ref.get().mNew )
						++cellRecord.mRefNumCounter;
				}
			}

			// write cell data
			uint32_t cellFormID=writer.getNextAvailableFormID();
			writer.reserveFormID(cellFormID);
			uint32_t flags=0;
			if (cell.mState == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x01;

			writer.startRecordTES4 (cellRecord.sRecordId, flags, cellFormID);
			cellRecord.exportTES4 (writer);
			// Cell record ends before creation of child records (which are full records and not subrecords)
			writer.endRecord (cellRecord.sRecordId);

			// write references
			if (references!=mState.getSubRecords().end())
			{
				// Create Cell children group
				writer.startGroupTES4(cellFormID, 6);

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
						uint32_t refFormID=writer.getNextAvailableFormID();
						writer.reserveFormID(refFormID);
						uint32_t refFlags=0;
						if (ref.mState == CSMWorld::RecordBase::State_Deleted)
							refFlags |= 0x01;
						// start record
						writer.startRecordTES4("REFR", refFlags, refFormID);
						refRecord.exportTES4 (writer, false, false, ref.mState == CSMWorld::RecordBase::State_Deleted);
						// end record
						writer.endRecordTES4("REFR");
					}
				}

				// close cell children group
				writer.endGroupTES4(cellFormID);
			}

		}
	}

	if (stage == (mDocument.getData().getCells().getSize()-1))
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
	return mDocument.getData().getCells().getSize();
}

void CSMDoc::ExportExteriorCellCollectionTES4Stage::perform (int stage, Messages& messages)
{
	ESM::ESMWriter& writer = mState.getWriter();
	const CSMWorld::Record<CSMWorld::Cell>& cell = mDocument.getData().getCells().getRecord (stage);

	std::map<std::string, std::deque<int> >::const_iterator references =
		mState.getSubRecords().find (Misc::StringUtils::lowerCase (cell.get().mId));

	if (cell.isModified() ||
		cell.mState == CSMWorld::RecordBase::State_Deleted ||
		references!=mState.getSubRecords().end())
	{
		CSMWorld::Cell cellRecord = cell.get();
		bool interior = cellRecord.mId.substr (0, 1)!="#";

		if (interior == false)
		{

			// count new references and adjust RefNumCount accordingsly
			int newRefNum = cellRecord.mRefNumCounter;

			if (references!=mState.getSubRecords().end())
			{
				for (std::deque<int>::const_iterator iter (references->second.begin());
					iter!=references->second.end(); ++iter)
				{
					const CSMWorld::Record<CSMWorld::CellRef>& ref =
						mDocument.getData().getReferences().getRecord (*iter);

					if (ref.get().mNew ||
						(!interior && ref.mState==CSMWorld::RecordBase::State_ModifiedOnly &&
							/// \todo consider worldspace
							CSMWorld::CellCoordinates (ref.get().getCellIndex()).getId("")!=ref.get().mCell))
						++cellRecord.mRefNumCounter;
				}
			}

			// write cell data
			writer.startRecord (cellRecord.sRecordId);

			if (interior)
				cellRecord.mData.mFlags |= ESM::Cell::Interior;
			else
			{
				cellRecord.mData.mFlags &= ~ESM::Cell::Interior;

				std::istringstream stream (cellRecord.mId.c_str());
				char ignore;
				stream >> ignore >> cellRecord.mData.mX >> cellRecord.mData.mY;
			}

			cellRecord.exportTES3 (writer, cell.mState == CSMWorld::RecordBase::State_Deleted);

			// write references
			if (references!=mState.getSubRecords().end())
			{
				//			for (std::deque<int>::const_iterator iter (references->second.begin());
				//				iter!=references->second.end(); ++iter)
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
						if (!interior)
						{
							std::pair<int, int> index = refRecord.getCellIndex();
							stream << "#" << index.first << " " << index.second;
						}

						if (refRecord.mNew ||
							(!interior && ref.mState==CSMWorld::RecordBase::State_ModifiedOnly &&
								refRecord.mCell!=stream.str()))
						{
							refRecord.mRefNum.mIndex = newRefNum++;
						}
						else if ((refRecord.mOriginalCell.empty() ? refRecord.mCell : refRecord.mOriginalCell)
							!= stream.str() && !interior)
						{
							// An empty mOriginalCell is meant to indicate that it is the same as
							// the current cell.  It is possible that a moved ref is moved again.

							ESM::MovedCellRef moved;
							moved.mRefNum = refRecord.mRefNum;

							// Need to fill mTarget with the ref's new position.
							std::istringstream istream (stream.str().c_str());

							char ignore;
							istream >> ignore >> moved.mTarget[0] >> moved.mTarget[1];

							refRecord.mRefNum.save (writer, false, "MVRF");
							writer.writeHNT ("CNDT", moved.mTarget, 8);
						}

						refRecord.exportTES3 (writer, false, false, ref.mState == CSMWorld::RecordBase::State_Deleted);
					}
				}
			}

			writer.endRecord (cellRecord.sRecordId);
		}
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
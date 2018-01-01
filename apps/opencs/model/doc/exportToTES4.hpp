#ifndef CSM_DOC_EXPORT_TO_TES4_H
#define CSM_DOC_EXPORT_TO_TES4_H

#include "../world/record.hpp"
#include "../world/idcollection.hpp"
#include "../world/scope.hpp"
#include "../world/infoselectwrapper.hpp"

#include <components/esm/defs.hpp>
#include <components/esm/loadland.hpp>

//#include <components/esm/loaddial.hpp>
//#include "../world/infocollection.hpp"
//#include "../world/cellcoordinates.hpp"
//#include "../world/data.hpp"
//#include "components/esm/loadlevlist.hpp"
#include "savingstate.hpp"
#include "stage.hpp"

#include "exportToBase.hpp"

namespace ESM
{
    struct Dialogue;
}

namespace CSMWorld
{
	struct CellRef;
    struct Cell;
    class InfoCollection;
	class RefIdCollection;
}

namespace CSMDoc
{
	uint32_t FindSiblingDoor(Document& mDocument, SavingState& mState, CSMWorld::CellRef& refRecord, uint32_t refFormID, ESM::Position& returnPosition);
	void StartModRecord(const std::string& sSIG, const std::string& mId, ESM::ESMWriter& esm, const CSMWorld::RecordBase::State& state);

    class Document;
    class SavingState;
    
    class ExportToTES4 : public ExportToBase
    {
	public:
        ExportToTES4();
		void defineExportOperation(Document& currentDoc, SavingState& currentSave);

    };

	class OpenExportTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		bool mProjectFile;

	public:

		OpenExportTES4Stage (Document& document, SavingState& state, bool projectFile);
		///< \param projectFile Saving the project file instead of the content file.

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportHeaderTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		bool mSimple;

	public:

		ExportHeaderTES4Stage (Document& document, SavingState& state, bool simple);
		///< \param simple Simplified header (used for project files).

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	template<class CollectionT>
	class ExportCollectionTES4Stage : public Stage
	{
		const CollectionT& mCollection;
		SavingState& mState;
		CSMWorld::Scope mScope;
		bool mSkipMasterRecords;
		int mNumRecords=0;

	public:

		ExportCollectionTES4Stage (const CollectionT& collection, SavingState& state,
			CSMWorld::Scope scope = CSMWorld::Scope_Content, bool skipMasters = true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	template<class CollectionT>
	ExportCollectionTES4Stage<CollectionT>::ExportCollectionTES4Stage (const CollectionT& collection,
		SavingState& state, CSMWorld::Scope scope, bool skipMasters)
		: mCollection (collection), mState (state), mScope (scope)
	{
		mSkipMasterRecords = skipMasters;
	}

	template<class CollectionT>
	int ExportCollectionTES4Stage<CollectionT>::setup()
	{
		uint32_t formID;
		ESM::ESMWriter& writer = mState.getWriter();

		// assign all formIDs prior to perform()
		for (int recordIndex=0; recordIndex < mCollection.getSize(); recordIndex++)
		{
			CSMWorld::RecordBase::State state = mCollection.getRecord(recordIndex).mState;
			typename CollectionT::ESXRecord record = mCollection.getRecord (recordIndex).get();

			bool exportOrSkip = false;
			if (mSkipMasterRecords)
			{
				exportOrSkip = (state == CSMWorld::RecordBase::State_Modified ||
					state == CSMWorld::RecordBase::State_ModifiedOnly ||
					state == CSMWorld::RecordBase::State_Deleted);
			}
			else
			{
				exportOrSkip = true;
			}

			if (exportOrSkip)
			{
				mNumRecords++;
				std::string strEDID = writer.generateEDIDTES4(record.mId, 0);
				std::string sSIG;
				for (int tempindex = 0; tempindex<4; ++tempindex)
					/// \todo make endianess agnostic
					sSIG += reinterpret_cast<const char *> (&record.sRecordId)[tempindex];
				sSIG[4] = '\0';

				if (record.sRecordId == ESM::REC_SCPT)
					strEDID = writer.generateEDIDTES4(record.mId, 3);
				if (record.sRecordId == ESM::REC_REGN)
				{
					strEDID = writer.generateEDIDTES4(record.mId, 2);
					if (Misc::StringUtils::lowerCase(strEDID).find("region") == std::string::npos)
					{
						strEDID += "Region";
					}
				}
				formID = writer.crossRefStringID(strEDID, sSIG, false, true);
				if (formID == 0)
				{
					// first fall-back
					strEDID = writer.generateEDIDTES4(record.mId, 2);
					formID = writer.crossRefStringID(strEDID, sSIG, false, true);
				}
				if (formID == 0)
				{
					// second fall-back, create record
					if (record.sRecordId == ESM::REC_REGN)
					{
						strEDID = writer.generateEDIDTES4(record.mId, 2);
						if (Misc::StringUtils::lowerCase(strEDID).find("region") == std::string::npos)
						{
							strEDID += "Region";
						}
					}
					else if (record.sRecordId == ESM::REC_SCPT)
						strEDID = writer.generateEDIDTES4(record.mId, 3);
					else
						strEDID = writer.generateEDIDTES4(record.mId, 0);
					formID = writer.getNextAvailableFormID();
					formID = writer.reserveFormID(formID, strEDID, sSIG);
				}
			}
		}

		return mCollection.getSize();
	}

	template<class CollectionT>
	void ExportCollectionTES4Stage<CollectionT>::perform (int stage, Messages& messages)
	{
		bool exportOrSkip=false;
		ESM::ESMWriter& writer = mState.getWriter();
		CSMWorld::RecordBase::State state = mCollection.getRecord (stage).mState;
		typename CollectionT::ESXRecord record = mCollection.getRecord (stage).get();

		std::string sSIG;
		for (int i = 0; i<4; ++i)
			/// \todo make endianess agnostic
			sSIG += reinterpret_cast<const char *> (&record.sRecordId)[i];
		sSIG[4] = '\0';

		// if stage == 0, then add the group record first
		if (stage == 0 && mNumRecords > 0)
		{
			writer.startGroupTES4(sSIG, 0);
		}

		if (CSMWorld::getScopeFromId (mCollection.getRecord (stage).get().mId)!=mScope)
			return;
		
		if (mSkipMasterRecords == true)
		{
			// check for modified / deleted state, otherwise skip
//			exportOrSkip = ( record.isModified() || record.isDeleted() );
			exportOrSkip = ( state == CSMWorld::RecordBase::State_Modified ||
				state == CSMWorld::RecordBase::State_ModifiedOnly ||
				state == CSMWorld::RecordBase::State_Deleted );

/*
				state == CSMWorld::RecordBase::State_Modified || 
				state == CSMWorld::RecordBase::State_ModifiedOnly ||
				state == CSMWorld::RecordBase::State_Deleted ||
				((formID & 0xFF000000) > 0x01000000) );
*/
		}
		else {
			// no skipping, export all
			exportOrSkip=true;
		}

//		if ( (record.sRecordId == ESM::REC_CLAS || record.sRecordId == ESM::REC_REGN) && 
//		if ( (mSkipMasterRecords == true) && ((formID & 0xFF000000) <= 0x01000000) && (formID != 0) )
//			exportOrSkip = false;

		if (exportOrSkip)
		{
			std::string strEDID = writer.generateEDIDTES4(record.mId, 0);
			if (record.sRecordId == ESM::REC_SCPT)
				strEDID = writer.generateEDIDTES4(record.mId, 3);
			uint32_t formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			if (formID == 0)
			{
				// fall-back
				strEDID = writer.generateEDIDTES4(record.mId, 2);
				formID = writer.crossRefStringID(strEDID, sSIG, false, true);
			}
			if (formID == 0)
			{
				std::cout << "WARNING: exporting record without pre-defined FormID (New FormID will be assigned): [" << sSIG << "] " << strEDID << std::endl;
//				throw std::runtime_error("WARNING: found collection item without pre-assigned FormID: " + strEDID);
			}

			uint32_t flags=0;
			if (state == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x01;
			writer.startRecordTES4 (record.sRecordId, flags, formID, strEDID);
			record.exportTESx (writer);
			writer.endRecordTES4 (record.sRecordId);
		}
		
		if (stage == (mCollection.getSize()-1) && mNumRecords > 0)
		{
			writer.endGroupTES4(sSIG);
		}
	}


	class ExportDialogueCollectionTES4Stage : public Stage
	{
		SavingState& mState;
		const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
		CSMWorld::InfoCollection& mInfos;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
		bool mQuestMode;
		std::vector<std::pair<ESM::DialInfo, std::string> > mGreetingInfoList;
		std::vector<std::pair<ESM::DialInfo, std::string> > mHelloInfoList;
		std::vector<std::string> mKeyPhraseList;

	public:
		ExportDialogueCollectionTES4Stage (Document& document, SavingState& state, bool processQuests, bool skipMasters=true);
		virtual int setup();
		virtual void perform (int stage, Messages& messages);
		std::vector<std::string> CreateAddTopicList(std::string infoText);

	};


	class ExportRefIdCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;

	public:

		ExportRefIdCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportAmmoCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportAmmoCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportWeaponCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportWeaponCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportSoulgemCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportSoulgemCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportKeyCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportKeyCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportMiscCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportMiscCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportLightCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportLightCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportLeveledItemCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportLeveledItemCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportIngredientCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportIngredientCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportFurnitureCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportFurnitureCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportFloraCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportFloraCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportContainerCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportContainerCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportClothingCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportClothingCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportClimateCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportClimateCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportBookCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportBookCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportArmorCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportArmorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportApparatusCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;
	public:
		ExportApparatusCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportPotionCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		bool mSkipMasterRecords;
		std::vector<int> mActiveRecords;
	public:
		ExportPotionCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportActivatorCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;
	public:
		ExportActivatorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);
		virtual int setup();
		///< \return number of steps
		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportDoorCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;

	public:

		ExportDoorCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportSTATCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;

	public:

		ExportSTATCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportNPCCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;

	public:

		ExportNPCCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportCreatureCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;
		bool mSkipMasterRecords;

	public:

		ExportCreatureCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportLeveledCreatureCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		std::vector<int> mActiveRecords;
		bool mSkipMasterRecords;

	public:

		ExportLeveledCreatureCollectionTES4Stage (Document& document, SavingState& state, bool skipMasters=true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportReferenceCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mNumStages;

	public:

		ExportReferenceCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

    class CellExportData
    {
    public:
        int blockX, blockY;
        int subblockX, subblockY;
        uint32_t formID;
        CSMWorld::Record<CSMWorld::Cell>* cellRecordPtr;
    };
    
	class ExportInteriorCellCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
        int mNumCells;
        bool blockInitialized[10][10];
        std::vector< std::pair< uint32_t, CSMWorld::Record<CSMWorld::Cell>* > > Blocks[10][10];

	public:

		ExportInteriorCellCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportExteriorCellCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
        int mNumCells;
		int cellCount;
		int oldBlockX, oldBlockY;
		int oldSubblockX, oldSubblockY;
        std::vector<CellExportData> mCellExportList;
        typedef std::map<int, std::map<int, CellExportData* > > SubBlockT;
        typedef std::map<int, std::map<int, SubBlockT* > > BlockT;
		std::map<int, std::map<int, BlockT* > > WorldGrid;
		typedef struct { int blockX; int blockY; int subblockX; int subblockY; } GridTrackT;
		std::vector<GridTrackT> GridTracker;
		bool mIsWrldHeaderWritten=false;
		std::vector<uint32_t> mSubCellQuadTexList;
		std::map<uint16_t, float> mTexLayerOpacityMap;
		uint32_t mPreBlendMap[6][6]; // ESM3-based, single-layer 4x4 grid +1 surrounding texture
		std::vector<uint32_t> mPreBlendTextureList;

		bool getLandDataFromXY(int origX, int origY, int& plugindex, const ESM::Land::LandData*& landData);
		void drawPreBlendMapXY(const ESM::Land::LandData *landData, int plugindex, int subCX, int subCY, int quadX, int quadY, int inputX, int inputY, int outputX, int outputY);
		void drawLeftBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawRightBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawTopBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawBottomBorder(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawTopLeftCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawTopRightCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawBottomLeftCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);
		void drawBottomRightCorner(int origX, int origY, int subCX, int subCY, int quadX, int quadY, int plugindex, const ESM::Land::LandData*& landData);

		void exportLTEX_basetexture(ESM::ESMWriter& writer, int quadVal);
		void exportLTEX_interpolatelayers(ESM::ESMWriter& writer, int quadVal);

	public:

		ExportExteriorCellCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.

		void createPreBlendMap(ESM::ESMWriter& writer, int baseX, int baseY, int subCX, int subCY, int quadX, int quadY);
		void gatherPreBlendTextureList();

		void gatherSubCellQuadrantLTEX(int SubCell, int subX, int subY, int quadrant, const ESM::Land::LandData *landData, int plugindex);
		void calculateTexLayerOpacityMap(int SubCell, int subX, int subY, int quadrant, const ESM::Land::LandData *landData, int plugindex, int layerID);
		
	};

	class ExportPathgridCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportPathgridCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportLandCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportLandCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportLandTextureCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount=0;
		bool mSkipMasterRecords=true;

	public:

		ExportLandTextureCollectionTES4Stage (Document& document, SavingState& state, bool skipMaster=true);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class CloseExportTES4Stage : public Stage
	{
		SavingState& mState;

	public:

		CloseExportTES4Stage (SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class FinalizeExportTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		FinalizeExportTES4Stage (Document& document, SavingState& state);
		void MakeBatchNIFFiles(ESM::ESMWriter& esm);
		void MakeBatchDDSFiles(ESM::ESMWriter& esm);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

}

#endif

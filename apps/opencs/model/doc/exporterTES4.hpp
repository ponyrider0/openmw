#ifndef CSM_DOC_EXPORTER_TES4_H
#define CSM_DOC_EXPORTER_TES4_H

#include "../world/scope.hpp"

#include "exporter.hpp"

namespace CSMWorld
{
	class RefIdCollection;
}

namespace CSMDoc
{
	class Exporter;
	class Document;
	class SavingState;

    class TES4Exporter : public Exporter
    {
//		Q_OBJECT

//	public:
//		Document *mDocument;
//		Operation *mExportOperation;
//		SavingState *mState;
	public:
		TES4Exporter(Document& document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding);
//		void initializeExporter(Document *document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding);
		void queryExportPath();
		void defineExportOperation();

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

	public:

		ExportCollectionTES4Stage (const CollectionT& collection, SavingState& state,
			CSMWorld::Scope scope = CSMWorld::Scope_Content);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	template<class CollectionT>
	ExportCollectionTES4Stage<CollectionT>::ExportCollectionTES4Stage (const CollectionT& collection,
		SavingState& state, CSMWorld::Scope scope)
		: mCollection (collection), mState (state), mScope (scope)
	{}

	template<class CollectionT>
	int ExportCollectionTES4Stage<CollectionT>::setup()
	{
		return mCollection.getSize();
	}

	template<class CollectionT>
	void ExportCollectionTES4Stage<CollectionT>::perform (int stage, Messages& messages)
	{
		if (CSMWorld::getScopeFromId (mCollection.getRecord (stage).get().mId)!=mScope)
			return;

		ESM::ESMWriter& writer = mState.getWriter();
		CSMWorld::RecordBase::State state = mCollection.getRecord (stage).mState;
		typename CollectionT::ESXRecord record = mCollection.getRecord (stage).get();

		// if stage == 0, then add the group record first
		if (stage == 0)
		{
			std::string sSIG;
			for (int i=0; i<4; ++i)
				sSIG += reinterpret_cast<const char *> (&record.sRecordId)[i];
			writer.startGroupTES4(sSIG, 0);
		}

		if (state == CSMWorld::RecordBase::State_Modified ||
			state == CSMWorld::RecordBase::State_ModifiedOnly ||
			state == CSMWorld::RecordBase::State_Deleted)
		{
			uint32_t flags=0;
			if (state == CSMWorld::RecordBase::State_Deleted)
				flags |= 0x01;
			writer.startRecordTES4 (record.sRecordId, flags);
			record.exportTES4 (writer);
			writer.endRecordTES4 (record.sRecordId);
		}
		
		if (stage == (mCollection.getSize()-1))
		{
			std::string sSIG;
			for (int i=0; i<4; ++i)
				sSIG += reinterpret_cast<const char *> (&record.sRecordId)[i];
			writer.endGroupTES4(sSIG);
		}
	}


	class ExportDialogueCollectionTES4Stage : public Stage
	{
		SavingState& mState;
		const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
		CSMWorld::InfoCollection& mInfos;

	public:

		ExportDialogueCollectionTES4Stage (Document& document, SavingState& state, bool journal);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
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

	class ExportCreaturesCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;

	public:

		ExportCreaturesCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportLeveledCreaturesCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		int mActiveRefCount;

	public:

		ExportLeveledCreaturesCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportReferenceCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportReferenceCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportInteriorCellCollectionTES4Stage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		bool blockInitialized[10][10];
		std::vector< std::pair< uint32_t, CSMWorld::Record<CSMWorld::Cell>* > > Blocks[10][10];
		int mNumCells;

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
		bool blockInitialized[10][10];
		std::vector< std::pair< uint32_t, CSMWorld::Record<CSMWorld::Cell>* > > Blocks[10][10];
		int mNumCells;

	public:

		ExportExteriorCellCollectionTES4Stage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
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

	public:

		ExportLandTextureCollectionTES4Stage (Document& document, SavingState& state);

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

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

}

#endif

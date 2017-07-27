#ifndef CSM_DOC_EXPORTER_TES3_H
#define CSM_DOC_EXPORTER_TES3_H

#include "../world/scope.hpp"

#include "exporter.hpp"

namespace CSMDoc
{
	class Exporter;
	class Document;
	class SavingState;

    class TES3Exporter : public Exporter
    {
//		Q_OBJECT

//	public:
//		Document *mDocument;
//		Operation *mExportOperation;
//		SavingState *mState;
	public:
		TES3Exporter(Document& document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding);
//		void initializeExporter(Document *document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding);

		void defineExportOperation();

    };

	class OpenExportStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		bool mProjectFile;

	public:

		OpenExportStage (Document& document, SavingState& state, bool projectFile);
		///< \param projectFile Saving the project file instead of the content file.

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportHeaderStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;
		bool mSimple;

	public:

		ExportHeaderStage (Document& document, SavingState& state, bool simple);
		///< \param simple Simplified header (used for project files).

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	template<class CollectionT>
	class ExportCollectionStage : public Stage
	{
		const CollectionT& mCollection;
		SavingState& mState;
		CSMWorld::Scope mScope;

	public:

		ExportCollectionStage (const CollectionT& collection, SavingState& state,
			CSMWorld::Scope scope = CSMWorld::Scope_Content);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	template<class CollectionT>
	ExportCollectionStage<CollectionT>::ExportCollectionStage (const CollectionT& collection,
		SavingState& state, CSMWorld::Scope scope)
		: mCollection (collection), mState (state), mScope (scope)
	{}

	template<class CollectionT>
	int ExportCollectionStage<CollectionT>::setup()
	{
		return mCollection.getSize();
	}

	template<class CollectionT>
	void ExportCollectionStage<CollectionT>::perform (int stage, Messages& messages)
	{
		if (CSMWorld::getScopeFromId (mCollection.getRecord (stage).get().mId)!=mScope)
			return;

		ESM::ESMWriter& writer = mState.getWriter();
		CSMWorld::RecordBase::State state = mCollection.getRecord (stage).mState;
		typename CollectionT::ESXRecord record = mCollection.getRecord (stage).get();

		if (state == CSMWorld::RecordBase::State_Modified ||
			state == CSMWorld::RecordBase::State_ModifiedOnly ||
			state == CSMWorld::RecordBase::State_Deleted)
		{
			writer.startRecord (record.sRecordId);
			record.save (writer, state == CSMWorld::RecordBase::State_Deleted);
			writer.endRecord (record.sRecordId);
		}
	}


	class ExportDialogueCollectionStage : public Stage
	{
		SavingState& mState;
		const CSMWorld::IdCollection<ESM::Dialogue>& mTopics;
		CSMWorld::InfoCollection& mInfos;

	public:

		ExportDialogueCollectionStage (Document& document, SavingState& state, bool journal);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportRefIdCollectionStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportRefIdCollectionStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportReferenceCollectionStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportReferenceCollectionStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class ExportCellCollectionStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportCellCollectionStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportPathgridCollectionStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportPathgridCollectionStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportLandCollectionStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportLandCollectionStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};


	class ExportLandTextureCollectionStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		ExportLandTextureCollectionStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class CloseExportStage : public Stage
	{
		SavingState& mState;

	public:

		CloseExportStage (SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

	class FinalizeExportStage : public Stage
	{
		Document& mDocument;
		SavingState& mState;

	public:

		FinalizeExportStage (Document& document, SavingState& state);

		virtual int setup();
		///< \return number of steps

		virtual void perform (int stage, Messages& messages);
		///< Messages resulting from this stage will be appended to \a messages.
	};

}

#endif

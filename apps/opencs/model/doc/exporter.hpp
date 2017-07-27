#ifndef CSM_DOC_EXPORTER_H
#define CSM_DOC_EXPORTER_H

#include "operationholder.hpp"
#include "operation.hpp"
#include "state.hpp"
#include "savingstate.hpp"

namespace CSMDoc
{
	class OperationHolder;
	class Document;
	class SavingState;

    class Exporter
    {
//		Q_OBJECT

	public:
		bool mExportDefined=false;
		Operation mExportOperation;
		OperationHolder mExportManager;
		SavingState *mStatePtr=0;
		boost::filesystem::path mExportPath;
		ToUTF8::FromType mEncoding;
		Document& mDocument;

		Exporter(Document& document, const boost::filesystem::path exportPath, ToUTF8::FromType encoding);
		virtual ~Exporter();

		virtual void queryExportPath();
		// Define export steps within this method
		virtual void defineExportOperation();
		virtual void startExportOperation();

    };

}

#endif

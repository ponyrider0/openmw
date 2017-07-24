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

    class Exporter : public OperationHolder
    {
		Q_OBJECT

	public:
		Operation mExportOperation;
		Document& mDocument;
		SavingState mState;

		Exporter(Document& document, const boost::filesystem::path projectPath, ToUTF8::FromType encoding);

		// Define export steps within this method
		virtual void defineExportOperation();

    };

}

#endif

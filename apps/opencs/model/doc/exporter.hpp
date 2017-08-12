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
    protected:
        Operation *mExportOperation;
        SavingState *mStatePtr=0;
        boost::filesystem::path mExportPath;
        ToUTF8::FromType mEncoding;
        Document& mDocument;
        bool mExportDefined=false;
        
	public:
		OperationHolder mExportManager;

		Exporter(Document& document, const boost::filesystem::path exportPath, ToUTF8::FromType encoding);
		virtual ~Exporter();

		// Define export steps within this method
		virtual void defineExportOperation();
        virtual void startExportOperation(boost::filesystem::path filename);

    };

}

#endif

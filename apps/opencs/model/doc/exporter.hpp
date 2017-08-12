#ifndef CSM_DOC_EXPORTER_H
#define CSM_DOC_EXPORTER_H

#include <boost/filesystem/path.hpp>
#include "components/to_utf8/to_utf8.hpp"
#include "operationholder.hpp"
#include "savingstate.hpp"
#include "state.hpp"
#include "exportToBase.hpp"

namespace CSMDoc
{
	class Document;

    class Exporter
    {
	public:
        ExportToBase *mExportOperation=0;
        SavingState *mStatePtr=0;
        
        boost::filesystem::path mExportPath;
        ToUTF8::FromType mEncoding;
        Document& mDocument;
        OperationHolder mExportManager;

		Exporter(Document& document, ToUTF8::FromType encoding);
		virtual ~Exporter();
        virtual void startExportOperation(boost::filesystem::path filename);

    };

}

#endif

#ifndef CSM_DOC_EXPORT_TO_BASE_H
#define CSM_DOC_EXPORT_TO_BASE_H

#include "operation.hpp"
#include "state.hpp"

namespace CSMDoc
{
    class Document;
    class SavingState;
    
    class ExportToBase : public Operation
    {
	public:
        ExportToBase();
		virtual void defineExportOperation(Document& currentDoc, SavingState& currentSave)=0;

    };

}

#endif

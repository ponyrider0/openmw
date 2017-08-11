#ifndef CSV_DOC_OPERATIONS_H
#define CSV_DOC_OPERATIONS_H

#include <vector>

#include <QWidget>

class QVBoxLayout;
class QDockWidget;

namespace CSVDoc
{
    class Operation;

    class Operations : public QWidget
    {
            Q_OBJECT

            QVBoxLayout *mLayout;
            std::vector<Operation *> mOperations;

            // not implemented
            Operations (const Operations&);
            Operations& operator= (const Operations&);

        public:

            QDockWidget *mDock;
            Operations();

            void setProgress (int current, int max, int type, int threads);
            ///< Implicitly starts the operation, if it is not running already.

            void quitOperation (int type);
            ///< Calling this function for an operation that is not running is a no-op.

        signals:

            void abortOperation (int type);
    };
}

#endif

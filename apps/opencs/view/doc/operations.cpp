#include "operations.hpp"

#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "operation.hpp"

CSVDoc::Operations::Operations()
{
    /// \todo make widget height fixed (exactly the height required to display all operations)

    mDock = new QDockWidget;
    mDock->setFeatures (QDockWidget::NoDockWidgetFeatures);
    mLayout = new QVBoxLayout;
    setLayout (mLayout);
    mDock->setWidget (this);
    setVisible (false);
}

void CSVDoc::Operations::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    mDock->setVisible(visible);
}

void CSVDoc::Operations::setProgress (int current, int max, int type, int threads)
{
    for (std::vector<Operation *>::iterator iter (mOperations.begin()); iter!=mOperations.end(); ++iter)
        if ((*iter)->getType()==type)
        {
            (*iter)->setProgress (current, max, threads);
            return;
        }

    int oldCount = mOperations.size();
    int newCount = oldCount + 1;

    Operation *operation = new Operation (type, this);
    connect (operation, SIGNAL (abortOperation (int)), this, SIGNAL (abortOperation (int)));

    mLayout->addLayout (operation->getLayout());
    mOperations.push_back (operation);
    operation->setProgress (current, max, threads);

    if ( oldCount > 0)
        setFixedHeight (height()/oldCount * newCount);

    setVisible (true);
}

void CSVDoc::Operations::quitOperation (int type)
{
    for (std::vector<Operation *>::iterator iter (mOperations.begin()); iter!=mOperations.end(); ++iter)
        if ((*iter)->getType()==type)
        {
            int oldCount = mOperations.size();
            int newCount = oldCount - 1;

            mLayout->removeItem ((*iter)->getLayout());

            (*iter)->deleteLater();
            mOperations.erase (iter);

            if (oldCount > 1)
                setFixedHeight (height() / oldCount * newCount);
            else
                setVisible (false);

            break;
        }
}

/*
 * QueryTab.h
 *
 *  Created on: Mar 12, 2016
 *      Author: stephane
 */

#ifndef UI_EXPLORER_TABS_QUERY_QUERYTAB_H_
#define UI_EXPLORER_TABS_QUERY_QUERYTAB_H_

#include <qsplitter.h>
#include <QTabWidget>
#include <QSqlRecord>
#include <QPushButton>
#include "QueryTextEdit.h"
#include "QueryThread.h"


namespace UI {
namespace Explorer {
namespace Tabs {
namespace Query {

class QueryTab: public QSplitter {

	Q_OBJECT
public:

	QueryTab(QWidget *parent = 0);
	virtual ~QueryTab();
	void focus();

public slots:
	void queryChanged();
	void stopQueries();
    void handleQueryResultReady(QList<QueryExecutionResult> results);

private:
	QueryTextEdit *queryTextEdit;
	QTabWidget *queryTabs;
	QueryThread *queryWorker;
	QPushButton *executeButton;
	QPushButton *stopButton;
};

} /* namespace Query */
} /* namespace Tabs */
} /* namespace Explorer */
} /* namespace UI */

#endif /* UI_EXPLORER_TABS_QUERY_QUERYTAB_H_ */

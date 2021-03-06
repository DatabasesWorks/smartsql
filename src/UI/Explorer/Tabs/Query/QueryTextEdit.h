/**
 * Copyright (C) 2016  Stéphane Martarello
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef UI_EXPLORER_TABS_QUERY_QUERYTEXTEDIT_H_
#define UI_EXPLORER_TABS_QUERY_QUERYTEXTEDIT_H_

#include <qtextedit.h>
#include <QCompleter>
#include <QStringListModel>

namespace UI {
namespace Explorer {
namespace Tabs {
namespace Query {

class QueryTextEdit: public QTextEdit {

	Q_OBJECT

public:
	QueryTextEdit(QWidget *parent = 0);
	virtual ~QueryTextEdit();

signals:
	void queryChanged();

protected:
	void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;

private slots:
	void insertCompletion(const QString &completion);
    void showContextMenu(const QPoint &);

public slots:
		void databaseChanged();
        void formatSql();

private:
	QString textUnderCursor() const;
	QStringList getTableList();
	QCompleter *autocomplete;
	QStringList tableList;
	QStringListModel *autoCompleteModel;
	void loadTableFields();
};

} /* namespace Query */
} /* namespace Tabs */
} /* namespace Explorer */
} /* namespace UI */

#endif /* UI_EXPLORER_TABS_QUERY_QUERYTEXTEDIT_H_ */

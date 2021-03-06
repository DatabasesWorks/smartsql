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

#ifndef UI_EXPLORER_TABS_TABLEFILTERTEXTEDIT_H_
#define UI_EXPLORER_TABS_TABLEFILTERTEXTEDIT_H_

#include <qtextedit.h>
#include <QWidget>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QCompleter>

namespace UI {
namespace Explorer {
namespace Tabs {
namespace Table {

class TableFilterTextEdit: public QTextEdit {

	Q_OBJECT

public:
	TableFilterTextEdit(QWidget *parent = 0);
	virtual ~TableFilterTextEdit();
	QCompleter *getAutocomplete();

signals:
	    void filterChanged(QString filter);

protected:
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;

private slots:
    void insertCompletion(const QString &completion);

private:
    QString textUnderCursor() const;

private:
    QCompleter *autocomplete;
};

} /* namespace Table */
} /* namespace Tabs */
} /* namespace Explorer */
} /* namespace UI */

#endif /* UI_EXPLORER_TABS_TABLEFILTERTEXTEDIT_H_ */

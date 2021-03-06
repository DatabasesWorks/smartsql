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

#ifndef SERVERTAB_H
#define SERVERTAB_H

#include <QTableView>
#include <QWidget>

class ServerTab : public QTableView
{
    Q_OBJECT

public:
    ServerTab(QWidget *parent = 0);
    virtual ~ServerTab();
    void reload();

public slots:
    void handleDoubleClicked(QModelIndex index);

signals:
    void showDatabase(QString databaseName);
};

#endif // SERVERTAB_H

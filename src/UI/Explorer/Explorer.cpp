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

#include <UI/Explorer/Explorer.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QToolButton>
#include <QSqlDatabase>
#include <QSplitter>
#include <QItemSelectionModel>
#include <QDebug>
#include <QKeySequence>
#include <QShortcut>
#include <QPushButton>
#include <QMessageBox>
#include <QSqlError>
#include <QPixmap>
#include "Model/DataBaseModel.h"
#include "Model/TableFilterProxyModel.h"
#include "Tabs/Query/QueryTab.h"
#include "Tabs/TabView.h"
#include "Util/DataBase.h"
#include "Tabs/TableDetails/TableDetailsTab.h"


namespace UI {
namespace Explorer {

Explorer::Explorer(QWidget *parent, QJsonObject sessionConf) : QWidget(parent) {


	this->dataBaseTree = new DataBaseTree(this, sessionConf);

	QSplitter *splitter = new QSplitter(Qt::Horizontal);

	QHBoxLayout *hboxlayout = new QHBoxLayout;
	this->setLayout(hboxlayout);

	QWidget *leftPartWidget = new QWidget(this);
	QVBoxLayout *leftPartlayout = new QVBoxLayout;
	leftPartlayout->setContentsMargins(0, 0, 0, 0);
	leftPartWidget->setLayout(leftPartlayout);

	QWidget *databaseFilterContainer = new QWidget(this);
	QHBoxLayout *databaseFilterLayout = new QHBoxLayout();
	databaseFilterLayout->setContentsMargins(0, 0, 0, 0);
	databaseFilterContainer->setLayout(databaseFilterLayout);

	QLabel *databaseIconLabel = new QLabel();
	databaseIconLabel->setToolTip(tr("Database filter"));
	QPixmap iconDatabase(":/resources/icons/database-icon-24.png");
	databaseIconLabel->setPixmap(iconDatabase);

	databaseFilterLayout->addWidget(databaseIconLabel);
	this->databaseFilterLineEdit = new QLineEdit();
	this->databaseFilterLineEdit->setPlaceholderText(tr("Database filter"));
	databaseFilterLayout->addWidget(this->databaseFilterLineEdit);


	QWidget *tableFilterContainer = new QWidget(this);
	QHBoxLayout *tableFilterLayout = new QHBoxLayout();
	tableFilterLayout->setContentsMargins(0, 0, 0, 0);
	tableFilterContainer->setLayout(tableFilterLayout);

	QLabel *tableIconLabel = new QLabel();
	tableIconLabel->setToolTip(tr("Table filter"));
	QPixmap icon(":/resources/icons/database-table-icon-24.png");
	tableIconLabel->setPixmap(icon);

	tableFilterLayout->addWidget(tableIconLabel);
	this->tableFilterLineEdit = new QLineEdit();
	this->tableFilterLineEdit->setPlaceholderText(tr("Table filter"));
	tableFilterLayout->addWidget(this->tableFilterLineEdit);


	leftPartlayout->addWidget(databaseFilterContainer);
	leftPartlayout->addWidget(tableFilterContainer);
	leftPartlayout->addWidget(this->dataBaseTree);

	splitter->addWidget(leftPartWidget);

	this->explorerTabs = new Tabs::TabView(splitter);

    QShortcut * addTabShortCut = new QShortcut(QKeySequence("Ctrl+T"), this->explorerTabs);
    QShortcut * closeTabShortCut = new QShortcut(QKeySequence("Ctrl+W"), this->explorerTabs);
	splitter->addWidget(this->explorerTabs);

	QSortFilterProxyModel *filterModel = (QSortFilterProxyModel *)this->dataBaseTree->model();
    QModelIndex	index = filterModel->index(0, 0);

	this->dataBaseTree->selectionModel()->setCurrentIndex(index, (QItemSelectionModel::Select | QItemSelectionModel::Rows));

    this->databaseTab = new Tabs::Database::DataBaseTab(this);
    this->databaseTab->hide();
    this->serverTab = new ServerTab(this);
    this->explorerTabs->addTab(this->serverTab, tr("Host"));

	Tabs::Query::QueryTab *queryTab = new Tabs::Query::QueryTab(this);
	this->explorerTabs->addTab(queryTab, tr("Query"));

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 3);
	hboxlayout->addWidget(splitter);

    this->tableDetailsTab = new TableDetailsTab(this);
    this->tableDetailsTab->hide();

	this->tableTab = new Tabs::Table::TableTab(this);
	this->tableTab->hide();

	// The first tabs are not closable
	this->explorerTabs->tabBar()->tabButton(0, QTabBar::RightSide)->hide();
	this->explorerTabs->tabBar()->tabButton(1, QTabBar::RightSide)->hide();

	connect(this->tableFilterLineEdit, SIGNAL (textEdited(QString)), this->dataBaseTree, SLOT (filterTable(QString)));
	connect(this->databaseFilterLineEdit, SIGNAL (textEdited(QString)), this->dataBaseTree, SLOT (filterDatabase(QString)));

	connect(this->dataBaseTree, SIGNAL (closeExplorer()), this, SLOT (handleCloseExplorer()));
    connect(this->dataBaseTree, SIGNAL (openTableInNewTab()), this, SLOT (handleOpenTableInTab()));
	connect(this->dataBaseTree, SIGNAL (doubleClicked(QModelIndex)), this, SLOT (dataBaseTreeDoubleClicked(QModelIndex)));
	connect(addTabShortCut, SIGNAL(activated()), this, SLOT(addQueryTab()));
    connect(closeTabShortCut, SIGNAL(activated()), this, SLOT(closeTab()));
    connect(this->explorerTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeQueryTab(int)));
	connect(this->explorerTabs->tabBar(), SIGNAL(newTabRequested()), this, SLOT(addQueryTab()));
    connect(this->explorerTabs->tabBar(), SIGNAL(currentChanged(int)), SLOT(handleCurrentTabChanged(int)));
    connect(this->dataBaseTree->getDataBaseModel(), SIGNAL(databaseChanged()), this, SLOT(refreshDatabase()));
    connect(this->tableTab, SIGNAL(openForeignKeyInTab(QSqlDatabase,QString,QString)), SLOT(handleOpenForeignKeyInTab(QSqlDatabase,QString,QString)));


    connect(this->dataBaseTree->selectionModel(),
          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          this, SLOT(dataBaseTreeItemChanged()));

    connect(this->databaseTab, SIGNAL(showTable(QString)), SLOT(handleShowTable(QString)));
    connect(this->serverTab, SIGNAL(showDatabase(QString)), SLOT(handleShowDatabase(QString)));
}

/**
 * Open the selected table in a new tab
 *
 */
void Explorer::handleOpenTableInTab()
{
    QModelIndex index = ((Model::TableFilterProxyModel *)this->dataBaseTree->model())->mapToSource(this->dataBaseTree->currentIndex());
    QStandardItem *serverItem = this->dataBaseTree->getDataBaseModel()->invisibleRootItem()->child(index.parent().parent().row(), 0);
    QStandardItem *dbItem = serverItem->child(index.parent().row());
    QStandardItem *tableItem = dbItem->child(index.row());

    QJsonObject connectionConf = serverItem->data().toJsonObject();
    if (Util::DataBase::open(connectionConf, dbItem->text())) {
        Tabs::Table::TableTab *tableTab = new Tabs::Table::TableTab(this);
        tableTab->setTable(Util::DataBase::cloneCurrentConnection(), tableItem->text());
        tableTab->loadData();
        int tabIndex = this->explorerTabs->addTab(tableTab, dbItem->text()+"."+tableItem->text());
        this->explorerTabs->setCurrentIndex(tabIndex);

        connect(tableTab, SIGNAL(openForeignKeyInTab(QSqlDatabase,QString,QString)), SLOT(handleOpenForeignKeyInTab(QSqlDatabase,QString,QString)));
    }
}

void Explorer::handleOpenForeignKeyInTab(QSqlDatabase connection, QString table, QString whereCondition)
{
    Tabs::Table::TableTab *tableTab = new Tabs::Table::TableTab(this);
    tableTab->setTable(connection, table);
    tableTab->setFilter(whereCondition);
    tableTab->loadData();
    int tabIndex = this->explorerTabs->addTab(tableTab, connection.databaseName()+"."+table);
    this->explorerTabs->setCurrentIndex(tabIndex);
    connect(tableTab, SIGNAL(openForeignKeyInTab(QSqlDatabase,QString,QString)), SLOT(handleOpenForeignKeyInTab(QSqlDatabase,QString,QString)));
}

void Explorer::addQueryTab(){
	Tabs::Query::QueryTab *queryTab = new Tabs::Query::QueryTab(this);
	this->explorerTabs->addTab(queryTab, tr("Query"));
	this->explorerTabs->setCurrentWidget(queryTab);
	queryTab->focus();
}

void Explorer::closeQueryTab(int index)
{
	if (index > 1){
        QWidget *w = this->explorerTabs->widget(index);
		this->explorerTabs->removeTab(index);
        delete w;
	}
}

void Explorer::closeTab()
{
    int currentIndex = this->explorerTabs->currentIndex();

    if (!this->explorerTabs->tabBar()->tabButton(currentIndex, QTabBar::RightSide)->isHidden()) {
        QWidget *w = this->explorerTabs->widget(currentIndex);
        this->explorerTabs->removeTab(currentIndex);
        delete w;
    }
}

void Explorer::dataBaseTreeDoubleClicked(QModelIndex index)
{
	int tableTabIndex = this->explorerTabs->indexOf(this->tableTab) ;

	if (tableTabIndex != -1){
		QSortFilterProxyModel *model = (QSortFilterProxyModel *)this->dataBaseTree->model();
		index = model->mapToSource(index);

		// Click on a table
		if (index.parent().parent().isValid()){
			this->explorerTabs->setCurrentWidget(this->tableTab);
		}
	}
}

void Explorer::dataBaseTreeItemChanged()
{
	int tableTabIndex = this->explorerTabs->indexOf(this->tableTab) ;
    int tableDetailsTabIndex = this->explorerTabs->indexOf(this->tableDetailsTab) ;
    int dataBaseTabIndex = this->explorerTabs->indexOf(this->databaseTab) ;
    int serverTabIndex = this->explorerTabs->indexOf(this->serverTab) ;
    QModelIndex currentIndex = this->dataBaseTree->currentIndex();


    QSortFilterProxyModel *model = (QSortFilterProxyModel *)this->dataBaseTree->model();
    QModelIndex index = model->mapToSource(currentIndex);
    QStandardItem *root = this->dataBaseTree->getDataBaseModel()->invisibleRootItem();

    if (!currentIndex.parent().isValid()){
		// click on the server host
		if (tableTabIndex != -1){
			this->explorerTabs->removeTab(tableTabIndex);
		}

        if (tableDetailsTabIndex != -1){
            this->explorerTabs->removeTab(tableDetailsTabIndex);
        }

        if (dataBaseTabIndex != -1) {
            this->explorerTabs->removeTab(dataBaseTabIndex);
        }

        if (serverTabIndex == -1) {
            this->explorerTabs->insertTab(0, this->serverTab, tr("Host"));
             this->explorerTabs->tabBar()->tabButton(0, QTabBar::RightSide)->hide();
        }

        this->explorerTabs->setCurrentIndex(0);

        QStandardItem *server = root->child(index.row());
        QJsonObject sessionConf = server->data().toJsonObject();
        Util::DataBase::open(sessionConf);
        this->serverTab->reload();

		return ;
    } else if (serverTabIndex != -1) {
        this->explorerTabs->removeTab(serverTabIndex);
        this->databaseTab->show();
        this->explorerTabs->insertTab(0, this->databaseTab, "");
        this->explorerTabs->tabBar()->tabButton(0, QTabBar::RightSide)->hide();
    }

	QString dataBaseName;
	QString tableName;
	QStandardItem *dbItem ;

	QModelIndex dbIndex = index;
	if (index.parent().parent().isValid()){
		// click on a table
		dbIndex = index.parent();

		// Show table details
		dbItem = root->child(dbIndex.parent().row())->child(dbIndex.row());
		QStandardItem *tableItem = dbItem->child(index.row());

		dataBaseName = dbItem->text();
		tableName = tableItem->text();
	}
	else{
        // Click on a database
		dbItem = root->child(dbIndex.parent().row())->child(dbIndex.row());
		dataBaseName = dbItem->text();
         this->explorerTabs->setCurrentIndex(0);
	}

	QJsonObject sessionConf = dbItem->parent()->data().toJsonObject();
    QSqlDatabase currentDatabase = QSqlDatabase::database();
    QString previousDatabase = currentDatabase.databaseName();
	if (!Util::DataBase::open(sessionConf, dataBaseName)){

		QMessageBox *message = new QMessageBox();
		message->setWindowTitle(tr("Connection error"));
		message->setText(tr("Unable to connect to the Data Base"));

		message->setIcon(QMessageBox::Critical);
		message->exec();

		return;
	}


	if (!tableName.isNull()){
        // Set the current table
        this->tableTab->setTable(QSqlDatabase::database(), tableName);
        this->tableDetailsTab->setTable(QSqlDatabase::database(), tableName);

        // If the table tab is not visible, do not load the data
        // We use lazy loading, the data will be loaded when the tab will be activated.
        int tableTabIndex = this->explorerTabs->indexOf(this->tableTab);
        if (tableTabIndex == -1){
			this->tableTab->show();
            this->tableDetailsTab->show();
            this->explorerTabs->insertTab(1, this->tableDetailsTab, QString(tr("Table: %1")).arg(tableName));
            this->explorerTabs->insertTab(2, this->tableTab, tr("Data"));
            this->explorerTabs->setCurrentIndex(2);
			this->explorerTabs->tabBar()->tabButton(1, QTabBar::RightSide)->hide();
            this->explorerTabs->tabBar()->tabButton(2, QTabBar::RightSide)->hide();
            this->tableTab->loadData();
        } else if (tableTabIndex == this->explorerTabs->currentIndex()) {
            this->tableTab->loadData();
        }

        this->explorerTabs->setTabText(1, QString(tr("Table: %1")).arg(tableName));
    }
    else {
        if (tableTabIndex != -1){
            this->explorerTabs->removeTab(tableTabIndex);
        }

        if (tableDetailsTabIndex != -1) {
            this->explorerTabs->removeTab(tableDetailsTabIndex);
        }
    }

    this->explorerTabs->setTabText(0, QString(tr("Database: %1")).arg(dataBaseName));
    if (dataBaseName != previousDatabase) {
        this->databaseTab->refresh();
        emit databaseChanged();
    }
}

void Explorer::refreshDatabase()
{
    int tableTabIndex = this->explorerTabs->indexOf(this->tableTab) ;
    int tableDetailsTabIndex = this->explorerTabs->indexOf(this->tableDetailsTab) ;
    if (tableTabIndex != -1){
        this->explorerTabs->removeTab(tableTabIndex);
    }

    if (tableDetailsTabIndex != -1){
        this->explorerTabs->removeTab(tableDetailsTabIndex);
    }

	QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen() && !db.open()) {
        qDebug() << "Unable to refresh database, database connection failed";

        QMessageBox *message = new QMessageBox();
        message->setWindowTitle(tr("Connection error"));
        message->setText(tr("Unable to connect to the Data Base"));
        message->setDetailedText(db.lastError().text());

        message->setIcon(QMessageBox::Critical);
        message->exec();

        return;
    }

	this->databaseTab->refresh();
	this->explorerTabs->setTabText(0, QString(tr("Database: %1")).arg(db.databaseName()));

    this->explorerTabs->setCurrentWidget(this->databaseTab);

    emit databaseChanged();
}

void Explorer::addDatabase(QJsonObject sessionConfiguration)
{
    this->dataBaseTree->getDataBaseModel()->addServer(sessionConfiguration);
}

void Explorer::handleCloseExplorer()
{
	// We are closing the last open connection, we have to close the explorer and show the session window
	QSqlDatabase db = QSqlDatabase::database();
	if (db.isOpen()) {
		db.close();
	}

	emit closeExplorer();
}

/**
 * Shows the table tab with the selected table
 *
 * @param tableName the table name
 */
void Explorer::handleShowTable(QString tableName)
{
    // Removes filter on table name
    this->tableFilterLineEdit->clear();
    this->dataBaseTree->filterTable("");

    QModelIndex index = this->dataBaseTree->selectionModel()->currentIndex();
    this->dataBaseTree->expand(index);

    // Search the index for the table name in the tree view
    QModelIndexList items = this->dataBaseTree->model()->match(
                index,
                Qt::DisplayRole,
                QVariant::fromValue(tableName),
                1,
                Qt::MatchRecursive);

    if (!items.isEmpty()) {

        this->dataBaseTree->selectionModel()->setCurrentIndex(items.first(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        this->explorerTabs->setCurrentWidget(this->tableTab);
    }
}

/**
 * Shows the database tab with the selected database
 *
 * @param databaseName
 */
void Explorer::handleShowDatabase(QString databaseName)
{
    // Removes filter on database
    this->databaseFilterLineEdit->clear();
    this->dataBaseTree->filterDatabase("");

    QModelIndex index = this->dataBaseTree->selectionModel()->currentIndex();

    // Search the index for the database name in the tree view
    QModelIndexList items = this->dataBaseTree->model()->match(
                index,
                Qt::DisplayRole,
                QVariant::fromValue(databaseName),
                1,
                Qt::MatchRecursive);

    if (!items.isEmpty()) {

        this->dataBaseTree->selectionModel()->setCurrentIndex(items.first(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        this->explorerTabs->setCurrentWidget(this->databaseTab);
    }
}

void Explorer::handleCurrentTabChanged(int index)
{
    // The following code is used for the lazy loading of the table tab.
    if (this->explorerTabs->indexOf(this->tableTab) == index) {
        this->tableTab->loadData();
    }
}

Explorer::~Explorer() {

}

} /* namespace Explorer */
} /* namespace UI */

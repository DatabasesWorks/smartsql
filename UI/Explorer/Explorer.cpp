/*
 * Explorer.cpp
 *
 *  Created on: Mar 3, 2016
 *      Author: stephane
 */

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
#include "Model/DataBaseModel.h"
#include "Tabs/Query/QueryTab.h"
#include "Tabs/TabView.h"

namespace UI {
namespace Explorer {

Explorer::Explorer(QWidget *parent, QJsonObject sessionConf) : QWidget(parent) {


	this->dataBaseTree = new DataBaseTree(this, sessionConf);

	QSplitter *splitter = new QSplitter(Qt::Horizontal);

	QHBoxLayout *hboxlayout = new QHBoxLayout;
	this->setLayout(hboxlayout);

	QWidget *leftPartWidget = new QWidget;
	QVBoxLayout *leftPartlayout = new QVBoxLayout;
	leftPartWidget->setLayout(leftPartlayout);

	this->tableFilterLineEdit = new QLineEdit();
	leftPartlayout->addWidget(this->tableFilterLineEdit);
	leftPartlayout->addWidget(this->dataBaseTree);

	splitter->addWidget(leftPartWidget);

	this->explorerTabs = new Tabs::TabView(splitter);

	QShortcut * addTabShortCut = new QShortcut(QKeySequence("Ctrl+N"), this->explorerTabs);
	splitter->addWidget(this->explorerTabs);

	Model::DataBaseModel *model = this->dataBaseTree->getDataBaseModel();
	QStandardItem *rootItem = model->invisibleRootItem();
	QStandardItem *firstDataBase = rootItem->child(0)->child(0);

	// Select the first DataBase
	QSortFilterProxyModel *filterModel = (QSortFilterProxyModel *)this->dataBaseTree->model();
	QModelIndex	index = filterModel->mapFromSource(firstDataBase->index());
	this->dataBaseTree->selectionModel()->setCurrentIndex(index, (QItemSelectionModel::Select | QItemSelectionModel::Rows));

	QSqlDatabase db = QSqlDatabase::database();
	db.setDatabaseName(firstDataBase->text());
	db.open();

	this->databaseTab = new Tabs::Database::DataBaseTab();
	this->explorerTabs->addTab(this->databaseTab, QString(tr("Database: %1")).arg(db.databaseName()));

	Tabs::Query::QueryTab *queryTab = new Tabs::Query::QueryTab(this);
	this->explorerTabs->addTab(queryTab, tr("Query"));

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 3);
	hboxlayout->addWidget(splitter);

	this->tableTab = new Tabs::Table::TableTab(this);
	this->tableTab->hide();

	// The first tabs are not closable
	this->explorerTabs->tabBar()->tabButton(0, QTabBar::RightSide)->hide();
	this->explorerTabs->tabBar()->tabButton(1, QTabBar::RightSide)->hide();

	connect(this->tableFilterLineEdit, SIGNAL (textEdited(QString)), this->dataBaseTree, SLOT (filterTable(QString)));
	connect(this->dataBaseTree, SIGNAL (clicked(QModelIndex)), this, SLOT (dataBaseTreeClicked(QModelIndex)));
	connect(this->dataBaseTree, SIGNAL (doubleClicked(QModelIndex)), this, SLOT (dataBaseTreeDoubleClicked(QModelIndex)));
	connect(addTabShortCut, SIGNAL(activated()), this, SLOT(addQueryTab()));
	connect(this->explorerTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeQueryTab(int)));
	connect(this->explorerTabs->tabBar(), SIGNAL(newTabRequested()), this, SLOT(addQueryTab()));
	connect(model, SIGNAL(databaseChanged()), this, SLOT(refreshDatabase()));
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
		this->explorerTabs->removeTab(index);
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

void Explorer::dataBaseTreeClicked(QModelIndex index)
{
	int tableTabIndex = this->explorerTabs->indexOf(this->tableTab) ;
	if (!index.parent().isValid()){
		// click on the server host
		if (tableTabIndex != -1){
			this->explorerTabs->removeTab(tableTabIndex);
		}
		return ;
	}

	QSortFilterProxyModel *model = (QSortFilterProxyModel *)this->dataBaseTree->model();
	index = model->mapToSource(index);
	QStandardItem *root = this->dataBaseTree->getDataBaseModel()->invisibleRootItem();


	QString dataBaseName;
	QString tableName;
	QStandardItem *dbItem ;

	QModelIndex dbIndex = index;
	if (index.parent().parent().isValid()){
		// click on a table
		dbIndex = index.parent();

		// Show table details
		dbItem = root->child(0)->child(dbIndex.row());
		QStandardItem *tableItem = dbItem->child(index.row());

		dataBaseName = dbItem->text();
		tableName = tableItem->text();
	}
	else{
		dbItem = root->child(0)->child(dbIndex.row());
		dataBaseName = dbItem->text();
	}

	QSqlDatabase db = QSqlDatabase::database();
	QJsonObject sessionConf = dbItem->parent()->data().toJsonObject();

	if (dataBaseName != db.databaseName()){
		db.close();
		qDebug() << "Closing current connection";
		qInfo() << "Connection to the new database: " + dataBaseName;
		qDebug() << sessionConf;

		db.setHostName(sessionConf.value("hostname").toString());
		db.setUserName(sessionConf.value("user").toString());
		db.setPassword(sessionConf.value("password").toString());
		db.setPort(sessionConf.value("port").toInt());
		db.setDatabaseName(dataBaseName);

		if (!db.open()){
			qDebug() << db.lastError();

			QMessageBox *message = new QMessageBox();
			message->setWindowTitle(tr("Connection error"));
			message->setText(tr("Unable to connect to the Data Base"));
			message->setDetailedText(db.lastError().text());
			message->setIcon(QMessageBox::Critical);
			message->exec();

			return;
		}

		this->refreshDatabase();
	}

	if (!tableName.isNull()){
		this->tableTab->setTable(tableName);
		if (this->explorerTabs->indexOf(this->tableTab) == -1){
			this->tableTab->show();
			this->explorerTabs->insertTab(1, this->tableTab, tr("Data"));
			this->explorerTabs->tabBar()->tabButton(1, QTabBar::RightSide)->hide();
		}
	}
	else if (tableTabIndex != -1){
		this->explorerTabs->removeTab(tableTabIndex);
	}


	emit databaseChanged();
}

void Explorer::refreshDatabase()
{
	QSqlDatabase db = QSqlDatabase::database();
	this->databaseTab->refresh();
	this->explorerTabs->setTabText(0, QString(tr("Database: %1")).arg(db.databaseName()));
}

Explorer::~Explorer() {

}

} /* namespace Explorer */
} /* namespace UI */

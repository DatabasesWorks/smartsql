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
#include <QSqlDatabase>
#include <QSplitter>
#include <QDebug>
#include <QSqlError>
#include "Model/DataBaseModel.h"

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

	this->explorerTabs = new QTabWidget(this);
	splitter->addWidget(this->explorerTabs);



	Model::DataBaseModel *model = this->dataBaseTree->getDataBaseModel();
	QStandardItem *rootItem = model->invisibleRootItem();
	QStandardItem *firstDataBase = rootItem->child(0)->child(0);

	QSqlDatabase db = QSqlDatabase::database();
	db.setDatabaseName(firstDataBase->text());
	db.open();

	this->databaseTab = new Tabs::DataBaseTab(db);
	this->explorerTabs->addTab(this->databaseTab, "Database");

	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 3);
	hboxlayout->addWidget(splitter);

	this->tableTab = new Tabs::TableDataTab();

	connect(this->tableFilterLineEdit, SIGNAL (textEdited(QString)), this->dataBaseTree, SLOT (filterTable(QString)));
	connect(this->dataBaseTree, SIGNAL (clicked(QModelIndex)), this, SLOT (dataBaseTreeClicked(QModelIndex)));
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
	db.close();
	qDebug() << "Closing current connection";
	qInfo() << "Connection to the new database";
	qDebug() << sessionConf;

	db.setHostName(sessionConf.value("hostname").toString());
	db.setUserName(sessionConf.value("user").toString());
	db.setPassword(sessionConf.value("password").toString());
	db.setPort(sessionConf.value("port").toInt());
	db.setDatabaseName(dataBaseName);

	db.open();

	if (!tableName.isNull()){
		this->tableTab->setTable(tableName);
		if (this->explorerTabs->indexOf(this->tableTab) == -1){
			this->explorerTabs->insertTab(1, this->tableTab, tr("Data"));
		}
	}
	else if (tableTabIndex != -1){
		this->explorerTabs->removeTab(tableTabIndex);
	}

	this->databaseTab->refresh();
}

Explorer::~Explorer() {

}

} /* namespace Explorer */
} /* namespace UI */

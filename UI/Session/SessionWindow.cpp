/*
 * SessionWindow.cpp
 *
 *  Created on: Feb 28, 2016
 *      Author: stephane
 */
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QBoxLayout>
#include <QMainWindow>
#include <QSettings>
#include <QLabel>
#include <QDebug>
#include <QIcon>
#include <QListView>
#include <QStandardItemModel>
#include <QStringList>
#include <QCoreApplication>
#include <QStandardItem>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QUuid>
#include <QShortcut>
#include "SessionWindow.h"

namespace UI {

namespace Session {

SessionWindow::SessionWindow(QWidget *parent) : QWidget(parent){

	 QVBoxLayout *vlayout = new QVBoxLayout;
	 this->setLayout(vlayout);

	 QWidget *header = new QWidget;
	 vlayout->addWidget(header);

	 QHBoxLayout *hlayout = new QHBoxLayout;
	 header->setLayout(hlayout);

	 this->sessionList = new QListView;
	 this->sessionList->setFixedWidth(250);
	 this->sessionList->setEditTriggers(QAbstractItemView::NoEditTriggers);

	 // Create model
	 this->model = new QStandardItemModel(this);

	 // Make data
	 QStringList sessionNames;
	 hlayout->addWidget(this->sessionList);

	 QSettings settings("smartarello", "mysqlclient");
	 QString sessions = settings.value("sessions").toString();
	 if (!sessions.isNull() && !sessions.isEmpty()){
		 QJsonDocument doc = QJsonDocument::fromJson(sessions.toUtf8());
		 this->sessionStore = doc.array();
	 }

	 QStandardItem *rootItem = this->model->invisibleRootItem();
	 for (int i = 0; i < this->sessionStore.count(); i++){
		 QJsonObject session = this->sessionStore.at(i).toObject();
		 QStandardItem *item = new QStandardItem(session.value("name").toString());
		 item->setData(QIcon("resources/icons/mysql_logo.png"),Qt::DecorationRole);
		 rootItem->appendRow(item);
	 }

	 // Glue model and view together
	 this->sessionList->setModel(model);

	 this->editSession = new EditSessionWindow(this);
	 hlayout->addWidget(this->editSession);

	 // Footer part
	 QWidget *buttonContainer = new QWidget;
	 QBoxLayout *buttonLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	 buttonContainer->setLayout(buttonLayout);

	 QWidget *leftPartWidget = new QWidget;
	 QHBoxLayout *leftPartLayout = new QHBoxLayout;
	 leftPartWidget->setLayout(leftPartLayout);
	 leftPartLayout->setAlignment(Qt::AlignLeft);

	 QPushButton *newConnection = new QPushButton(tr("New"));
	 QPushButton *deleteButton = new QPushButton(tr("Delete"));
	 newConnection->setFixedWidth(150);
	 deleteButton->setFixedWidth(150);
	 leftPartLayout->addWidget(newConnection);
	 leftPartLayout->addWidget(deleteButton);
	 buttonLayout->addWidget(leftPartWidget);

	 QWidget *rightPartWidget = new QWidget;
	 QHBoxLayout *rightPartLayout = new QHBoxLayout;
	 rightPartLayout->setAlignment(Qt::AlignRight);
	 rightPartWidget->setLayout(rightPartLayout);
	 QPushButton *exitButton = new QPushButton(tr("Exit"));
	 exitButton->setFixedWidth(100);
	 rightPartLayout->addWidget(exitButton);
	 buttonLayout->addWidget(rightPartWidget);

	 vlayout->addWidget(buttonContainer);

	 // Connect button signal to appropriate slot
	 connect(newConnection, SIGNAL (released()), this, SLOT (handleNewConnection()));
	 connect(exitButton, SIGNAL (released()), this, SLOT (handleExit()));
	 connect(deleteButton, SIGNAL (released()), this, SLOT (handleDelete()));

	 connect(this->sessionList->selectionModel(),
	       SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
	       this, SLOT(handleSelectionChanged(QItemSelection)));

	 connect(this->sessionList, SIGNAL(doubleClicked(QModelIndex)), this->parent(), SLOT( handleOpenConnection() ) );

	 QShortcut* shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this->sessionList);
	 connect(shortcut, SIGNAL(activated()), this, SLOT(handleDelete()));

	 if ( this->sessionStore.count() == 0){
		 	this->handleNewConnection();
	 }
	 else{
		 this->sessionList->setCurrentIndex(this->model->index(0, 0));
		 this->updateSelected();
	 }
}

void SessionWindow::handleNewConnection()
{
	qInfo() << "Creating the new session configuration";
	QJsonObject newSession;
	newSession.insert("name", "Unamed");
	newSession.insert("user", "root");
	newSession.insert("password", "");
	newSession.insert("port", 3306);
	newSession.insert("hostname", "localhost");
	newSession.insert("uuid", QUuid::createUuid().toString());

	qDebug() << newSession;

	this->sessionStore.append(newSession);

	this->persistSessionStore();

	QStandardItem *rootItem = this->model->invisibleRootItem();
	QStandardItem *item = new QStandardItem("Unamed");
	item->setData(QIcon("resources/icons/mysql_logo.png"),Qt::DecorationRole);
	rootItem->appendRow(item);
	this->sessionList->setCurrentIndex(this->model->index(this->sessionStore.count() - 1, 0));

	this->updateSelected();
}

void SessionWindow::handleDelete()
{
	QModelIndex index = this->sessionList->currentIndex();
	if (index.row() < this->sessionStore.count()){

		QStandardItem *rootItem = this->model->invisibleRootItem();
		QMessageBox *confirm = new QMessageBox();
		confirm->setText("Delete session \""+ rootItem->child(index.row())->text() +"\" ?");
		confirm->setIcon(QMessageBox::Question);
		confirm->setWindowTitle("Confirm");
		confirm->setStandardButtons(QMessageBox::Cancel | QMessageBox::Yes);
		int ret = confirm->exec();

		if (ret == QMessageBox::Yes){
			QModelIndex index = this->sessionList->currentIndex();

			rootItem->removeRow(index.row());
			this->sessionStore.removeAt(index.row());
			this->persistSessionStore();

			if (this->sessionStore.count() > 0){
				this->sessionList->setCurrentIndex(this->model->index(0, 0));
			}
			else{
				this->handleNewConnection();
			}
		}
	}
}

void SessionWindow::handleSelectionChanged(const QItemSelection& selection)
{
	this->updateSelected();
}

void SessionWindow::updateSelected()
{
	QModelIndex index = this->sessionList->currentIndex();
	if (index.row() < this->sessionStore.count()){
		QJsonObject session = this->sessionStore.at(index.row()).toObject();

		this->editSession->setName(session.value("name").toString());
		this->editSession->setHostName(session.value("hostname").toString());
		this->editSession->setUser(session.value("user").toString());
		this->editSession->setPassword(session.value("password").toString());
		this->editSession->setPort(session.value("port").toInt());
		this->editSession->saveButton->setDisabled(true);
	}
}

void SessionWindow::handleExit()
{
	QCoreApplication::quit();
}

void SessionWindow::handleSaveConnection()
{
	QPushButton *button = (QPushButton *)sender();
	button->setDisabled(true);
	QModelIndex index = this->sessionList->currentIndex();
	if (index.row() < this->sessionStore.count()){
		QJsonObject session = this->sessionStore.at(index.row()).toObject();

		session.insert("name", this->editSession->getName());
		session.insert("hostname", this->editSession->getHostName());
		session.insert("user", this->editSession->getUser());
		session.insert("password", this->editSession->getPassword());
		session.insert("port", this->editSession->getPort());

		this->sessionStore.replace(index.row(), session);

		this->persistSessionStore();

		QStandardItem *rootItem = this->model->invisibleRootItem();
		rootItem->setChild(index.row(), new QStandardItem( this->editSession->getName()));

		this->sessionList->setCurrentIndex(index);
	}
}

void SessionWindow::persistSessionStore()
{
	qInfo() << "Saving the session configuration";
	QSettings settings("smartarello", "mysqlclient");

	QJsonDocument doc;
	doc.setArray(this->sessionStore);
	QString strJson(doc.toJson(QJsonDocument::Compact));

	qDebug() << strJson;
	settings.setValue("sessions", strJson);
}

QJsonObject SessionWindow::getSelectedSession()
{
	QModelIndex index = this->sessionList->currentIndex();
	QJsonObject session = this->sessionStore.at(index.row()).toObject();
	return session;
}

SessionWindow::~SessionWindow(){

}


}
}

/*
 * TableTab.cpp
 *
 *  Created on: Mar 4, 2016
 *      Author: stephane
 */

#include <UI/Explorer/Tabs/Table/TableTab.h>
#include <QDebug>
#include <QSqlTableModel>
#include <QStringListModel>
#include <QStringList>
#include <QSqlQuery>
#include <QSqlError>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QVariant>
#include <QHeaderView>
#include <QCompleter>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QInputDialog>
#include "TableModel.h"

namespace UI {
namespace Explorer {
namespace Tabs {
namespace Table {

TableTab::TableTab(QWidget *parent) : QSplitter(parent) {

	this->setOrientation(Qt::Vertical);
	this->tableData = new QTableView();
	this->tableData->setSortingEnabled(true);
	this->tableData->verticalHeader()->hide();
	this->tableData->setEditTriggers(QAbstractItemView::DoubleClicked);

	this->tableData->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this->tableData, SIGNAL(customContextMenuRequested(QPoint)), SLOT(customContextMenuRequested(QPoint)));

	this->whereConditionText = new TableFilterTextEdit();

	QWidget *topPart = new QWidget(this);
	QVBoxLayout *topPartLayout = new QVBoxLayout();
	topPart->setLayout(topPartLayout);

	this->tableInfoLabel = new QLabel();
	topPartLayout->addWidget(this->tableInfoLabel);
	topPartLayout->addWidget(this->tableData);
	this->addWidget(topPart);

	QWidget *bottomPart = new QWidget(this);
	QVBoxLayout *bottomPartLayout = new QVBoxLayout();
	bottomPart->setLayout(bottomPartLayout);

	QLabel *filterLabel = new QLabel(this);
	filterLabel->setText(tr("Filter:"));

	QPushButton *filterButton = new QPushButton(tr("Apply"));
	filterButton->setFixedWidth(100);
	filterButton->setToolTip("Apply filter (F5)");

	bottomPartLayout->addWidget(filterLabel);
	bottomPartLayout->addWidget(this->whereConditionText);
	bottomPartLayout->addWidget(filterButton);

	this->addWidget(bottomPart);

	this->setStretchFactor(0, 4);
	this->setStretchFactor(1, 1);

	TableModel *queryModel = new TableModel();
	this->tableData->setModel(queryModel);
	this->tableData->setSelectionBehavior(QAbstractItemView::SelectRows);

	connect(this->whereConditionText, SIGNAL(filterChanged(QString)), queryModel, SLOT(refreshWithFilter(QString)));
	connect(queryModel, SIGNAL(queryError(QString, QString)), this, SLOT(queryError(QString, QString)));
	connect(filterButton, SIGNAL(clicked(bool)), this, SLOT(applyFilterClicked(bool)));
}

void TableTab::applyFilterClicked(bool checked)
{
	((TableModel *)this->tableData->model())->refreshWithFilter(this->whereConditionText->toPlainText());
}

void TableTab::setTable(QString tableName) {

	TableModel *queryModel = (TableModel *)this->tableData->model();
	queryModel->setTable(tableName);

	QSqlDatabase db = QSqlDatabase::database();
	QSqlQuery query;
	query.prepare("SHOW TABLE STATUS WHERE Name LIKE :table");
	query.bindValue(":table", tableName);
	query.exec();

	if (query.lastError().isValid()) {
		qDebug() << "TableTab::setTable - " + query.lastError().text();
		this->tableInfoLabel->setText("");
	} else {
		if (query.next()){
			int rows = query.value(4).toInt();
			if (rows > 1000){
				this->tableInfoLabel->setText(QString(tr("%1.%2: %3 rows (approximately), limited to 1000")).arg(db.databaseName()).arg(tableName).arg(rows));
			} else {
				this->tableInfoLabel->setText(QString(tr("%1.%2: %3 rows (approximately)")).arg(db.databaseName()).arg(tableName).arg(rows));
			}
		} else {
			this->tableInfoLabel->setText("");
		}
	}


	QCompleter *completer = this->whereConditionText->getAutocomplete();

	QStringListModel *model =  new QStringListModel(QStringList(queryModel->getColumns()), completer);
	completer->setModel(model);
}

void TableTab::customContextMenuRequested(QPoint point)
{
	this->contextMenuIndex = this->tableData->indexAt(point);

	if (!this->contextMenuIndex.isValid()){
		return ;
	}

	TableModel * model = ((TableModel *)this->tableData->model());
	QModelIndexList list = this->tableData->selectionModel()->selectedRows(0);
	QMenu *menu = new QMenu(this);

	QAction *copyAction = new QAction(tr("Copy"), this);
	copyAction->setIcon(QIcon(":/resources/icons/copy-icon.png"));
	menu->addAction(copyAction);

	QAction *pasteAction = new QAction(tr("Paste"), this);
	pasteAction->setIcon(QIcon(":/resources/icons/paste-icon.png"));
	menu->addAction(pasteAction);

	menu->addSeparator();

	QAction *setNullAction = new QAction(tr("Set NULL"), this);
	setNullAction->setIcon(QIcon(":/resources/icons/empty-document-icon.png"));
	menu->addAction(setNullAction);

	QAction *deleteAction = new QAction(tr("Delete selected row"), this);
	deleteAction->setIcon(QIcon(":/resources/icons/delete-icon.png"));
	menu->addAction(deleteAction);

	QList<QString> columns = model->getColumns();
	if (columns.size() > this->contextMenuIndex.column()) {
		menu->addSeparator();

		QString colTitle = columns.at(this->contextMenuIndex.column());
		QAction *filterLikeAction = new QAction(QString(tr("`%1` LIKE \"%...%\"")).arg(colTitle), this);
		filterLikeAction->setIcon(QIcon(":/resources/icons/filter-icon.png"));
		menu->addAction(filterLikeAction);

		QAction *filterEqualAction = new QAction(QString(tr("`%1` LIKE \"...\"")).arg(colTitle), this);
		filterEqualAction->setIcon(QIcon(":/resources/icons/filter-icon.png"));
		menu->addAction(filterEqualAction);

		connect(filterLikeAction, SIGNAL(triggered(bool)), SLOT(handleFilterColumnLikeAction()));
		connect(filterEqualAction, SIGNAL(triggered(bool)), SLOT(handleFilterColumnEqualAction()));

		if (list.size() > 1) {
			filterLikeAction->setEnabled(false);
			filterEqualAction->setEnabled(false);
		}
	}

	QAction *refreshAction = new QAction(tr("Refresh"), this);
	refreshAction->setIcon(QIcon(":/resources/icons/refresh-icon.png"));
	menu->addAction(refreshAction);

	connect(setNullAction, SIGNAL(triggered(bool)), SLOT(handleSetNullAction()));
	connect(copyAction, SIGNAL(triggered(bool)), SLOT(handleCopyAction()));
	connect(pasteAction, SIGNAL(triggered(bool)), SLOT(handlePastAction()));
	connect(deleteAction, SIGNAL(triggered(bool)), SLOT(handleDeleteAction()));
	connect(refreshAction, SIGNAL(triggered(bool)), SLOT(applyFilterClicked(bool)));

	if (list.size() > 1) {
		setNullAction->setEnabled(false);
		copyAction->setEnabled(false);
		pasteAction->setEnabled(false);
	}

	menu->popup(this->tableData->viewport()->mapToGlobal(point));
}

void TableTab::handleFilterColumnLikeAction()
{
	TableModel * model = ((TableModel *)this->tableData->model());
	QList<QString> columns = model->getColumns();
	QString colTitle = columns.at(this->contextMenuIndex.column());

	bool ok;
	QString text = QInputDialog::getText(this,
			"",
			QString(tr("`%1` LIKE \"%...%\"")).arg(colTitle),
			QLineEdit::Normal,
			"", &ok);

	if (ok && !text.isEmpty()) {
		this->whereConditionText->setText(QString(tr("`%1` LIKE \"%%2%\"")).arg(colTitle).arg(text));
		this->applyFilterClicked(true);
	}
}

void TableTab::handleFilterColumnEqualAction()
{
	TableModel * model = ((TableModel *)this->tableData->model());
	QList<QString> columns = model->getColumns();
	QString colTitle = columns.at(this->contextMenuIndex.column());

	bool ok;
	QString text = QInputDialog::getText(this,
			"",
			QString(tr("`%1` LIKE \"...\"")).arg(colTitle),
			QLineEdit::Normal,
			"", &ok);

	if (ok && !text.isEmpty()) {
		this->whereConditionText->setText(QString(tr("`%1` LIKE \"%2\"")).arg(colTitle).arg(text));
		this->applyFilterClicked(true);
	}
}

void TableTab::handleSetNullAction()
{
	((TableModel *)this->tableData->model())->setData(this->contextMenuIndex, QVariant(), Qt::EditRole);
}

void TableTab::handleCopyAction()
{
	QVariant cellData = this->tableData->model()->data(this->contextMenuIndex);
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(cellData.toString());

}

void TableTab::handlePastAction()
{
	QClipboard *clipboard = QApplication::clipboard();
	QString value = clipboard->text(QClipboard::Clipboard);
	this->tableData->model()->setData(this->contextMenuIndex, QVariant(value), Qt::EditRole);
}

void TableTab::handleDeleteAction()
{
	TableModel * model = ((TableModel *)this->tableData->model());
	QModelIndexList list = this->tableData->selectionModel()->selectedRows(0);

	if (list.isEmpty()) {
		return ;
	}

	model->removeRows(list.first().row(), list.count(), list.first().parent());
}

void TableTab::queryError(QString query, QString error)
{
	QMessageBox *message = new QMessageBox(this);
	message->setText(error);
	message->setIcon(QMessageBox::Critical);
	if (!query.isEmpty()) {
		message->setDetailedText(query);
	}

	message->show();
}

TableTab::~TableTab() {

}

} /* namespace Table */
} /* namespace Tabs */
} /* namespace Explorer */
} /* namespace UI */
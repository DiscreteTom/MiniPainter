#include <QColorDialog>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scene.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
																					ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	// init color picker button
	fgColor = new QColor(0, 0, 0);
	bgColor = new QColor(255, 255, 255);
	setBtnColor(ui->fgColorBtn, *fgColor);
	setBtnColor(ui->bgColorBtn, *bgColor);

	// add scene
	ui->mainVerticalLayout->addWidget(new Scene(this));
}

MainWindow::~MainWindow()
{
	delete ui;
}

MainWindow::Tool MainWindow::getTool() const
{
	if (ui->penBtn->isChecked())
		return PEN;
	else if (ui->lineBtn->isChecked())
		return LINE;
	else if (ui->rectBtn->isChecked())
		return RECT;
	else if (ui->ellipseBtn->isChecked())
		return ELLIPSE;
	else if (ui->floodBtn->isChecked())
		return FLOOD;
	else
		return POLYGON;
}

MainWindow::PolyFillType MainWindow::getPolyFillType() const
{
	if (ui->shadowBtn->isChecked())
		return SHADOW;
	else if (ui->colorBtn->isChecked())
		return COLOR;
	else
		return NO;
}

void MainWindow::on_fgColorBtn_clicked()
{
	auto result = QColorDialog::getColor(*fgColor, this, "Choose foreground color");
	if (result.isValid()){
		*fgColor = result;
		setBtnColor(ui->fgColorBtn, result);
	}
}

void MainWindow::setBtnColor(QPushButton *btn, QColor color)
{
	QString qss = QString("background-color: rgb(%1,%2,%3);").arg(color.red()).arg(color.green()).arg(color.blue());
	btn->setStyleSheet(qss);
}

void MainWindow::on_bgColorBtn_clicked()
{
	auto result = QColorDialog::getColor(*fgColor, this, "Choose background color");
	if (result.isValid()){
		*bgColor = result;
		setBtnColor(ui->bgColorBtn, result);
	}
}

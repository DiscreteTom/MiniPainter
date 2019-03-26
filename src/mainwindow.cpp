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

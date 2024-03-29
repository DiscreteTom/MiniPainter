#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	enum Tool
	{
		PEN,
		LINE,
		RECT,
		ELLIPSE,
		FLOOD,
		POLYGON
	};

	enum PolyFillType
	{
		SHADOW,
		COLOR,
		NO
	};

	// state getter
	Tool getTool() const;
	PolyFillType getPolyFillType() const;
	int getShadowInterval() const { return ui->intervalSb->value(); }
	QColor getFgColor() const { return *fgColor; }
	QColor getBgColor() const { return *bgColor; }

private slots:
	void on_fgColorBtn_clicked();
	void on_bgColorBtn_clicked();

private:
	Ui::MainWindow *ui;

	QColor *fgColor; // foreground color
	QColor *bgColor; // background color

	void setBtnColor(QPushButton * btn, QColor color);
};

#endif // MAINWINDOW_H

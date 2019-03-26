#ifndef SCENE_H
#define SCENE_H

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include "mainwindow.h"

class Scene : public QWidget
{
	Q_OBJECT
public:
	Scene(MainWindow *parent = 0);
	virtual ~Scene();

	void done();

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;

	MainWindow *window;

	QColor **permanent; // left bottom is (0, 0), all white by default
	QColor **temp;			// left bottom is (0, 0), all invalid by default
	QPixmap *cache;			// left bottom is (0, 0), to optimize drawing speed

	bool refresh = true; // if true, refresh cache

	int startX; // x of start point, left bottom is (0, 0)
	int startY; // y of start point, left bottom is (0, 0)
	int endX;
	int endY;

	int transformY(int y) const { return HEIGHT - y - 1; } // left bottom (0, 0) <-> left top (0, 0)
	void drawLine(int x, int y); // with startX and startY, using Bresenham's Algorithm
	int max(int a, int b) const {return a > b ? a : b;}
	int min(int a, int b) const {return a < b ? a : b;}
	int abs(int a) const {return a > 0? a : -a;}

	void myUpdate(int x, int y, int width, int height);

protected:
	virtual void paintEvent(QPaintEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
};

#endif // SCENE_H

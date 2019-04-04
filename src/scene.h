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

private:
	const int WIDTH = 800;
	const int HEIGHT = 600;

	struct Temp // temp pixels
	{
		QColor color;
		int x;
		int y;
	};

	MainWindow *window;

	QColor **permanent; // left bottom is (0, 0), all white by default
	Temp *temp;					// record all temp points. left bottom point is (0, 0). End with invalid color.
	QPixmap *cache;			// left top is (0, 0), to optimize drawing speed

	bool clearingTemp = false;
	bool drawingTemp = false;

	int startX; // x of start point, left bottom is (0, 0)
	int startY; // y of start point, left bottom is (0, 0)
	int endX;
	int endY;

	void drawLine(int x, int y);												// with startX and startY, using Bresenham's Algorithm
	void BresenhamLine(int x1, int y1, int x2, int y2); // x1 & y1: left bottom point, x2 & y2: right top point
	void drawRect(int x, int y); // with startX/Y
	void floodFill(int x, int y); // flood fill 4-connected-region of (x, y) with foreground color

	int transformY(int y) const { return HEIGHT - y - 1; } // left bottom (0, 0) <-> left top (0, 0)
	int max(int a, int b) const { return a > b ? a : b; }
	int min(int a, int b) const { return a < b ? a : b; }
	int abs(int a) const { return a > 0 ? a : -a; }

	void clearTemp(); // set temp[] to empty and erase them on canvas according to startX/Y & endX/Y
	void drawTemp();
	void swapTemp(); // temp[].x <-> temp[].y
	void flipY(bool usingStartY = true); // temp[].y = 2 * startY - temp[].y
	void done(); // merge temp to permanent

protected:
	virtual void paintEvent(QPaintEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
};

#endif // SCENE_H

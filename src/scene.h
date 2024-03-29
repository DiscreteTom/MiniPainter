#ifndef SCENE_H
#define SCENE_H

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include "mainwindow.h"
#include <QVector>

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
		int x;
		int y;
		QColor color;
		Temp(int x = 0, int y = 0, QColor color = QColor()) : x(x), y(y), color(color) {}
	};

	struct Edge
	{
		QPoint p1;
		QPoint p2;
		QPoint lowerPoint;
		QPoint upperPoint;
		Edge(QPoint p1 = QPoint(), QPoint p2 = QPoint()) : p1(p1), p2(p2) {}
	};

	struct Node
	{
		int yMax;
		double x;
		double deltaX;
		int edgeIndex;
		bool operator<(const Node &ano) const { return (this->x == ano.x) ? (this->deltaX < ano.deltaX) : (this->x < ano.x); }
	};

	MainWindow *window;

	QColor **permanent; // left bottom is (0, 0), all white by default
	QVector<Temp> temp; // record all temp points. left bottom point is (0, 0)
	QPixmap *cache;			// left top is (0, 0), to optimize drawing speed

	bool clearingTemp = false;
	bool drawingTemp = false;
	bool refreshingPermanent = false;
	bool drawingPolygon = false;

	int startX; // x of start point, left bottom is (0, 0)
	int startY; // y of start point, left bottom is (0, 0)
	int endX;
	int endY;
	QVector<Edge> edges;

	void BresenhamLine(int x1, int y1, int x2, int y2); // x1 & y1: left bottom point, x2 & y2: right top point
	void getLine(int x1, int y1, int x2, int y2);				// get line in temp
	void drawLine(int x, int y);												// with startX and startY, using Bresenham's Algorithm
	void drawRect(int x, int y);												// with startX/Y
	void floodFill(int x, int y);												// flood fill 4-connected-region of (x, y) with foreground color
	void fill(int step = 0);														// according to edges
	QMap<int, QVector<Node>> constructET();
	void drawEllipse(int x, int y);

	int transformY(int y) const { return HEIGHT - y - 1; } // left bottom (0, 0) <-> left top (0, 0)
	int max(int a, int b) const { return a > b ? a : b; }
	int min(int a, int b) const { return a < b ? a : b; }
	int abs(int a) const { return a > 0 ? a : -a; }

	void clearTemp(); // set temp[] to empty and erase them on canvas according to startX/Y & endX/Y
	void drawTemp();
	void swapTemp();										 // temp[].x <-> temp[].y
	void flipY(int centerY); // temp[].y = 2 * centerY - temp[].y
	void done();												 // merge temp to permanent

protected:
	virtual void paintEvent(QPaintEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
};

#endif // SCENE_H

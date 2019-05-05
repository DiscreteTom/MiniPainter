#include "scene.h"
#include <QMap>
#include <QPainter>
#include <QDebug>
#include <QVector>
#include <QPoint>
#include <QtAlgorithms>

Scene::Scene(MainWindow *parent) : QWidget(parent)
{
	window = parent; // to get state

	setFixedSize(WIDTH, HEIGHT);

	// init pixels
	permanent = new QColor *[HEIGHT];
	for (int i = 0; i < HEIGHT; ++i)
	{
		permanent[i] = new QColor[WIDTH];
		for (int j = 0; j < WIDTH; ++j)
		{
			permanent[i][j] = QColor(255, 255, 255); // white
		}
	}

	// init cache
	cache = new QPixmap(WIDTH, HEIGHT);
	cache->fill(); // fill with white color

	setAttribute(Qt::WA_OpaquePaintEvent); // enable paint without erase

	repaint(); // draw background
}

Scene::~Scene()
{
	for (int i = 0; i < HEIGHT; ++i)
	{
		delete[] permanent[i];
	}
	delete[] permanent;
}

void Scene::done()
{
	// merge temp to permanent
	for (int i = 0; i < temp.size(); ++i)
	{
		// judge whether current point is inside canvas
		if (temp[i].x >= 0 && temp[i].x < WIDTH && temp[i].y >= 0 && temp[i].y < HEIGHT)
			permanent[temp[i].y][temp[i].x] = temp[i].color;
	}
	temp.clear();

	// drawingTemp and clearingTemp should always be false
	// just in case
	drawingTemp = false;
	clearingTemp = false;
}

void Scene::drawLine(int x, int y)
{
	clearTemp();

	endX = x;
	endY = y;

	getLine(startX, startY, x, y);

	drawTemp();
}

void Scene::BresenhamLine(int x1, int y1, int x2, int y2)
{
	// check x1, y1, x2, y2
	if (x1 > x2)
	{
		qDebug() << "bad x";
		return;
	}
	if (y1 > y2)
	{
		qDebug() << "bad y";
		return;
	}
	if (abs(x1 - x2) < abs(y1 - y2))
	{
		qDebug() << "bad scale";
		return;
	}

	temp.clear();
	int e = -(x2 - x1);
	int currentY = y1;
	for (int i = x1; i <= x2; ++i)
	{
		if (e >= 0)
		{
			e -= 2 * (x2 - x1);
			++currentY;
		}
		e += 2 * (y2 - y1);
		temp.push_back(Temp(i, currentY, window->getFgColor())); // temp[i - x1]
	}
}

void Scene::getLine(int x1, int y1, int x2, int y2)
{
	if (x1 <= x2)
	{
		if (y1 <= y2)
		{
			if (abs(x1 - x2) >= abs(y1 - y2))
			{
				// 0 <= gradient <= 1
				BresenhamLine(x1, y1, x2, y2);
			}
			else
			{
				// 1 < gradient < infinite
				BresenhamLine(y1, x1, y2, x2);
				swapTemp();
			}
		}
		else
		{
			if (abs(x1 - x2) >= abs(y1 - y2))
			{
				// -1 <= gradient < 0
				BresenhamLine(x1, y1, x2, 2 * y1 - y2);
				flipY(y1);
			}
			else
			{
				// -infinite < gradient < -1
				BresenhamLine(y1, x1, 2 * y1 - y2, x2);
				swapTemp();
				flipY(y1);
			}
		}
	}
	else
	{
		// exchange start and end
		if (y2 <= y1)
		{
			if (abs(x2 - x1) >= abs(y2 - y1))
			{
				// 0 <= gradient <= 1
				BresenhamLine(x2, y2, x1, y1);
			}
			else
			{
				// 1 < gradient < infinite
				BresenhamLine(y2, x2, y1, x1);
				swapTemp();
			}
		}
		else
		{
			if (abs(x2 - x1) >= abs(y2 - y1))
			{
				// -1 <= gradient < 0
				BresenhamLine(x2, y2, x1, 2 * y2 - y1);
				flipY(y2);
			}
			else
			{
				// -infinite < gradient < -1
				BresenhamLine(y2, x2, 2 * y2 - y1, x1);
				swapTemp();
				flipY(y2);
			}
		}
	}
}

void Scene::drawRect(int x, int y)
{
	clearTemp();

	endX = x;
	endY = y;

	// draw rect border
	for (int i = min(x, startX); i <= max(x, startX); ++i)
	{
		temp.push_back(Temp(i, startY, window->getFgColor()));
		temp.push_back(Temp(i, endY, window->getFgColor()));
	}
	for (int i = min(y, startY); i <= max(y, startY); ++i)
	{
		temp.push_back(Temp(startX, i, window->getFgColor()));
		temp.push_back(Temp(x, i, window->getFgColor()));
	}

	drawTemp();
}

void Scene::floodFill(int x, int y)
{
	if (rect().contains(x, transformY(y)))
	{
		QColor baseColor = permanent[y][x];
		if (baseColor == window->getFgColor())
			return;
		QVector<QPoint> openTable;
		openTable.push_back(QPoint(x, y));
		QPainter cachePainter(cache);
		cachePainter.setPen(window->getFgColor());
		int left = WIDTH;
		int right = 0;
		int top = 0;
		int bottom = HEIGHT;
		while (openTable.size())
		{
			auto p = openTable[0];
			openTable.pop_front();
			if (permanent[p.y()][p.x()] == baseColor)
			{
				permanent[p.y()][p.x()] = window->getFgColor();
				cachePainter.drawPoint(p.x(), transformY(p.y()));
				// judge border
				if (p.x() < left)
					left = p.x();
				if (p.x() > right)
					right = p.x();
				if (p.y() > top)
					top = p.y();
				if (p.y() < bottom)
					bottom = p.y();
				// expand
				if (rect().contains(p.x() + 1, transformY(p.y())))
					openTable.push_back(QPoint(p.x() + 1, p.y()));
				if (rect().contains(p.x() - 1, transformY(p.y())))
					openTable.push_back(QPoint(p.x() - 1, p.y()));
				if (rect().contains(p.x(), transformY(p.y() + 1)))
					openTable.push_back(QPoint(p.x(), p.y() + 1));
				if (rect().contains(p.x(), transformY(p.y() - 1)))
					openTable.push_back(QPoint(p.x(), p.y() - 1));
			}
		}
		cachePainter.end();
		repaint(left, transformY(top), right - left + 1, top - bottom + 1);
	}
}

void Scene::fill(int step)
{
	auto ET = constructET();

	// AEL just store a Node vector
	QVector<Node> AEL;
	int currentY;
	while (AEL.size() || ET.size())
	{
		if (!AEL.size())
		{
			// AEL is empty, move ET.first() to AEL
			AEL = ET.first();
			currentY = ET.firstKey();
			ET.remove(ET.firstKey());
			++currentY;
		}
		else
		{
			if (ET.size() && ET.firstKey() == currentY)
			{
				// merge ET.first() to AEL
				AEL.append(ET.first());
				std::sort(AEL.begin(), AEL.end());

				// remove ET.first()
				ET.remove(ET.firstKey());
			}

			// draw
			if (currentY >= 0 && currentY < HEIGHT && (step == 0 || currentY % (step + 1) == 0))
			{
				auto AEL_copy = AEL;
				// process extreme singularity points
				QVector<int> reachBorder; // -1 means lower border, 1 means upper border, 0 means normal
				for (int i = 0; i < AEL_copy.size(); ++i)
				{
					if (edges[AEL_copy[i].edgeIndex].upperPoint.y() == currentY)
					{
						reachBorder.push_back(1);
					}
					else if (edges[AEL_copy[i].edgeIndex].lowerPoint.y() == currentY)
					{
						reachBorder.push_back(-1);
					}
					else
					{
						reachBorder.push_back(0);
					}
				}
				bool flag = true; // means still processing extreme singularity points
				while (flag)
				{
					flag = false;
					for (int i = 0; i < reachBorder.size() - 1; ++i)
					{
						if (reachBorder[i] * reachBorder[i + 1] == -1)
						{
							if (reachBorder[i] == -1)
								++i; // i = i + 1, remove the lower edge
							reachBorder.remove(i);
							AEL_copy.remove(i);
							flag = true; // continue loop
							break;
						}
					}
				}

				while (AEL_copy.size())
				{
					// draw line according to the first 2 items in AEL
					for (int i = max(0, AEL_copy[0].x); i <= min(WIDTH - 1, AEL_copy[1].x); ++i)
					{
						permanent[currentY][i] = window->getBgColor();
					}
					// remove the first 2 items in AEL
					AEL_copy.pop_front();
					AEL_copy.pop_front();
				}
			}
			else if (currentY < 0 || currentY >= HEIGHT) // overflow
			{
				break;
			}

			// strip AEL
			for (int i = 0; i < AEL.size(); ++i)
			{
				if (AEL[i].yMax == currentY)
				{
					AEL.remove(i);
					--i;
				}
			}

			// get next x
			for (auto &node : AEL)
			{
				node.x += node.deltaX;
			}
			// sort AEL
			std::sort(AEL.begin(), AEL.end());

			++currentY;
		}
	}

	// repaint border
	for (int i = 0; i < edges.size(); ++i)
	{
		startX = edges[i].p1.x();
		startY = edges[i].p1.y();
		drawLine(edges[i].p2.x(), edges[i].p2.y());
		done();
	}

	refreshingPermanent = true;
	repaint();
}

QMap<int, QVector<Scene::Node>> Scene::constructET()
{
	QMap<int, QVector<Node>> ET;
	for (int i = 0; i < edges.size(); ++i)
	{
		// ignore horizontal edge
		if (edges[i].p1.y() == edges[i].p2.y())
			continue;

		// judge upperPoint & lowerPoint
		QPoint lowerPoint = (edges[i].p1.y() <= edges[i].p2.y()) ? edges[i].p1 : edges[i].p2;
		QPoint upperPoint = (edges[i].p1.y() <= edges[i].p2.y()) ? edges[i].p2 : edges[i].p1;
		edges[i].lowerPoint = lowerPoint;
		edges[i].upperPoint = upperPoint;

		// construct new Node
		Node node;
		node.yMax = upperPoint.y();
		node.x = lowerPoint.x();
		node.deltaX = (double)(lowerPoint.x() - upperPoint.x()) / (double)(lowerPoint.y() - upperPoint.y());
		node.edgeIndex = i;

		// link
		if (ET.contains(lowerPoint.y()))
		{
			// ordered by x asc; if equal, ordered by deltaX
			bool flag = true; // need to add to tail of ET
			for (int i = 0; i < ET[lowerPoint.y()].size(); ++i)
			{
				if (ET[lowerPoint.y()][i].x == node.x)
				{
					if (ET[lowerPoint.y()][i].deltaX > node.deltaX)
					{
						ET[lowerPoint.y()].insert(i, node);
						flag = false;
						break;
					}
				}
				else if (ET[lowerPoint.y()][i].x > node.x)
				{
					ET[lowerPoint.y()].insert(i, node);
					flag = false;
					break;
				}
			}
			if (flag)
			{
				ET[lowerPoint.y()].push_back(node);
			}
		}
		else
		{
			ET.insert(lowerPoint.y(), QVector<Node>());
			ET[lowerPoint.y()].push_back(node);
		}
	}
	return ET;
}

void Scene::clearTemp()
{
	// repaint only if start point or end point in canvas
	if (rect().contains(startX, transformY(startY)) || rect().contains(endX, transformY(endY)))
	{
		clearingTemp = true;
		repaint(min(startX, endX), transformY(max(startY, endY)), abs(startX - endX) + 1, abs(startY - endY) + 1);
	}

	temp.clear();
}

void Scene::drawTemp()
{
	// repaint only if start point or end point in canvas
	if (rect().contains(startX, transformY(startY)) || rect().contains(endX, transformY(endY)))
	{
		drawingTemp = true;
		repaint(min(startX, endX), transformY(max(startY, endY)), abs(startX - endX) + 1, abs(startY - endY) + 1);
	}
}

void Scene::swapTemp()
{
	for (int i = 0; i < temp.size(); ++i)
	{
		int t = temp[i].x;
		temp[i].x = temp[i].y;
		temp[i].y = t;
	}
}

void Scene::flipY(int centerY)
{
	for (int i = 0; i < temp.size(); ++i)
	{
		temp[i].y = 2 * centerY - temp[i].y;
	}
}

void Scene::paintEvent(QPaintEvent *e)
{
	QPainter cachePainter(cache);
	QPainter painter(this);
	if (!clearingTemp && !drawingTemp && !refreshingPermanent) // not need to refresh, use cache
	{
		painter.drawPixmap(e->rect(), *cache, e->rect());
	}
	else // refresh canvas and cache
	{
		if (refreshingPermanent)
		{
			refreshingPermanent = false;
			for (int x = e->rect().left(); x <= e->rect().right(); ++x)
			{
				for (int y = e->rect().bottom(); y >= e->rect().top(); --y)
				{
					painter.setPen(permanent[transformY(y)][x]);
					cachePainter.setPen(permanent[transformY(y)][x]);
					painter.drawPoint(x, y);
					cachePainter.drawPoint(x, y);
				}
			}
		}
		if (drawingTemp)
		{
			drawingTemp = false;
			for (int i = 0; i < temp.size(); ++i)
			{
				if (e->rect().contains(temp[i].x, transformY(temp[i].y)))
				{
					// using temp color
					painter.setPen(temp[i].color);
					cachePainter.setPen(temp[i].color);
					painter.drawPoint(temp[i].x, transformY(temp[i].y));
					cachePainter.drawPoint(temp[i].x, transformY(temp[i].y));
				}
			}
		}
		else // clearing temp
		{
			clearingTemp = false;
			for (int i = 0; i < temp.size(); ++i)
			{
				if (e->rect().contains(temp[i].x, transformY(temp[i].y)))
				{
					// using permanent color
					painter.setPen(permanent[temp[i].y][temp[i].x]);
					cachePainter.setPen(permanent[temp[i].y][temp[i].x]);
					painter.drawPoint(temp[i].x, transformY(temp[i].y));
					cachePainter.drawPoint(temp[i].x, transformY(temp[i].y));
				}
			}
		}
	}
}

void Scene::mousePressEvent(QMouseEvent *e)
{
	switch (window->getTool())
	{
	case MainWindow::PEN:
		startX = endX = e->x();
		startY = endY = transformY(e->y());
		setMouseTracking(true);
		break;
	case MainWindow::LINE:
		startX = endX = e->x();
		startY = endY = transformY(e->y());
		setMouseTracking(true);
		break;
	case MainWindow::RECT:
		startX = endX = e->x();
		startY = endY = transformY(e->y());
		setMouseTracking(true);
		break;
	case MainWindow::FLOOD:
		floodFill(e->x(), transformY(e->y()));
		break;
	case MainWindow::POLYGON:
		if (drawingPolygon)
		{
			if (e->button() == Qt::LeftButton)
			{
				// add an edge
				edges.push_back(Edge(QPoint(startX, startY), QPoint(e->x(), transformY(e->y()))));

				done();
				startX = e->x();
				startY = transformY(e->y());
			}
			else if (e->button() == Qt::RightButton)
			{
				drawingPolygon = false;
				setMouseTracking(false);
				clearTemp();
				// add the last edge
				drawLine(edges[0].p1.x(), edges[0].p1.y());
				done();
				edges.push_back(Edge(QPoint(startX, startY), QPoint(endX, endY)));

				// add shadow
				switch (window->getPolyFillType())
				{
				case MainWindow::SHADOW:
					fill(window->getShadowInterval());
					break;
				case MainWindow::COLOR:
					fill();
					break;
				default:
					break;
				}
			}
		}
		else // drawingPolygon == false
		{
			drawingPolygon = true;
			edges.clear();
			startX = endX = e->x();
			startY = endY = transformY(e->y());
			setMouseTracking(true);
		}
		break;
	default:
		break;
	}
}

void Scene::mouseMoveEvent(QMouseEvent *e)
{
	switch (window->getTool())
	{
	case MainWindow::PEN:
		done();
		startX = endX;
		startY = endY;
		drawLine(e->x(), transformY(e->y()));
		break;
	case MainWindow::LINE:
		drawLine(e->x(), transformY(e->y()));
		break;
	case MainWindow::RECT:
		drawRect(e->x(), transformY(e->y()));
		break;
	case MainWindow::POLYGON:
		drawLine(e->x(), transformY(e->y()));
		break;
	default:
		break;
	}
}

void Scene::mouseReleaseEvent(QMouseEvent *)
{
	switch (window->getTool())
	{
	case MainWindow::PEN:
		done();
		setMouseTracking(false);
		break;
	case MainWindow::LINE:
		done();
		setMouseTracking(false);
		break;
	case MainWindow::RECT:
		done();
		setMouseTracking(false);
		edges.clear();
		edges.push_back(Edge(QPoint(startX, startY), QPoint(startX, endY)));
		edges.push_back(Edge(QPoint(startX, startY), QPoint(endX, startY)));
		edges.push_back(Edge(QPoint(endX, endY), QPoint(startX, endY)));
		edges.push_back(Edge(QPoint(endX, endY), QPoint(endX, startY)));
		switch (window->getPolyFillType())
		{
		case MainWindow::SHADOW:
			fill(window->getShadowInterval());
			break;
		case MainWindow::COLOR:
			fill();
			break;
		default:
			break;
		}

		break;
	case MainWindow::POLYGON:
		break;
	default:
		setMouseTracking(false);
		break;
	}
}

#include "scene.h"
#include <QMap>
#include <QPainter>
#include <QDebug>
#include <QVector>
#include <QPoint>

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
	// init temp
	temp = new Temp[HEIGHT * WIDTH];
	for (int i = 0; i < HEIGHT * WIDTH; ++i)
	{
		temp[i].color = QColor(); // invalid color
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
	delete[] temp;
	delete[] permanent;
}

void Scene::done()
{
	// merge temp to permanent
	for (int i = 0; i < HEIGHT * WIDTH && temp[i].color.isValid(); ++i)
	{
		// judge whether current point is inside canvas
		if (temp[i].x >= 0 && temp[i].x < WIDTH && temp[i].y >= 0 && temp[i].y < HEIGHT)
			permanent[temp[i].y][temp[i].x] = temp[i].color;
		temp[i].color = QColor(); // set invalid
	}

	// fill rect with bgColor
	if (window->getTool() == MainWindow::RECT && window->getPolyFillType() == MainWindow::COLOR)
	{
		int left = max(min(startX, endX) + 1, 0);
		int right = min(max(startX, endX) - 1, WIDTH - 1);
		int bottom = max(min(startY, endY) + 1, 0);
		int top = min(max(startY, endY) - 1, HEIGHT - 1);
		for (int x = left; x <= right; ++x)
		{
			for (int y = bottom; y <= top; ++y)
			{
				permanent[y][x] = window->getBgColor();
			}
		}
		refreshingPermanent = true;
		repaint(left, transformY(top), right - left + 1, top - bottom + 1);
	}

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
	// check x1, y1, x2, y2, temp
	if (temp[0].color.isValid())
	{
		qDebug() << "Scene::BresenhamLine: temp is not empty";
		return;
	}
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
		temp[i - x1].x = i;
		temp[i - x1].y = currentY;
		temp[i - x1].color = window->getFgColor();
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
				flipY();
			}
			else
			{
				// -infinite < gradient < -1
				BresenhamLine(y1, x1, 2 * y1 - y2, x2);
				swapTemp();
				flipY();
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
				flipY(false);
			}
			else
			{
				// -infinite < gradient < -1
				BresenhamLine(y2, x2, 2 * y2 - y1, x1);
				swapTemp();
				flipY(false);
			}
		}
	}
}

void Scene::drawRect(int x, int y)
{
	clearTemp();

	endX = x;
	endY = y;
	int index = 0; // index of temp

	// draw rect border
	for (int i = min(x, startX); i <= max(x, startX); ++i)
	{
		temp[index].x = i;
		temp[index].y = startY;
		temp[index].color = window->getFgColor();
		++index;
		temp[index].x = i;
		temp[index].y = y;
		temp[index].color = window->getFgColor();
		++index;
	}
	for (int i = min(y, startY); i <= max(y, startY); ++i)
	{
		temp[index].x = startX;
		temp[index].y = i;
		temp[index].color = window->getFgColor();
		++index;
		temp[index].x = x;
		temp[index].y = i;
		temp[index].color = window->getFgColor();
		++index;
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

void Scene::getShadow()
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
		}
		else
		{
			if (ET.firstKey() == currentY)
			{
				// merge ET.first() to AEL
				int i = 0, j = 0;
				while (i < AEL.size() && j < ET.first().size())
				{
					if (AEL[i].x == ET.first()[j].x)
					{
						if (AEL[i].deltaX > ET.first()[j].deltaX)
						{
							AEL.insert(i, ET.first()[j]);
							++j;
							i += 2;
						}
						else
						{
							++i;
							++j;
						}
					}
					else if (AEL[i].x > ET.first()[j].x)
					{
						AEL.insert(i, ET.first()[j]);
						++j;
						i += 2;
					}
					else
					{
						++i;
						++j;
					}
				}
				// remove ET.first()
				ET.remove(ET.firstKey());
			}

			// draw
			if (currentY >= 0 && currentY < HEIGHT)
			{
				while (AEL.size())
				{
					// draw line according to the first 2 items in AEL
					for (int i = max(0, AEL[0].x); i <= min(WIDTH - 1, AEL[1].x); ++i)
					{
						permanent[currentY][i] = window->getBgColor();
					}
					// remove the first 2 items in AEL
					AEL.pop_front();
					AEL.pop_front();
				}
			}
			else
			{
				break;
			}

			// strip AEL
			for (int i = 0; i < AEL.size(); ++i)
			{
				if (AEL[i].yMax == currentY)
				{
					AEL.remove(i);
				}
			}

			// get next x
			for (auto &node : AEL)
			{
				node.x += node.deltaX;
			}
		}
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

		// construct new Node
		Node node;
		node.yMax = upperPoint.y();
		node.x = lowerPoint.x();
		node.deltaX = (double)(lowerPoint.x() - upperPoint.x()) / (double)(lowerPoint.y() - upperPoint.y());

		// link
		if (ET.contains(lowerPoint.y()))
		{
			// ordered by x asc; if equal, ordered by deltaX
			for (int i = 0; i < ET[lowerPoint.y()].size(); ++i)
			{
				bool flag = true; // need to add to tail of ET
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
			ET[lowerPoint.y].push_back(node);
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

	for (int i = 0; i < HEIGHT * WIDTH && temp[i].color.isValid(); ++i)
	{
		temp[i].color = QColor(); // set invalid
	}
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
	for (int i = 0; i < WIDTH * HEIGHT && temp[i].color.isValid(); ++i)
	{
		int t = temp[i].x;
		temp[i].x = temp[i].y;
		temp[i].y = t;
	}
}

void Scene::flipY(bool usingStartY)
{
	for (int i = 0; i < WIDTH * HEIGHT && temp[i].color.isValid(); ++i)
	{
		if (usingStartY)
			temp[i].y = 2 * startY - temp[i].y;
		else
			temp[i].y = 2 * endY - temp[i].y;
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
			for (int i = 0; i < HEIGHT * WIDTH && temp[i].color.isValid(); ++i)
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
			for (int i = 0; i < HEIGHT * WIDTH && temp[i].color.isValid(); ++i)
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
		break;
	case MainWindow::LINE:
		done();
		break;
	case MainWindow::RECT:
		done();
		break;
	default:
		break;
	}

	setMouseTracking(false);
}

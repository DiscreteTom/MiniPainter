#include "scene.h"
#include <QPainter>
#include <QDebug>

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

	setAttribute(Qt::WA_OpaquePaintEvent); // enable paint without erase
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
	// need not to refresh, just merge temp to permanent
}

void Scene::drawLine(int x, int y)
{
	clearTemp();

	endX = x;
	endY = y;

	if (startX <= endX)
	{
		if (startY <= endY)
		{
			if (abs(startX - endX) >= abs(startY - endY))
			{
				// 0 <= gradient <= 1
				BresenhamLine(startX, startY, endX, endY);
			}
			else
			{
				// 1 < gradient < infinite
				BresenhamLine(startY, startX, endY, endX);
				swapTemp();
			}
		}
		else
		{
			if (abs(startX - endX) >= abs(startY - endY))
			{
				// -1 <= gradient < 0
				BresenhamLine(startX, startY, endX, 2 * startY - endY);
				flipY();
			}
			else
			{
				// -infinite < gradient < -1
				BresenhamLine(startY, startX, 2 * startY - endY, endX);
				swapTemp();
				flipY();
			}
		}
	}
	else
	{
		// exchange start and end
		if (endY <= startY)
		{
			if (abs(endX - startX) >= abs(endY - startY))
			{
				// 0 <= gradient <= 1
				BresenhamLine(endX, endY, startX, startY);
			}
			else
			{
				// 1 < gradient < infinite
				BresenhamLine(endY, endX, startY, startX);
				swapTemp();
			}
		}
		else
		{
			if (abs(endX - startX) >= abs(endY - startY))
			{
				// -1 <= gradient < 0
				BresenhamLine(endX, endY, startX, 2 * endY - startY);
				flipY(false);
			}
			else
			{
				// -infinite < gradient < -1
				BresenhamLine(endY, endX, 2 * endY - startY, startX);
				swapTemp();
				flipY(false);
			}
		}
	}

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

void Scene::clearTemp()
{
	clearingTemp = true;

	repaint(min(startX, endX), transformY(max(startY, endY)), abs(startX - endX) + 1, abs(startY - endY) + 1);

	for (int i = 0; i < HEIGHT * WIDTH && temp[i].color.isValid(); ++i)
	{
		temp[i].color = QColor(); // set invalid
	}
}

void Scene::drawTemp()
{
	drawingTemp = true;
	repaint(min(startX, endX), transformY(max(startY, endY)), abs(startX - endX) + 1, abs(startY - endY) + 1);
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
	if (!permanentChanged && !clearingTemp && !drawingTemp) // not need to refresh, use cache
	{
		painter.drawPixmap(0, 0, *cache);
	}
	else // refresh cancas and cache
	{
		if (permanentChanged)
		{
			permanentChanged = false;
			for (int x = e->rect().left(); x <= e->rect().right(); ++x)
			{
				for (int y = e->rect().top(); y <= e->rect().bottom(); ++y)
				{
					painter.setPen(permanent[transformY(y)][x]);
					cachePainter.setPen(permanent[transformY(y)][x]);
					painter.drawPoint(x, y);
					cachePainter.drawPoint(x, y);
				}
			}
		}
		else if (drawingTemp)
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
		else if (clearingTemp)
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
		break;
	case MainWindow::LINE:
		startX = endX = e->x();
		startY = endY = transformY((e->y()));
		break;
	default:
		break;
	}

	setMouseTracking(true);
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
	default:
		break;
	}

	setMouseTracking(false);
}

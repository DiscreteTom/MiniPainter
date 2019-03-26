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
	temp = new QColor *[HEIGHT];
	for (int i = 0; i < HEIGHT; ++i)
	{
		temp[i] = new QColor[WIDTH]; // invalid color
	}

	// init cache
	cache = new QPixmap(WIDTH, HEIGHT);
}

Scene::~Scene()
{
	for (int i = 0; i < HEIGHT; ++i)
	{
		delete[] temp[i];
		delete[] permanent[i];
	}
	delete[] temp;
	delete[] permanent;
}

void Scene::done()
{
	// merge temp to permanent
	for (int i = 0; i < HEIGHT; ++i)
	{
		for (int j = 0; j < WIDTH; ++j)
		{
			if (temp[i][j].isValid())
			{
				permanent[i][j] = temp[i][j];
				temp[i][j] = QColor(); // set invalid
			}
		}
	}
}

void Scene::paintEvent(QPaintEvent *e)
{
	if (!refresh) // not need to refresh, use cache
	{
		QPainter painter(this);
		painter.drawPixmap(0, 0, *cache);
	}
	else // refresh cache and paint
	{
		QPainter cachePainter(cache);
		QPainter painter(this);
		for (int x = e->rect().x(); x < e->rect().x() + e->rect().width(); ++x)
		{
			for (int y = e->rect().y(); y < e->rect().y() + e->rect().height(); ++y)
			{
				if (temp[transformY(y)][x].isValid())
				{
					painter.setPen(temp[transformY(y)][x]);
					cachePainter.setPen(temp[transformY(y)][x]);
				}
				else
				{
					painter.setPen(permanent[transformY(y)][x]);
					cachePainter.setPen(permanent[transformY(y)][x]);
				}
				painter.drawPoint(x, y);
				cachePainter.drawPoint(x, y);
			}
		}
		refresh = false;
	}
}

void Scene::mousePressEvent(QMouseEvent *)
{
	setMouseTracking(true);
}

void Scene::mouseMoveEvent(QMouseEvent *e)
{
	permanent[transformY(e->y())][e->x()] = window->getFgColor();
	refresh = true;
	update(e->x(), e->y(), 1, 1);
}

void Scene::mouseReleaseEvent(QMouseEvent *)
{
	setMouseTracking(false);
}

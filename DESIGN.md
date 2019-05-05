# DESIGN

## 环境

基于QT框架，使用QT按钮、单选框等控件实现交互。

画图相关函数把每个计算出来的像素点保存在permanent和temp两个数据结构中，仅调用QT的drawPoint函数逐个点绘制。

## 交互设计

用户可操作区域分为四个部分：
- 工具选择区
- 填充设置区
- 颜色选择区
- 绘图区

### 工具选择区

用户可以在此区域选择绘图工具。提供的绘图工具包括：
- 铅笔(Pen)
- 直线(Line)
- 矩形(Rectangle)
- 椭圆(Ellipse)
- 多边形(Polygon)
- 油漆桶(Flood Fill)

使用QRadioButton单选框实现控件的交互（选择与取消），使用QGroupBox实现每次只有一个工具被选择

### 填充设置区

多边形、椭圆和矩形可以设置内部填充模式。填充模式分为三种：
- 阴影填充
- 纯色填充
- 不填充

填充模式同样使用QRadioButton实现单选的交互功能。

用户选择阴影填充或纯色填充时，画出的图形将使用**背景色**进行对应的填充

用户还可以使用一个QSpinBox设置阴影填充时阴影的间隔

### 颜色选择区

包含两个按钮，分别是前景色选择按钮和背景色选择按钮。点击后会弹出颜色选择框。按钮颜色使用QSS控制，颜色选择框使用QT内置的QColorDialog实现。

### 绘图区

用户使用鼠标绘图的地方。不同的工具会有不同的交互方式，具体可以参考windows画图的交互方式。

## 内部数据结构设计

### MainWindow

MainWindow负责提供交互所需的控件。

设计了枚举类`Tool`和`PolyFillType`用来给画布提供交互控件的状态。

`QColor`类型的成员变量`fgColor`和`bgColor`用来保存当前选中的前景色和背景色。

### Scene

Scene类是画布类。内部使用了一个二维数组`permanent`保存了所有**已经画在画布上**的点。

定义结构体`Temp`，保存用户**正在画，但是没有确定画在画布上**的内容（如确定了直线的起点而没有确定终点，那么画布上的直线是临时的）。`Temp`包含三个成员变量，分别是`color`记录颜色，`x`和`y`记录临时像素的坐标。这样的设计允许`temp`中出现越界的坐标。

设置成员变量`temp`为`Temp`类型的向量，以便进行插入与删除操作。

为了提高画图效率，设置了`QPixmap`类型的`cache`变量，用来在画布没有被更改而需要重绘的时候（比如改变窗口大小）重绘画布。

设置`bool`类型的变量`clearingTemp`、`drawingTemp`、`refreshingPermanent`表示当前的paintEvent需要根据哪些内容进行刷新，这些在下文实现交互的内容中将会提到。

设置变量`drawingPolygon`实现绘制多边形时的交互，将在后文提到。

设置成员变量`startX`、`endX`、`startY`、`endY`实现跨函数的信息传递。在绘制不同的图形时这些坐标的含义可能不同。

设置成员变量`edges`保存当前的所有边。用于多边形、矩形、圆形的填充。

## 交互的实现

### 直线

检测到mousePressEvent的时候记录直线起点的坐标并启动鼠标跟踪。每次鼠标移动都会刷新`temp`中的内容，在`temp`中写入用户画出的临时线段的坐标与颜色，然后调用`drawTemp`函数同步到画布上。

鼠标松开时关闭鼠标跟踪，并记录结束点。把起点与结束点作为线段的两个端点得到`temp`，然后调用`done`函数把`temp`的内容同步到`permanent`里面并清除`temp`数组。

画直线使用Bresenhan算法。定义函数`BresenhamLine(x1, y1, x2, y2)`，仅实现$x1\le x2$且$y1\le y2$且$\left| x1-x2\right| \le \left|y1-y2\right|$的时候直线的生成（即斜率0-1且左下角为起始点，右上角为结束点），代码如下：

```cpp
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
```

生成的点会保存在`temp`数组中。然后函数`getLine`函数会对`BresenhamLine`得到的`temp`数组进行各种反转与对称操作，代码如下：

```cpp
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
```

这样`temp`数组中就可以得到正确的点集。所以在`drawLine`函数中只需要进行如下操作就可以绘制出直线：

```cpp
void Scene::drawLine(int x, int y)
{
	clearTemp();

	endX = x;
	endY = y;

	getLine(startX, startY, x, y);

	drawTemp();
}
```

### 铅笔

在检测到`mousePressEvent`的时候记录起始点并启动鼠标跟踪。每隔一段很短的时间检测到`mouseMoveEvent`的时候，记录当时的坐标作为结束点，然后在起始点和结束点之间调用`drawLine`函数画线段，并调用`done`函数把`temp`同步到`permanent`中，然后把当前结束点作为起始点开启下一轮直线段的绘制。检测到`mouseReleaseEvent`的时候绘制最后一个直线段即可。

因为调用了`drawLine`绘制线段，所以画出来的线不会断。因为临时数据保存在`temp`数组中，而`temp`数组向`permanent`数组合并的时候会判断点是否在画布内，这样就允许越界笔划的存在。

### 漫水填充

Flood Fill工具使用漫水法进行填充。使用vector数据类型模拟递归对4-连通区域进行漫水判断以实现漫水法填充。代码如下：

```cpp
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
```

### 多边形填充

多边形的填充或阴影填充使用多边形扫描转换算法实现，根据`Scene::edges`中的边在`temp`数组中生成相应的点，然后调用`drawTemp`画到画布上。

与课本上的算法不同，对**非极值奇点**的处理放到了画图的过程中。算法步骤如下：

1. 建立ET，数据结构为`map<int, vector<Node>>`，其中`int`为此链所在的`y`坐标，`vector<Node>`为节点链表。这样就不必为所有`y`坐标设置节点链表，提升效率，压缩空间。
2. 初始化AEL
3. 如果AEL和ET都不为空，到4。否则到11
4. 如果AEL为空，直接把ET的第一项移动到AEL。否则如果ET不为空而且当前扫描的`y`为ET的最小`y`则合并ET的第一个链和AEL
5. 如果当前`y`没有越界且如果绘制阴影的话当前`y`能够整除阴影的间隔，到6。否则转到9
6. 设置`reachBorder`变量保存正在扫描的边是否到达上界或者下界。1表示到达上界，-1表示到达下界。设置`AEL_copy`变量以便临时对AEL进行操作。计算`reachBorder`变量
7. 循环扫描`reachBorder`，如果出现两个相邻的值乘积为-1则表示出现了**非极值奇点**，在`AEL_copy`和`reachBorder`中删除此节点
8. 经过上一步处理，此时`AEL_copy`长度一定是偶数。每次取两个进行阴影绘制
9. 删除AEL中到达边界上界的点
10. 计算AEL中所有节点的x并进行排序。转到3
11. 为了防止阴影覆盖边界，根据`edges`重绘边界

代码如下：

```cpp
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
```

### 矩形

`drawRect`函数中没有调用`drawLine`函数。直接把矩形轮廓保存在`temp`数组中。同样，此举允许越界像素的存在。

为了提高刷新效率，`drawRect`函数内部不提供填充功能，用户在画矩形的时候只能看到矩形轮廓，松开鼠标后才能看到填充效果。所以填充效果在`mouseReleaseEvent`里面实现。

### 多边形

多边形的交互方式：鼠标左键依次点击生成多边形的顶点，最后鼠标右键点击以闭合多边形。每次点击生成的边都会保存在`edges`中，然后通过调用`fill`函数进行多边形扫描转换填充。

第一次鼠标点击时会把标志变量`drawingRect`置为`true`，鼠标右键点击时把此变量置为`false`并停止多边形的绘制。

### 圆与椭圆

使用多边形逼近算法进行绘制。把椭圆划分为360份，使用递推公式进行计算。递推公式如下：

$$
x[i]=a\cos(?+\theta)\\
y[i] = b\sin(?+\theta)\\
x[i+1]=a\cos(?+2\theta)
=a\cos(?+\theta)\cos\theta-a\sin(?+\theta)\sin\theta
=x[i]\cos\theta-\frac{a}{b}y[i]\sin\theta\\
y[i+1]=b\sin(?+2\theta)
=b\sin(?+\theta)\cos\theta+b\cos(?+\theta)\sin\theta
=y[i]\cos\theta+\frac{b}{a}x[i]\sin\theta
$$

根据递推公式就可以只计算一次`sin`值和`cos`值进行计算。得到所需要的点后按顺序转换为边并保存在`edges`中，然后调用`fill`函数即可像填充多边形一样填充椭圆。

为了提高绘图效率，绘制圆和椭圆时仅绘制矩形轮廓，松开鼠标后才会显示椭圆及其填充。绘制椭圆代码如下：

```cpp
void Scene::drawEllipse(int x, int y)
{
	clearTemp();

	endX = x;
	endY = y;

	// using Polygon Approximation Method, edges of polygon is 360
	double a = abs(startX - endX) / 2;
	double b = abs(startY - endY) / 2;
	QPoint center = QPoint((x + startX) / 2, (y + startY) / 2);
	int n = 360; // edges' number of polygon

	edges.clear();
	double sin = qSin(qDegreesToRadians(double(360 / n)));
	double cos = qCos(qDegreesToRadians(double(360 / n)));
	double currentX = a;
	double currentY = 0;
	for (int i = 0; i < n; ++i)
	{
		double nextX = currentX * cos - a / b * currentY * sin;
		double nextY = currentY * cos + b / a * currentX * sin;
		edges.push_back(Edge(QPoint(currentX + center.x(), currentY + center.y()), QPoint(nextX + center.x(), nextY + center.y())));
		currentX = nextX;
		currentY = nextY;
	}
	// add the last edge
	edges.push_back(Edge(QPoint(currentX + center.x(), currentY + center.y()), edges[0].p1));

	// put all lines in temp
	QVector<Temp> result;
	for (int i = 0; i < edges.size(); ++i)
	{
		getLine(edges[i].p1.x(), edges[i].p1.y(), edges[i].p2.x(), edges[i].p2.y());
		result.append(temp);
	}
	temp = result;

	drawTemp();
}
```

### 重绘事件paintEvent

由用户画图引起的重绘事件有三种：
- drawingTemp
- clearingTemp
- refreshingPermanent

这三个事件都已经定义为Scene类的私有bool型变量。一旦事件发生就会调用`repaint`函数立即重绘。

Scene类的构造函数中添加`setAttribute(Qt::WA_OpaquePaintEvent)`可以实现仅重绘矩形空间内的部分点而不清除原有的点，这样可以提升绘图效率。比如绘制一条直线，本来需要刷新整个矩形空间，现在只需要刷新直线所在的点即可。

`drawingTemp`是绘制用户正在画的临时图像的函数。绘制时优先绘制`temp`而不是`permanent`。`clearingTemp`则是清除用户正在画的临时图像，使用`permanent`的点绘制在`temp`的坐标处。这样可以实现像素级局部刷新。`refreshingPermanent`用于多边形填充这样的直接修改`permanent`的颜色值造成的刷新。

所有绘制操作同时绘制在UI画布和内部的cache上。在其他刷新画布的情况（比如窗口大小改变）时直接使用cache进行绘制以提高效率。

代码如下：

```cpp
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
```

可以看到，除了为了高效率刷新而调用的`drawPixmap`函数，其他情况都是调用`drawPoint`函数根据`temp`数组和`permanent`数组里面的数据一个点一个点画出来的。
# DESIGN

## 环境

基于QT框架，使用QT按钮、单选框等控件实现交互。

画图相关仅调用了QT的drawPoint函数一个点一个点绘制 。

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

用户还可以使用两个QSpinBox设置阴影填充时阴影的倾斜角度和间隔

### 颜色选择区

包含两个按钮，分别是前景色选择按钮和背景色选择按钮。点击后会弹出颜色选择框。

### 绘图区

用户使用鼠标绘图的地方。不同的工具会有不同的交互方式，具体可以参考windows画图的交互方式。

## 内部数据结构设计

### MainWindow

MainWindow负责提供交互所需的控件。

设计了枚举类`Tool`和`PolyFillType`用来给画布提供交互控件的状态。

`QColor`类型的成员变量`fgColor`和`bgColor`用来保存当前选中的前景色和背景色。

### Scene

Scene类是画布类。内部使用了一个二维数组`permanent`保存了所有已经画在画布上的点。

定义结构体`Temp`，保存用户正在画，但是没有确定画在画布上的内容（如确定了直线的起点而没有确定终点，那么画布上的直线是临时的）。`Temp`包含三个成员变量，分别是`color`记录颜色，`x`和`y`记录临时像素的坐标。这样的设计允许`temp`中出现越界的坐标。

设置成员变量`temp`为`Temp`类型的一位数组，最大长度为画布的长度乘以画布的宽度。

为了提高画图效率，设置了`QPixmap`类型的`cache`变量，用来在画布没有被更改而需要重绘的时候（比如改变窗口大小）重绘画布。

设置`bool`类型的变量`clearingTemp`、`drawingTemp`、`refreshingPermanent`表示当前的paintEvent需要根据哪些内容进行刷新，这些在下文实现交互的内容中将会提到。

## 交互的实现

### 直线

检测到mousePressEvent的时候记录直线起点的坐标并启动鼠标跟踪。每次鼠标移动都会刷新`temp`中的内容，在`temp`中写入用户画出的临时线段的坐标与颜色，然后调用`drawTemp`函数同步到画布上。

鼠标松开时关闭鼠标跟踪，并记录结束点。把起点与结束点作为线段的两个端点得到`temp`，然后调用`done`函数把`temp`的内容同步到`permanent`里面并清除`temp`数组。

画直线使用Bresenhan算法。定义函数`BresenhamLine(x1, y1, x2, y2)`，仅实现$x1\le x2$且$y1\le y2$且$\left| x1-x2\right| \le \left|y1-y2\right|$的时候直线的生成（即斜率0-1且左下角为起始点，右上角为结束点），代码如下：

```c++
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
```

这样`temp`数组中就可以得到正确的点集

### 铅笔

在检测到`mousePressEvent`的时候记录起始点并启动鼠标跟踪。每隔一段很短的时间检测到`mouseMoveEvent`的时候，记录当时的坐标作为结束点，然后在起始点和结束点之间调用`drawLine`函数画线段，并调用`done`函数把`temp`同步到`permanent`中，然后把当前结束点作为起始点开启下一轮直线段的绘制。检测到`mouseReleaseEvent`的时候绘制最后一个直线段即可。

因为调用了`drawLine`绘制线段，所以画出来的线不会断。因为临时数据保存在`temp`数组中，而`temp`数组向`permanent`数组合并的时候会判断点是否在画布内，这样就允许越界笔划的存在。

### 矩形

`drawRect`函数中没有调用`drawLine`函数。直接把矩形轮廓保存在`temp`数组中。同样，此举允许越界像素的存在。

为了提高刷新效率，`drawRect`函数内部不提供填充功能，用户在画矩形的时候只能看到矩形轮廓，松开鼠标后才能看到填充效果。填充效果在`done`里面实现。

### 填充

填充在`done`函数里面实现。用户在松开鼠标确定图案形状之后才会进行填充。在`done`函数合并`temp`数组到`permanent`数组的基础上，填充直接修改`permanent`值，以便更快地刷新。




### 多边形

### 圆与椭圆

### 重绘事件paintEvent

由用户画图引起的重绘事件有三种：
- drawingTemp
- clearingTemp
- refreshingPermanent

这三个事件都已经定义为Scene类的私有bool型变量。一旦事件发生就会调用`repaint`函数立即重绘。

Scene类的构造函数中添加`setAttribute(Qt::WA_OpaquePaintEvent)`可以实现仅重绘矩形空间内的部分点而不清除原有的点，这样可以提升绘图效率。比如绘制一条直线，本来需要刷新整个矩形空间，现在只需要刷新直线所在的点即可。

`drawingTemp`是绘制用户正在画的临时图像的函数。绘制时优先绘制`temp`而不是`permanent`。`clearingTemp`则是清除用户正在画的临时图像，使用`permanent`的点绘制在`temp`的坐标处。这样可以实现像素级局部刷新。`refreshingPermanent`用于多边形填充这样的，在`done`函数里面直接修改`permanent`的颜色值造成的刷新。

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
		if (refreshingPermanent){
			refreshingPermanent = false;
			for (int x = e->rect().left(); x <= e->rect().right(); ++x){
				for (int y = e->rect().bottom(); y >= e->rect().top(); --y){
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
```

可以看到，除了为了高效率刷新而调用的`drawPixmap`函数，其他情况都是调用`drawPoint`函数根据`temp`数组和`permanent`数组里面的数据一个点一个点画出来的。
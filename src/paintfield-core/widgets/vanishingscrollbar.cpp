#include <QtGui>
#include "vanishingscrollbar.h"

VanishingScrollBar::VanishingScrollBar(Qt::Orientation orientation, QWidget *parent) :
    QAbstractSlider(parent)
{
	setOrientation(orientation);
	onOrientationChanged();
}

QSize VanishingScrollBar::sizeHint() const
{
	constexpr int side = 2 * barMargin() + barWidth();
	return QSize(side, side);
}

void VanishingScrollBar::sliderChange(SliderChange change)
{
	if (change == SliderOrientationChange)
		onOrientationChanged();
	
	update();
}

void VanishingScrollBar::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	
	double begin, end;
	std::tie(begin, end) = scrollBarBeginEndPos(value(), minimum(), maximum(), pageStep());
	_barRect = scrollBarRect(begin, end, rect(), barMargin(), orientation());
	auto path = scrollBarPath(_barRect, orientation());
	
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::NoPen);
	painter.setBrush(Qt::black);
	
	painter.drawPath(path);
}

void VanishingScrollBar::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && _barRect.contains(event->pos()))
	{
		_isDragged = true;
		_dragStartPos = scrollPos(event->pos(), orientation());
		_dragStartValue = value();
		
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void VanishingScrollBar::mouseMoveEvent(QMouseEvent *event)
{
	if (_isDragged)
	{
		int validLen;
		
		if (orientation() == Qt::Horizontal)
			validLen = geometry().width() - 2 * barMargin();
		else
			validLen = geometry().height() - 2 * barMargin();
		
		double diff = double(scrollPos(event->pos(), orientation()) - _dragStartPos) / double(validLen);
		setValue(_dragStartValue + diff * (maximum() - minimum() + pageStep()));
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void VanishingScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
	if (_isDragged)
	{
		_isDragged = false;
		event->accept();
	}
	else
	{
		event->ignore();
	}
}

void VanishingScrollBar::onOrientationChanged()
{
	resize(sizeHint());
	setSizePolicy(sizePolicyForOrientation(orientation()));
}

// static functions

int VanishingScrollBar::scrollPos(const QPoint &mousePos, Qt::Orientation orientation)
{
	if (orientation == Qt::Horizontal)
		return mousePos.x();
	else
		return mousePos.y();
}

QSizePolicy VanishingScrollBar::sizePolicyForOrientation(Qt::Orientation orientation)
{
	if (orientation == Qt::Horizontal)
		return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	else
		return QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

std::tuple<double, double> VanishingScrollBar::scrollBarBeginEndPos(int value, int min, int max, int pageStep)
{
	double len = max - min + pageStep;
	double begin = double(value - min) / len;
	double end = double(value + pageStep - min) / len;
	
	return std::make_tuple(begin, end);
}

QRect VanishingScrollBar::scrollBarRect(double begin, double end, const QRect &rect, int margin, Qt::Orientation orientation)
{
	auto validRect = QRect(rect.x() + margin, rect.y() + margin, rect.width() - 2 * margin, rect.height() - 2 * margin);
	auto drawRect = validRect;
	
	if (orientation == Qt::Horizontal)
	{
		drawRect.setLeft(validRect.left() + validRect.width() * begin);
		drawRect.setWidth(validRect.width() * (end - begin));
	}
	else
	{
		drawRect.setTop(validRect.top() + validRect.height() * begin);
		drawRect.setHeight(validRect.height() * (end - begin));
	}
	
	return drawRect;
}

QPainterPath VanishingScrollBar::scrollBarPath(const QRect &rect, Qt::Orientation orientation)
{
	double radius;
	
	if (orientation == Qt::Horizontal)
		radius = rect.height() * 0.5;
	else
		radius = rect.width() * 0.5;
	
	auto path = QPainterPath();
	path.addRoundedRect(rect, radius, radius);
	return path;
}


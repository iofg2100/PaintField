#include <QtGui>
#include "fsscopedtimer.h"
#include "mlcurvesubdivision.h"
#include "fscore.h"

#include "fsbrushstroker.h"

FSStroker::FSStroker(MLSurface *surface, const FSBrushSetting *setting) :
	_surface(surface),
	_setting(setting)
{
}

void FSStroker::moveTo(const FSTabletInputData &data)
{
	_argb = fsPaletteManager()->currentColor().toArgb();
	_lastEditedKeys.clear();
	
	_count = 1;
	_dataEnd = data;
}

void FSStroker::lineTo(const FSTabletInputData &data)
{
	_count++;
	
	
	if (_count == 4)
	{
		drawFirst(data);
	}
	
	if (_count > 3)
	{
		MLPolygon polygon = MLCurveSubdivision(MLCurve4::fromCatmullRom(_posPrevious, _dataStart.pos, _dataEnd.pos, data.pos)).polygon();
		drawInterval(polygon, _dataStart, _dataEnd);
	}
	
	if (_count > 2)
		_posPrevious = _dataStart.pos;
	
	_dataStart = _dataEnd;
	_dataEnd = data;
}

void FSStroker::end()
{
}

void FSStroker::addEditedKeys(const QPointSet &keys)
{
	_lastEditedKeys |= keys;
	_totalEditedKeys |= keys;
}

MLPolygon mlTangentQuadrangle(double radius1, const MLVec2D &center1, double radius2, const MLVec2D &center2)
{
	double r1, r2;
	MLVec2D k1, k2, k2k1;
	
	if (radius1 < radius2)
	{
		r1 = radius1;
		r2 = radius2;
		k1 = center1;
		k2 = center2;
	}
	else
	{
		r1 = radius2;
		r2 = radius1;
		k1 = center2;
		k2 = center1;
	}
	
	k2k1 = k1 - k2;
	
	double dd = mlSqLength(k2k1);
	
	double dcos = r2 - r1;
	double dsin = sqrt(dd - dcos * dcos);
	
	MLVec2D cs(dcos, dsin);
	MLVec2D sc(dsin, dcos);
	MLVec2D cns(dcos, -dsin);
	MLVec2D nsc(-dsin, dcos);
	
	double f1 = r1 / dd;
	double f2 = r2 / dd;
	
	MLVec2D k1p1, k1q1, k2p2, k2q2;
	
	k1p1.x = mlDot(cns, k2k1);
	k1p1.y = mlDot(sc, k2k1);
	k1p1 *= f1;
	
	k1q1.x = mlDot(cs, k2k1);
	k1q1.y = mlDot(nsc, k2k1);
	k1q1 *= f1;
	
	k2q2.x = mlDot(cns, k2k1);
	k2q2.y = mlDot(sc, k2k1);
	k2q2 *= f2;
	
	k2p2.x = mlDot(cs, k2k1);
	k2p2.y = mlDot(nsc, k2k1);
	k2p2 *= f2;
	
	MLPolygon poly(4);
	
	poly[0] = k1 + k1p1;
	poly[1] = k2 + k2q2;
	poly[2] = k2 + k2p2;
	poly[3] = k1 + k1q1;
	
	return poly;
}

QVector<double> mlCountLength(const MLPolygon &polygon, double *totalLength)
{
	double total = 0;
	
	int count = polygon.size() - 1;
	if (count < 0)
		return QVector<double>();
	
	QVector<double> lengths(count);
	
	for (int i = 0; i < count; ++i)
	{
		double length = mlLength(polygon.at(i+1) - polygon.at(i));
		total += length;
		lengths[i] = length;
	}
	
	*totalLength = total;
	return lengths;
}

void FSPenStroker::drawFirst(const FSTabletInputData &data)
{
	double radius = data.pressure * setting()->diameter * 0.5;
	
	MLPainter painter(surface());
	painter.setArgb(argb());
	
	painter.drawEllipse(data.pos, radius, radius);
	
	_radiusPrev = radius;
}

void FSPenStroker::drawInterval(const MLPolygon &polygon, const FSTabletInputData &dataStart, const FSTabletInputData &dataEnd)
{
	int count = polygon.size() - 1;
	if (count < 1)
		return;
	
	double totalLength;
	QVector<double> lengths = mlCountLength(polygon, &totalLength);
	
	double radiusNormalized = (dataEnd.pressure - dataStart.pressure) / totalLength * setting()->diameter * 0.5;
	double radius = dataStart.pressure * setting()->diameter * 0.5;
	
	MLSurfacePainter painter(surface());
	painter.setArgb(argb());
	
	double radiusPrev = _radiusPrev;
	
	for (int i = 0; i < count; ++i)
	{
		radius += radiusNormalized * lengths[i];
		
		painter.drawPolygon(mlTangentQuadrangle(radiusPrev, polygon[i], radius, polygon[i+1]));
		painter.drawEllipse(polygon[i+1], radius, radius);
		
		radiusPrev = radius;
	}
	
	_radiusPrev = radiusPrev;
	
	painter.flush();
	
	addEditedKeys(painter.editedKeys());
}

void FSBrushStroker::drawFirst(const FSTabletInputData &data)
{
	drawDab(data);
	_carryOver = 1;
}

void FSBrushStroker::drawInterval(const MLPolygon &polygon, const FSTabletInputData &dataStart, const FSTabletInputData &dataEnd)
{
	int count = polygon.size() - 1;
	if (count < 1)
		return;
	
	double totalLength;
	QVector<double> lengths = mlCountLength(polygon, &totalLength);
	
	double totalNormalizeFactor = 1.0 / totalLength;
	
	double pressureNormalized = (dataEnd.pressure - dataStart.pressure) * totalNormalizeFactor;
	double rotationNormalized = (dataEnd.rotation - dataStart.rotation) * totalNormalizeFactor;
	double tangentialPressureNormalized = (dataEnd.tangentialPressure - dataStart.tangentialPressure) * totalNormalizeFactor;
	MLVec2D tiltNormalized = (dataEnd.tilt - dataStart.tilt) * totalNormalizeFactor;
	
	FSTabletInputData data = dataStart;
	
	for (int i = 0; i < count; ++i)
	{
		double length = lengths[i];
		
		if (length == 0)
			continue;
		
		if (_carryOver > length)
		{
			_carryOver -= length;
			continue;
		}
		
		MLVec2D dispNormalized = (polygon.at(i+1) - polygon.at(i)) / length;
		
		data.pos = polygon.at(i);
		
		data.pos += dispNormalized * _carryOver;
		data.pressure += pressureNormalized * _carryOver;
		data.rotation += rotationNormalized * _carryOver;
		data.tangentialPressure += tangentialPressureNormalized * _carryOver;
		data.tilt += tiltNormalized * _carryOver;
		
		drawDab(data);
		
		length -= _carryOver;
		
		forever
		{
			length -= 1;
			
			if (length < 0)
				break;
			
			data.pos += dispNormalized;
			data.pressure += pressureNormalized;
			data.rotation += rotationNormalized;
			data.tangentialPressure += tangentialPressureNormalized;
			data.tilt += tiltNormalized;
			
			drawDab(data);
		}
		
		_carryOver = -length;
	}
}

MLImage FSBrushStroker::drawDabImage(const FSTabletInputData &data, QRect *rect)
{
	Q_ASSERT(data.pressure > 0);
	
	double diameter = setting()->diameter * pow(data.pressure, setting()->diameterGamma);
	
	MLVec2D radiusVec;
	radiusVec.x = 0.5 * diameter;
	radiusVec.y = radiusVec.x * (1.0 - setting()->flattening);
	
	_lastMinorRadius = radiusVec.y;
	
	//qDebug() << "radius" << radiusVec.x << radiusVec.y;
	
	QPainterPath ellipse;
	ellipse.addEllipse(data.pos, radiusVec.x, radiusVec.y);
	
	QRect dabRect;
	
	if (setting()->rotation)
	{
		QTransform rotation;
		rotation.rotate(setting()->rotation);
		ellipse = rotation.map(ellipse);
		
		dabRect = ellipse.boundingRect().toAlignedRect();
	}
	else
	{
		dabRect = QRectF(QPointF(data.pos - radiusVec), QSizeF(2.0 * radiusVec)).toAlignedRect();
	}
	
	MLImage dabImage(dabRect.size());
	dabImage.clear();
	
	MLPainter dabPainter(&dabImage);
	
	dabPainter.translateShape(-dabRect.topLeft());
	
	if (setting()->tableWidth == 1 && setting()->tableHeight == 1)
	{
		// no gradient
		dabPainter.setArgb(argb());
	}
	else
	{
		MLArgbGradient gradient;
		gradient.addStop(0, argb());
		gradient.addStop(setting()->tableWidth, argb() * setting()->tableHeight);
		gradient.addStop(1, MLVec4F(0));
		
		dabPainter.setBrush(MLBrush::fromRadialGradient(gradient, data.pos, radiusVec));
	}
	
	dabPainter.drawPath(ellipse);
	
	dabPainter.end();
	
	*rect = dabRect;
	return dabImage;
}

void FSBrushStroker::drawDab(const FSTabletInputData &data)
{
	//qDebug() << "drawing dab" << data.pos.x << data.pos.y;
	
	if (data.pressure <= 0)
		return;
	
	MLSurfacePainter painter(surface());
	
	QRect dabRect;
	MLImage dabImage = drawDabImage(data, &dabRect);
	
	//qDebug() << "dab rect" << dabRect;
	
	// erase
	
	if (setting()->erasing)
	{
		painter.setBlendMode(ML::BlendModeDestinationOut);
		painter.setOpacity(setting()->erasing);
		painter.drawImage(dabRect.topLeft(), dabImage);
	}
	
	painter.setBlendMode(ML::BlendModeSourceOver);
	painter.drawImage(dabRect.topLeft(), dabImage);
	//painter.drawEllipse(dabRect);
	
	painter.flush();
	
	addEditedKeys(painter.editedKeys());
}


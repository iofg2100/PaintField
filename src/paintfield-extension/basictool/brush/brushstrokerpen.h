#ifndef PENSTROKER_H
#define PENSTROKER_H

#include "brushstroker.h"
#include <Malachite/BlendMode>

namespace PaintField {

class BrushStrokerPen : public BrushStroker
{
public:
	
	struct Settings
	{
		int blendMode = Malachite::BlendModeSourceOver;
	};
	
	BrushStrokerPen();
	
	void loadSettings(const QVariantMap &settings) override;
	
protected:
	
	void drawFirst(const TabletInput &data);
	void drawInterval(const Malachite::Polygon &polygon, const TabletInput &dataStart, const TabletInput &dataEnd);
	
private:
	
	void drawShape(const Malachite::FixedMultiPolygon &shape);
	//void drawOne(const Malachite::Vec2D &pos, double pressure, bool drawQuad);
	
	static Malachite::Polygon calcTangentQuadrangle(double radius1, const Malachite::Vec2D &center1, double radius2, const Malachite::Vec2D &center2, double distance);
	//static Malachite::Polygon calcTangentQuadrangle(double radius1, const Malachite::Vec2D &center1, double radius2, const Malachite::Vec2D &center2);
	
	QHash<QPoint, Malachite::FixedMultiPolygon> _drawnShapes;
	Settings _settings;
};

class BrushSourcePenFactory : public BrushStrokerFactory
{
	Q_OBJECT
	
public:
	
	explicit BrushSourcePenFactory(QObject *parent = 0) : BrushStrokerFactory(parent) {}
	
	QString name() const override { return "paintfield.brush.pen"; }
	QVariantMap defaultSettings() const override;
	BrushStroker *createStroker() override;
};

}

#endif // PENSTROKER_H

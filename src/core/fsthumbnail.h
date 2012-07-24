#ifndef FSTHUMBNAIL_H
#define FSTHUMBNAIL_H

#include <QPixmap>
#include <QHash>
#include "fsglobal.h"
#include "mlmisc.h"

class FSThumbnail
{
public:
	enum Metric {
		PixmapSize = 48,
		Margin = 6
	};
	
	FSThumbnail(const QPixmap &originalPixmap);
	
	QPixmap thumbnail() const { return _thumbnail; }
	static QPixmap createThumbnail(const QPixmap &pixmap);
	static QSize thumbnailSize() { return QSize(PixmapSize + 2 * Margin, PixmapSize + 2 * Margin); }
	
private:
	QPixmap _thumbnail;
	static QHash<QSize, QPixmap> _shadowCache;
};

#endif // FSTHUMBNAIL_H

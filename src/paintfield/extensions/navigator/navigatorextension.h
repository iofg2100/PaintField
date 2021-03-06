#pragma once

#include <QObject>
#include "paintfield/core/extension.h"

namespace PaintField
{

class NavigatorController;
class NavigatorView;

class NavigatorExtension : public CanvasExtension
{
	Q_OBJECT
public:
	NavigatorExtension(Canvas *canvas, QObject *parent);
	
signals:
	
public slots:
	
};

class NavigatorExtensionFactory : public ExtensionFactory
{
	Q_OBJECT
public:
	NavigatorExtensionFactory(QObject *parent = 0) : ExtensionFactory(parent) {}
	
	void initialize(AppController *app) override;
	
	CanvasExtensionList createCanvasExtensions(Canvas *canvas, QObject *parent);
};

}


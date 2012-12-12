#include <QMenu>

#include "paintfield-core/appcontroller.h"
#include "paintfield-core/widgets/simplebutton.h"
#include "layertreesidebar.h"
#include "layeruicontroller.h"

#include "layeruimodule.h"

namespace PaintField
{

const QString _layerTreeSidebarName = "paintfield.sidebar.layerTree";

LayerUIModule::LayerUIModule(CanvasController *canvas, QObject *parent) :
    CanvasModule(canvas, parent)
{
	LayerUIController *layerUIController;
	if (canvas)
		layerUIController = new LayerUIController(canvas);
	else
		layerUIController = 0;
	
	auto sideBar = new LayerTreeSidebar(layerUIController, 0);
	addSideBar(_layerTreeSidebarName, sideBar);
	
	if (layerUIController)
	{
		addAction(layerUIController->importAction());
		addAction(layerUIController->newRasterAction());
		addAction(layerUIController->newGroupAction());
		addAction(layerUIController->mergeAction());
	}
}

void LayerUIModuleFactory::initialize(AppController *app)
{
	app->declareMenu("paintfield.layer", tr("Layer"));
	
	app->declareAction("paintfield.layer.newRaster", tr("New Layer"));
	app->declareAction("paintfield.layer.newGroup", tr("New Group"));
	app->declareAction("paintfield.layer.import", tr("Import..."));
	app->declareAction("paintfield.layer.merge", tr("Merge"));
	
	app->declareSideBar(_layerTreeSidebarName, SidebarDeclaration(tr("Layers")));
}

}

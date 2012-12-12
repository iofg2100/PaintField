#include <QtGui>
#include "paintfield-core/layermodel.h"

#include "layeruicontroller.h"
#include "layermodelviewdelegate.h"

namespace PaintField
{

LayerModelViewDelegate::LayerModelViewDelegate(LayerUIController *actionController, QObject *parent) :
	QStyledItemDelegate(parent),
	_actionController(actionController)
{
}

bool LayerModelViewDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	Q_UNUSED(option);
	
	if (event->type() == QEvent::MouseButtonPress)
	{
		auto mouseEvent = static_cast<QMouseEvent *>(event);
		
		if (mouseEvent->button() == Qt::RightButton)
		{
			LayerModel *layerModel = _actionController->canvas()->layerModel();
			Q_ASSERT(layerModel == qobject_cast<LayerModel *>(model));
			
			_actionController->canvas()->selectionModel()->select(index, QItemSelectionModel::Select);
			
			QMenu menu;
			menu.addAction(tr("Remove"), _actionController, SLOT(removeLayers()));
			menu.exec(mouseEvent->globalPos());
			
			return true;
		}
	}
	
	return false;
}

}

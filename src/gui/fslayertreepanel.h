#ifndef FSLAYERTREEPANEL_H
#define FSLAYERTREEPANEL_H

#include "fspanel.h"
#include "fslayer.h"

class FSCanvasView;
class FSDocumentModel;
class QModelIndex;

class QTreeView;
class FSDoubleSlider;
class FSDoubleEdit;
class QComboBox;
class QFormLayout;
class FSSimpleButton;
class QHBoxLayout;
class QVBoxLayout;
class QMenu;
class QItemSelection;

class FSLayerTreePanel : public FSPanel
{
	Q_OBJECT
	
public:
	
	explicit FSLayerTreePanel(QWidget *parent = 0);
	~FSLayerTreePanel();
	
public slots:
	
	void documentChanged(FSDocumentModel *document);
	
	void addImage();
	void newLayer() { newLayerItem(FSLayer::TypeRaster); }
	void newGroup() { newLayerItem(FSLayer::TypeGroup); }
	void removeLayer();
	
signals:
	
protected:
	
private slots:
	
	void updatePropertyView();
	void setOpacityPercentage(double value);
	
private:
	
	void setItemSelected(bool selected);
	
	void createForms();
	
	void newLayerItem(FSLayer::Type type);
	
	// children
	
	FSDocumentModel *_document;
	
	
	// forms
	
	QTreeView *_treeView;
	
	FSDoubleSlider *_opacitySlider;
	FSDoubleEdit *_opacityEdit;
	
	QHBoxLayout *_opacityLayout;
	
	QComboBox *_blendModeComboBox;
	
	QFormLayout *_formLayout;
	QWidget *_formWidget;
	
	FSSimpleButton *_addButton, *_removeButton, *_menuButton;
	QHBoxLayout *_buttonLayout;
	
	QVBoxLayout *_propertyLayout;
	
	QVBoxLayout *_mainLayout;
	
	QMenu *_layerAddMenu, *_miscMenu;
};

#endif // FSLAYERTREEPANEL_H

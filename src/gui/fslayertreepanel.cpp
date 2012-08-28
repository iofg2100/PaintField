#include <QtGui>

#include "fslayertreeview.h"
#include "fsdoubleslider.h"
#include "fssimplebutton.h"
#include "fsguimain.h"
#include "fsrasterlayer.h"
#include "mlpainter.h"
#include "mlimageio.h"

#include "fslayertreepanel.h"

FSLayerTreePanel::FSLayerTreePanel(QWidget *parent) :
    FSPanelWidget(parent),
	_document(0)
{
	createForms();
	
	fsActionManager()->connectTriggered("newLayer", this, SLOT(newLayer()));
	fsActionManager()->connectTriggered("newGroup", this, SLOT(newGroup()));
	fsActionManager()->connectTriggered("addLayerFromFile", this, SLOT(addImage()));
	
	// creating menus
	
	_layerAddMenu = new QMenu(this);
	
	_layerAddMenu->addAction(fsActionManager()->action("newLayer"));
	_layerAddMenu->addAction(fsActionManager()->action("newGroup"));
	_layerAddMenu->addAction(fsActionManager()->action("addLayerFromFile"));
	
	_addButton->setMenu(_layerAddMenu);
	
	_miscMenu = new QMenu(this);
	_miscMenu->addAction(fsActionManager()->action("mergeLayer"));
	
	_menuButton->setMenu(_miscMenu);
	
	connect(_removeButton, SIGNAL(pressed()), this, SLOT(removeLayer()));
	
	connect(fsGuiMain(), SIGNAL(currentDocumentChanged(FSDocumentModel*)), this, SLOT(onDocumentChanged(FSDocumentModel*)));
	onDocumentChanged(fsGuiMain()->currentDocument());
	
	setWindowTitle(tr("Layer"));
}

FSLayerTreePanel::~FSLayerTreePanel()
{
}

QSize FSLayerTreePanel::sizeHint() const
{
	return QSize(DefaultWidth, 300);
}

void FSLayerTreePanel::updatePropertyView()
{
	if (_document)
	{
		setEnabled(true);
		if (_document->currentIndex().isValid())
		{
			_formWidget->setEnabled(true);
			_opacitySpinBox->setValue(_document->currentIndex().data(FSGlobal::RoleOpacity).toDouble() * 100.0);
		}
		else
		{
			_formWidget->setEnabled(false);
		}
	}
	else
	{
		setEnabled(false);
	}
}

void FSLayerTreePanel::setOpacityPercentage(double value)
{
	_document->setData(_document->currentIndex(), value / 100.0, FSGlobal::RoleOpacity);
}

void FSLayerTreePanel::onDocumentChanged(FSDocumentModel *document)
{
	if (_document)
		disconnect(_document, 0, this, 0);
	
	_document = document;
	
	_treeView->setModel(_document);
	if (_document)
	{
		connect(_document, SIGNAL(destroyed()), this, SLOT(onDocumentDeleted()));
		connect(_document, SIGNAL(modified()), this, SLOT(updatePropertyView()));
		connect(_document, SIGNAL(currentIndexChanged(QModelIndex,QModelIndex)), this, SLOT(updatePropertyView()));
		updatePropertyView();
		_treeView->setSelectionModel(_document->selectionModel());
	}
	
	updatePropertyView();
}

void FSLayerTreePanel::onDocumentDeleted()
{
	_document = 0;
}

void FSLayerTreePanel::addImage()
{
	QString filePath = QFileDialog::getOpenFileName(0,
													QObject::tr("Add Layer From Image File"),
													QDir::homePath(),
													QObject::tr("Image Files (*.bmp *.png *.jpg *.jpeg)"));
	if (filePath.isEmpty())
		return;
	
	MLImageImporter imageIO(filePath);
	
	MLSurface surface = imageIO.toSurface();
	if (surface.isNull())
		return;
	
	QFileInfo fileInfo(filePath);
	
	FSRasterLayer layer(fileInfo.fileName());
	layer.setSurface(surface);
	
	QModelIndex index = _document->currentIndex();
	int row = index.isValid() ? index.row() + 1 : _document->rowCount(QModelIndex());
	_document->addLayer(&layer, index.parent(), row, tr("Add Image"));
}

void FSLayerTreePanel::newLayerItem(FSLayer::Type type)
{
	QModelIndex index = _document->currentIndex();
	int row = index.isValid() ? index.row() + 1 : _document->rowCount(QModelIndex());
	_document->newLayer(type, index.parent(), row);
}

void FSLayerTreePanel::removeLayer()
{
	_document->removeLayers(_document->selectionModel()->selectedIndexes());
}

void FSLayerTreePanel::viewFocused()
{
	_document->updateDirtyThumbnails();
}

void FSLayerTreePanel::createForms()
{
	_treeView = new FSLayerTreeView();
	_treeView->setHeaderHidden(true);
	_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	_treeView->setDragDropMode(QAbstractItemView::DragDrop);
	_treeView->setDefaultDropAction(Qt::MoveAction);
	_treeView->setDropIndicatorShown(true);
	connect(_treeView, SIGNAL(windowFocused()), this, SLOT(viewFocused()));
	
	_opacitySlider = new FSDoubleSlider();
	_opacitySlider->setOrientation(Qt::Horizontal);
	_opacitySlider->setMinimum(0);
	_opacitySlider->setMaximum(1000);
	_opacitySlider->setDoubleMinimum(0.0);
	_opacitySlider->setDoubleMaximum(100.0);
	
	_opacitySpinBox = new FSLooseSpinBox();
	_opacitySpinBox->setDecimals(1);
	_opacitySpinBox->setMinimum(0);
	_opacitySpinBox->setMaximum(100);
	_opacitySpinBox->setSingleStep(1.0);
	
	connect(_opacitySlider, SIGNAL(doubleValueChanged(double)), _opacitySpinBox, SLOT(setValue(double)));
	connect(_opacitySpinBox, SIGNAL(valueChanged(double)), _opacitySlider, SLOT(setDoubleValue(double)));
	connect(_opacitySlider, SIGNAL(doubleValueChanged(double)), this, SLOT(setOpacityPercentage(double)));
	
	_opacityLayout = new QHBoxLayout();
	_opacityLayout->addWidget(_opacitySlider);
	_opacityLayout->addWidget(_opacitySpinBox);
	_opacityLayout->addWidget(new QLabel("%"));
	
	_blendModeComboBox = new QComboBox();
	
	_formLayout = new QFormLayout();
	_formLayout->addRow(tr("Opacity"), _opacityLayout);
	_formLayout->addRow(tr("Blend"), _blendModeComboBox);
	_formLayout->setHorizontalSpacing(5);
	_formLayout->setVerticalSpacing(5);
	_formLayout->setContentsMargins(5, 5, 5, 5);
	
	_formWidget = new QWidget();
	_formWidget->setLayout(_formLayout);
	
	_addButton = new FSSimpleButton(":/icons/16x16/add.svg");
	_addButton->setMargins(4, 0, 4, 0);
	_removeButton = new FSSimpleButton(":/icons/16x16/subtract.svg");
	_removeButton->setMargins(4, 0, 4, 0);
	_menuButton = new FSSimpleButton(":/icons/16x16/menuDown.svg");
	_menuButton->setMargins(4, 0, 4, 0);
	
	_buttonLayout = new QHBoxLayout();
	_buttonLayout->addWidget(_addButton);
	_buttonLayout->addWidget(_removeButton);
	_buttonLayout->addWidget(_menuButton);
	_buttonLayout->addStretch(1);
	_buttonLayout->setContentsMargins(5, 5, 5, 5);
	_buttonLayout->setSpacing(0);
	
	_propertyLayout = new QVBoxLayout();
	_propertyLayout->addWidget(_formWidget);
	_propertyLayout->addLayout(_buttonLayout);
	_propertyLayout->setContentsMargins(5, 5, 5, 5);
	
	_mainLayout = new QVBoxLayout();
	_mainLayout->addWidget(_treeView);
	_mainLayout->addLayout(_propertyLayout);
	_mainLayout->setContentsMargins(0, 0, 0, 0);
	_mainLayout->setSpacing(0);
	
	setLayout(_mainLayout);
	
	fsApplyMacSmallSize(this);
}


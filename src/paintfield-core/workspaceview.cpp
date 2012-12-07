#include <QtGui>
#include "widgets/docktabmotherwidget.h"
#include "canvascontroller.h"

#include "workspaceview.h"

namespace PaintField
{

SidebarTabWidget::SidebarTabWidget(WorkspaceView *workspaceView, QWidget *parent) :
	FloatingDockTabWidget(workspaceView, parent)
{}

SidebarTabWidget::SidebarTabWidget(SidebarTabWidget *other, QWidget *parent) :
	FloatingDockTabWidget(other, parent)
{}

bool SidebarTabWidget::tabIsInsertable(DockTabWidget *other, int index)
{
	Q_UNUSED(index)
	SidebarTabWidget *tabWidget = qobject_cast<SidebarTabWidget *>(other);
	return tabWidget && tabWidget->baseWindow() == baseWindow();
}

QObject *SidebarTabWidget::createNew()
{
	return new SidebarTabWidget(this, 0);
}



bool WorkspaceMotherWidget::tabIsInsertable(DockTabWidget *src, int srcIndex)
{
	Q_UNUSED(srcIndex)
	
	SidebarTabWidget *tabWidget = qobject_cast<SidebarTabWidget *>(src);
	return tabWidget && tabWidget->baseWindow() == _workspaceView;
}



namespace MenuArranger
{

QMenu *createMenu(const ActionDeclarationHash &actionDeclarations, const MenuDeclarationHash &menuDeclarations, const QVariantMap &order);
QMenuBar *createMenuBar(const ActionDeclarationHash &actionDeclarations, const MenuDeclarationHash &menuDeclarations, const QVariant &order);
	
void associateMenuWithActions(QMenu *menu, const QActionList &actions);
void associateMenuBarWithActions(QMenuBar *menuBar, const QActionList &actions);

QMenu *createMenu(const ActionDeclarationHash &actionDeclarations, const MenuDeclarationHash &menuDeclarations, const QVariantMap &order)
{
	QString menuId = order["menu"].toString();
	if (menuId.isEmpty())
		return 0;
	
	QString menuTitle = menuDeclarations[menuId].text;
	
	if (menuTitle.isEmpty())
		menuTitle = order["menu"].toString();
	
	QMenu *menu = new QMenu(menuTitle);
	
	QVariantList children = order["children"].toList();
	
	for (const QVariant &child : children)
	{
		switch (child.type())
		{
			case QVariant::String:
			{
				QString id = child.toString();
				
				if (id.isEmpty())
					menu->addSeparator();
				else
				{
					ActionDeclaration actionInfo = actionDeclarations.value(id);
					
					if (actionInfo.text.isEmpty())
						actionInfo.text = id;
					
					WorkspaceMenuAction *action = new WorkspaceMenuAction(menu);
					action->setObjectName(id);
					action->setText(actionInfo.text);
					action->setShortcut(actionInfo.shortcut);
					menu->addAction(action);
				}
				break;
			}
			case QVariant::Map:
			{
				QMenu *childMenu = createMenu(actionDeclarations, menuDeclarations, child.toMap());
				if (childMenu)
					menu->addMenu(childMenu);
				break;
			}
			default:
				break;
		}
	}
	
	return menu;
}

QMenuBar *createMenuBar(const ActionDeclarationHash &actionDeclarations, const MenuDeclarationHash &menuDeclarations, const QVariant &order)
{
	auto menuBar = new QMenuBar();
	
	for (const QVariant &menuOrder : order.toList())
		menuBar->addMenu(createMenu(actionDeclarations, menuDeclarations, menuOrder.toMap()));
	
	return menuBar;
}

void associateMenuWithActions(QMenu *menu, const QActionList &actions)
{
	for (QAction *action : menu->actions())
	{
		WorkspaceMenuAction *menuAction = qobject_cast<WorkspaceMenuAction *>(action);
		if (menuAction)
		{
			QAction *foundAction = findQObjectReverse(actions, menuAction->objectName());
			if (foundAction)
				menuAction->setBackendAction(foundAction);
			else
				menuAction->setEnabled(false);
		}
		else
		{
			if (action->menu())
				associateMenuWithActions(action->menu(), actions);
		}
	}
}

void associateMenuBarWithActions(QMenuBar *menuBar, const QActionList &actions)
{
	for (QAction *action : menuBar->actions())
	{
		QMenu *menu = action->menu();
		if (menu)
			associateMenuWithActions(menu, actions);
	}
}

}

SideBarFrame::SideBarFrame(QWidget *parent) :
	QWidget(parent),
	_layout(new QVBoxLayout)
{
	_layout->setContentsMargins(0, 0, 0, 0);
	setLayout(_layout);
}

void SideBarFrame::setSideBar(QWidget *sideBar)
{
	if (_sideBar)
	{
		_layout->removeWidget(_sideBar);
		_sideBar->setParent(0);
		_sideBar->hide();
	}
	
	_sideBar = sideBar;
	_layout->addWidget(_sideBar);
	_sideBar->show();
}

void WorkspaceMenuAction::setBackendAction(QAction *action)
{
	if (action == _backendAction)
		return;
	
	if (_backendAction)
	{
		disconnect(_backendAction, 0, this, 0);
		disconnect(this, 0, _backendAction, 0);
	}
	
	_backendAction = action;
	
	if (action)
	{
		connect(action, SIGNAL(changed()), this, SLOT(onBackendActionChanged()));
		
		if (action->isCheckable())
		{
			setCheckable(true);
			connect(action, SIGNAL(triggered(bool)), this, SLOT(setChecked(bool)));
			connect(this, SIGNAL(triggered(bool)), action, SLOT(setChecked(bool)));
		}
		else
		{
			connect(this, SIGNAL(triggered()), action, SLOT(trigger()));
		}
		onBackendActionChanged();
	}
}

void WorkspaceMenuAction::onBackendActionChanged()
{
	Q_ASSERT(_backendAction);
	setEnabled(_backendAction->isEnabled());
}

WorkspaceView::WorkspaceView(QWidget *parent) :
    QMainWindow(parent)
{
	setAnimated(false);
		
	_motherWidget = new WorkspaceMotherWidget(this, 0);
	QMainWindow::setCentralWidget(_motherWidget);
	
	onCurrentCanvasPropertyChanged();
}

void WorkspaceView::setCentralWidget(QWidget *widget)
{
	_motherWidget->setCentralWidget(widget);
}

void WorkspaceView::createSideBarFrames(const SideBarDeclarationHash &sidebarDeclarations, const QVariant &order)
{
	auto sideBarSplitterDirFromString = [](const QString &str)->DockTabMotherWidget::Direction
	{
		if (str == "left")
			return DockTabMotherWidget::Left;
		else if (str == "right")
			return DockTabMotherWidget::Right;
		else if (str == "top")
			return DockTabMotherWidget::Top;
		else if (str == "bottom")
			return DockTabMotherWidget::Bottom;
		else
			return DockTabMotherWidget::NoDirection;
	};
	
	QVariantMap orderMap = order.toMap();
	
	for (auto iter = orderMap.begin(); iter != orderMap.end(); ++iter)
	{
		DockTabMotherWidget::Direction splitterDir = sideBarSplitterDirFromString(iter.key());
		if (splitterDir != DockTabMotherWidget::NoDirection)
			createSideBarFramesInArea(splitterDir, sidebarDeclarations, iter.value());
	}
}

void WorkspaceView::createSideBarFramesInArea(DockTabMotherWidget::Direction splitterDir, const SideBarDeclarationHash &sidebarDeclarations, const QVariant &areaOrder)
{
	int i = 0;
	for (const QVariant &splitterOrder : areaOrder.toList())
		createSideBarFramesInSplitter(splitterDir, i++, sidebarDeclarations, splitterOrder.toList());
}

void WorkspaceView::createSideBarFramesInSplitter(DockTabMotherWidget::Direction splitterDir, int splitterIndex, const SideBarDeclarationHash &sidebarDeclarations, const QVariant &splitterOrder)
{
	for (const QVariant &tabWidgetOrder : splitterOrder.toList())
	{
		auto tabWidget = new SidebarTabWidget(this, 0);
		
		for (const QString &sideBarName : tabWidgetOrder.toStringList())
		{
			if (sidebarDeclarations.contains(sideBarName))
			{
				auto sideBarFrame = new SideBarFrame;
				sideBarFrame->setObjectName(sideBarName);
				_sideBarFrames << sideBarFrame;
				
				tabWidget->addTab(sideBarFrame, sidebarDeclarations[sideBarName].text);
			}
		}
		
		_motherWidget->addTabWidget(tabWidget, splitterDir, splitterIndex);
	}
}

void WorkspaceView::createToolBars(const ToolBarDeclarationHash &toolBarDeclarations, const QVariant &order)
{
	auto toolBarAreaFromString = [](const QString &str)->Qt::ToolBarArea
	{
		if (str == "left")
			return Qt::LeftToolBarArea;
		else if (str == "right")
			return Qt::RightToolBarArea;
		else if (str == "top")
			return Qt::TopToolBarArea;
		else if (str == "bottom")
			return Qt::BottomToolBarArea;
		else
			return Qt::NoToolBarArea;
	};
	
	QVariantMap orderMap = order.toMap();
	
	for (auto iter = orderMap.begin(); iter != orderMap.end(); ++iter)
	{
		Qt::ToolBarArea area = toolBarAreaFromString(iter.key());
		if (area != Qt::NoToolBarArea)
		{
			createToolBarsInArea(area, toolBarDeclarations, iter->toList());
		}
	}
}

void WorkspaceView::createToolBarsInArea(Qt::ToolBarArea area, const ToolBarDeclarationHash &toolBarDeclarations, const QVariant &areaOrder)
{
	for (const QString &toolBarName : areaOrder.toStringList())
	{
		for (auto iter = toolBarDeclarations.begin(); iter != toolBarDeclarations.end(); ++iter)
		{
			if (iter.key() == toolBarName)
			{
				auto toolBar = new QToolBar(iter->text);
				toolBar->setObjectName(iter.key());
				addToolBar(area, toolBar);
				_toolBars << toolBar;
				break;
			}
		}
	}
}

void WorkspaceView::createMenuBar(const ActionDeclarationHash &actionDeclarations, const MenuDeclarationHash &menuDeclaratioons, const QVariant &order)
{
	setMenuBar(MenuArranger::createMenuBar(actionDeclarations, menuDeclaratioons, order));
}

void WorkspaceView::setSidebar(const QString &id, QWidget *sidebar)
{
	applyMacSmallSize(sidebar);
	for (SideBarFrame *frame : _sideBarFrames)
	{
		if (frame->objectName() == id)
		{
			frame->setSideBar(sidebar);
			return;
		}
	}
}

QToolBar *WorkspaceView::toolBar(const QString &id)
{
	for (auto toolBar : _toolBars)
	{
		if (toolBar->objectName() == id)
			return toolBar;
	}
	return 0;
}

void WorkspaceView::associateMenuBarWithActions(const QActionList &actions)
{
	MenuArranger::associateMenuBarWithActions(menuBar(), actions);
}

void WorkspaceView::setCurrentCanvas(CanvasController *canvas)
{
	if (_currentCanvas)
	{
		disconnect(_currentCanvas->document(), 0, this, 0);
	}
	
	_currentCanvas = canvas;
	
	if (_currentCanvas)
	{
		connect(canvas->document(), SIGNAL(modifiedChanged(bool)), this, SLOT(onCurrentCanvasPropertyChanged()));
		connect(canvas->document(), SIGNAL(filePathChanged(QString)), this, SLOT(onCurrentCanvasPropertyChanged()));
		onCurrentCanvasPropertyChanged();
	}
}

void WorkspaceView::onCurrentCanvasPropertyChanged()
{
	if (_currentCanvas)
	{
		setWindowModified(_currentCanvas->document()->isModified());
		setWindowFilePath(_currentCanvas->document()->filePath());
		setWindowTitle(_currentCanvas->document()->fileName() + "[*] - PaintField");
	}
	else
	{
		setWindowModified(false);
		setWindowTitle("PaintField");
	}
}

void WorkspaceView::closeEvent(QCloseEvent *event)
{
	emit closeRequested();
	event->ignore();
}

void WorkspaceView::focusInEvent(QFocusEvent *)
{
	emit focusChanged(true);
}

void WorkspaceView::focusOutEvent(QFocusEvent *)
{
	emit focusChanged(false);
}


}

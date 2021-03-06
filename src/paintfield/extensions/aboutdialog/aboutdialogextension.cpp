#include <QAction>
#include <QApplication>
#include <QtPlugin>

#include "paintfield/core/appcontroller.h"
#include "paintfield/core/settingsmanager.h"

#include "aboutdialog.h"

#include "aboutdialogextension.h"

namespace PaintField {

static const QString _aboutActionName = "paintfield.help.about";
static const QString _aboutQtActionName = "paintfield.help.aboutQt";

AboutDialogExtension::AboutDialogExtension(Workspace *workspace, QObject *parent) :
    WorkspaceExtension(workspace, parent)
{
	{ // About PaintField
		auto action = new QAction(this);
		action->setObjectName(_aboutActionName);
		connect(action, SIGNAL(triggered()), this, SLOT(showAboutDialog()));
		
		addAction(action);
	}
	
	{ // About Qt
		auto action = new QAction(this);
		action->setObjectName(_aboutQtActionName);
		connect(action, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
		
		addAction(action);
	}
}

void AboutDialogExtension::showAboutDialog()
{
	AboutDialog dialog;
	dialog.exec();
}

void AboutDialogExtensionFactory::initialize(AppController *appController)
{
	appController->settingsManager()->declareAction(_aboutActionName, tr("About PaintField"));
	appController->settingsManager()->declareAction(_aboutQtActionName, tr("About Qt"));
}

} // namespace PaintField

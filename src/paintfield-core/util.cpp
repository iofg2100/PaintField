#include <QtGui>
#include "qjson/parser.h"
#include "util.h"

namespace PaintField
{

QAction *createAction(const QString &id, QObject *parent)
{
	auto action = new QAction(parent);
	action->setObjectName(id);
	return action;
}

QAction *createAction(const QString &id, QObject *receiver, const char *onTriggeredSlot, QObject *parent)
{
	auto action = createAction(id, parent);
	QObject::connect(action, SIGNAL(triggered()), receiver, onTriggeredSlot);
	return action;
}

QVariant loadJsonFromFile(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << Q_FUNC_INFO << ": failed to open file";
		return QVariant();
	}
	
	QJson::Parser parser;
	return parser.parse(&file);
}

void applyMacSmallSize(QWidget *widget)
{
	widget->setAttribute(Qt::WA_MacSmallSize);
	foreach (QObject *object, widget->children())
	{
		QWidget *widget = qobject_cast<QWidget *>(object);
		if (widget)
			applyMacSmallSize(widget);
	}
}

QString unduplicatedName(const QStringList &existingNames, const QString &name)
{
	if (!existingNames.contains(name))
		return name;
	
	int i = 0;
	forever
	{
		QString newName = name + " #" + QString::number(++i);
		if (!existingNames.contains(newName))
			return newName;
	}
}

}

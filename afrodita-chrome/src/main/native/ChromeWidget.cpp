/*
 * ChromeWidget.cpp
 *
 *  Created on: 02/03/2017
 *      Author: gbuades
 */

#include "ChromeWidget.h"
#include "AfroditaC.h"

#ifndef WIN32

#ifdef USE_QT

#include <QMenu>
#include <stdio.h>
#include <MazingerInternal.h>

//namespace mazinger_chrome {

ChromeWidget::ChromeWidget(QWidget *parent) :
    QMainWindow(parent)
{
    MZNSendDebugMessageA("Init main window\n");
	selectedAction = NULL;
}

std::string ChromeWidget::selectAction (const char * title,
		std::vector<std::string> &optionId,
		std::vector<std::string> &names)
{
	selectedAction = NULL;
    QMenu menu(this);
    std::vector<QAction*> actions;

	QAction * pAction0 = new QAction(QString::fromUtf8(title, strlen(title)), this);
	pAction0->setEnabled(false);
	menu.addAction(pAction0);

	for (int i = 0; i < optionId.size() && i < names.size(); i++ )
    {

    	QAction * pAction = new QAction(QString::fromStdString(names[i].c_str()), this);
    	if (optionId[i].empty())
    	{
    		pAction->setEnabled(false);
    	}
    	else
    	{
    		MZNSendDebugMessageA("Enabling %s", names[i].c_str());
    		pAction->setEnabled(true);
    	}
    	menu.addAction(pAction);
    	actions.push_back(pAction);
    }

    connect( &menu, &QMenu::triggered, this, &ChromeWidget::selected);

    MZNSendDebugMessageA ("Exec menu\n");
    QPoint p = mapFromGlobal(QCursor::pos());
    MZNSendDebugMessageA ("Pos = %d, %d\n", p.x(), p.y());
    menu.exec(p);
    MZNSendDebugMessageA ("End menu\n");
    std::string result;
    for (int i = 0; i < optionId.size() && i < names.size(); i++ )
    {
    	QAction * pAction = actions[i];
    	if (selectedAction == pAction)
    	{
    		result = optionId[i];
    	}
    }

    return result;

}


void ChromeWidget::selected(QAction *action) {
	selectedAction = action;
	MZNSendDebugMessageA("Selected %s\n", action->text().toStdString().c_str());
}


ChromeWidget::~ChromeWidget()
{
}

//}

#endif

#endif

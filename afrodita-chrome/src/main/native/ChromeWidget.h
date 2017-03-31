/*
 * ChromeWidget.h
 *
 *  Created on: 02/03/2017
 *      Author: gbuades
 */

#ifndef CHROMEWIDGET_H_
#define CHROMEWIDGET_H_


#ifndef WIN32
#ifdef USE_QT

#include <QMainWindow>
#include <string>
#include <vector>


// namespace mazinger_chrome {

class ChromeWidget : public QMainWindow {
    Q_OBJECT

public:
    explicit ChromeWidget(QWidget *parent = 0);
	virtual ~ChromeWidget();

	std::string selectAction (const char * title,
			std::vector<std::string> &optionId,
			std::vector<std::string> &names);
private:
	QAction *selectedAction;

private slots:
	void selected(QAction *action);

};

// } /* namespace mazinger_chrome */

#endif
#endif

#endif /* CHROMEWIDGET_H_ */

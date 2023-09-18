#pragma once

#include <obs.hpp>
#include <util/platform.h>
#include <obs-frontend-api.h>
#include <QPointer>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QList>

class QCloseEvent;
class QShowEvent;
class QHideEvent;
class QGridLayout;

class OBSBasicMultiview : public QFrame {
	Q_OBJECT

public:
	static void OBSBasicMultiviewEvent(enum obs_frontend_event event, void *ptr);

	OBSBasicMultiview(QWidget *parent = nullptr, bool closable = true);
	~OBSBasicMultiview();

	virtual void closeEvent(QCloseEvent *event) override;
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;

protected:
	void preCreate(int w = 640, int h = 400, const char *titleName = "titlename");
	void initControl();
	void funcUpdate();

	//void frame();
	//void render();

protected:
	QTimer timerUpdate;

	//control
public:
	void AddOutputLabels(QString name);

protected:
	QGridLayout *outputLayout = nullptr;
	QLabel *labelTest = nullptr;
};

#pragma once

#include <obs.hpp>
#include <util/platform.h>
#include <obs-frontend-api.h>
#include <QPointer>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QList>

#include "qt-display.hpp"
#include "multiview.hpp"

class QCloseEvent;
class QShowEvent;
class QHideEvent;
class QGridLayout;

class OBSBasicMultiview : public OBSQTDisplay {
	Q_OBJECT

public:
	static void OBSBasicMultiviewEvent(enum obs_frontend_event event, void *ptr);

	static void OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy);
	static void OBSSourceDestroyed(void *data, calldata_t *params);

	OBSBasicMultiview(QWidget *parent = nullptr, bool closable = true);
	~OBSBasicMultiview();

	virtual void closeEvent(QCloseEvent *event) override;
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;

	void SetSource(obs_source_t *source_);
	OBSSource GetSource();
	void ClearSource();

	//multiview
	Multiview *multiview = nullptr;
	bool ready = false;
	void UpdateMultiview();
	static void UpdateMultiviewProjectors();

protected:
	OBSWeakSourceAutoRelease weakSource;
	OBSSignal destroyedSignal;

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

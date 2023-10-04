#pragma once

#include <obs.hpp>
#include "qt-display.hpp"
#include "multiview.hpp"

class QMoveEvent;
class QMouseEvent;
class OBSProjectorMultiview : public OBSQTDisplay {
	Q_OBJECT

public:
	static void OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy);
	static void OBSSourceDestroyed(void *data, calldata_t *params);
	static void UpdateMultiviewProjectors();

	OBSProjectorMultiview(QWidget *widget, obs_source_t *source_, QRect rect);
	~OBSProjectorMultiview();

	OBSSource GetSource() { return OBSGetStrongRef(weakSource); }

private:
	void moveEvent(QMoveEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

	void UpdateMultiview(uint32_t w, uint32_t h);

public:
	QRect InitRect;

private slots:
	void OpenFullScreenProjector();
	void ResizeToContent();
	void OpenWindowedProjector();

private:
	OBSWeakSourceAutoRelease weakSource;
	OBSSignal destroyedSignal;

	Multiview *multiview = nullptr;
	bool ready = false;
	QRect prevGeometry;
};

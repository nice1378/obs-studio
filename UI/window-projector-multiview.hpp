#pragma once

#include <obs.hpp>
#include "qt-display.hpp"
#include "multiview.hpp"

class QMoveEvent;
class QMouseEvent;
class OBSProjectorMultiview : public OBSQTDisplay {
	Q_OBJECT

private:
	OBSWeakSourceAutoRelease weakSource;
	OBSSignal destroyedSignal;

	static void OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy);
	static void OBSSourceDestroyed(void *data, calldata_t *params);

	void moveEvent(QMoveEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;

	Multiview *multiview = nullptr;

	bool ready = false;

	void UpdateMultiview();

	QRect prevGeometry;

private slots:
	void OpenFullScreenProjector();
	void ResizeToContent();
	void OpenWindowedProjector();

public:
	OBSProjectorMultiview(QWidget *widget, obs_source_t *source_, QRect rect);
	~OBSProjectorMultiview();

	OBSSource GetSource();
	static void UpdateMultiviewProjectors();
};

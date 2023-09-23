#include <QAction>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QMenu>
#include <QScreen>
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "display-helpers.hpp"
#include "qt-wrappers.hpp"
#include "platform.hpp"
#include "multiview.hpp"

//------------------ static ---------------------
static OBSProjectorMultiview *multiviewProjector;

static bool updatingMultiview = false, mouseSwitching, transitionOnDoubleClick;

void OBSProjectorMultiview::OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy)
{
	OBSProjectorMultiview *window = (OBSProjectorMultiview *)data;
	if (updatingMultiview || !window->ready)
		return;

	window->multiview->Render(cx, cy);
}

void OBSProjectorMultiview::OBSSourceDestroyed(void *data, calldata_t *)
{
	OBSProjectorMultiview *window =
		reinterpret_cast<OBSProjectorMultiview *>(data);
	QMetaObject::invokeMethod(window, "EscapeTriggered");
}

void OBSProjectorMultiview::UpdateMultiviewProjectors()
{
	obs_enter_graphics();
	updatingMultiview = true;
	obs_leave_graphics();

	if (nullptr != multiviewProjector)
		multiviewProjector->UpdateMultiview();

	obs_enter_graphics();
	updatingMultiview = false;
	obs_leave_graphics();
}

//------------------ static ---------------------
OBSProjectorMultiview::OBSProjectorMultiview(QWidget *widget,
					     obs_source_t *source_, QRect rect)
	: OBSQTDisplay(widget, Qt::Window),
	  /*: OBSQTDisplay(widget, Qt::Window
				| Qt::CustomizeWindowHint
				| Qt::WindowTitleHint
	),*/
	  weakSource(OBSGetWeakRef(source_))
{
	OBSSource source = GetSource();
	if (source) {
		destroyedSignal.Connect(obs_source_get_signal_handler(source),
					"destroy", OBSSourceDestroyed, this);
	}

	//Qt::FramelessWindowHint 따로 쓰면 적용이 안됨
	setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__DragonFly__)
	// Prevents resizing of projector windows
	setAttribute(Qt::WA_PaintOnScreen, false);
#endif

	move(rect.x(), rect.y());
	resize(rect.width(), rect.height());

	setWindowTitle(QTStr("MultiviewWindowed") + "_dock");

	setAttribute(Qt::WA_DeleteOnClose, true);

	//disable application quit when last window closed
	setAttribute(Qt::WA_QuitOnClose, false);

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(GetDisplay(), OBSRenderMultiview, this);
		obs_display_set_background_color(GetDisplay(), 0x000000);
	};

	connect(this, &OBSQTDisplay::DisplayCreated, addDrawCallback);

	multiview = new Multiview();
	UpdateMultiview();
	multiviewProjector = this;

	App()->IncrementSleepInhibition();

	if (source)
		obs_source_inc_showing(source);

	ready = true;

	show();

	// We need it here to allow keyboard input in X11 to listen to Escape
	activateWindow();
}

OBSProjectorMultiview::~OBSProjectorMultiview()
{
	obs_display_remove_draw_callback(GetDisplay(), OBSRenderMultiview ,this);

	OBSSource source = GetSource();
	if (source)
		obs_source_dec_showing(source);

	if (nullptr != multiview)
	{
		delete multiview;
		multiview = nullptr;
	}
		
	multiviewProjector = nullptr;

	App()->DecrementSleepInhibition();
}

void OBSProjectorMultiview::moveEvent(QMoveEvent* event)
{
	QWidget::moveEvent(event);
}

void OBSProjectorMultiview::closeEvent(QCloseEvent *event)
{
	event->accept();

	//
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	if (nullptr != main)
	{
		main->ClearProjectorMultiview();
	}


	QWidget::closeEvent(event);
}

void OBSProjectorMultiview::mouseDoubleClickEvent(QMouseEvent *event)
{
	OBSQTDisplay::mouseDoubleClickEvent(event);

	if (!mouseSwitching)
		return;

	if (!transitionOnDoubleClick)
		return;

	OBSBasic *main = (OBSBasic *)obs_frontend_get_main_window();
	if (!main->IsPreviewProgramMode())
		return;

	if (event->button() == Qt::LeftButton) {
		QPoint pos = event->pos();
		OBSSource src = multiview->GetSourceByPosition(pos.x(), pos.y());
		if (!src)
			return;

		if (main->GetProgramSource() != src)
			main->TransitionToScene(src);
	}
}

void OBSProjectorMultiview::mousePressEvent(QMouseEvent *event)
{
	OBSQTDisplay::mousePressEvent(event);

	if (event->button() == Qt::RightButton) {
		//OBSBasic *main =
		//	reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
		//QMenu popup(this);

		//QMenu *projectorMenu = new QMenu(QTStr("Fullscreen"));
		//main->AddProjectorMenuMonitors(projectorMenu, this,
		//			       SLOT(OpenFullScreenProjector()));
		//popup.addMenu(projectorMenu);

		////if (GetMonitor() > -1) {
		////	popup.addAction(QTStr("Windowed"), this,
		////			SLOT(OpenWindowedProjector()));

		////} else if (!this->isMaximized()) {
		////	popup.addAction(QTStr("ResizeProjectorWindowToContent"),
		////			this, SLOT(ResizeToContent()));
		////}

		//QAction *alwaysOnTopButton = new QAction(QTStr("Basic.MainMenu.View.AlwaysOnTop"), this);
		//alwaysOnTopButton->setCheckable(true);
		//alwaysOnTopButton->setChecked(isAlwaysOnTop);

		//connect(alwaysOnTopButton, &QAction::toggled, this,
		//	&OBSProjectorMultiview::AlwaysOnTopToggled);

		//popup.addAction(alwaysOnTopButton);

		//popup.addAction(QTStr("Close"), this, SLOT(EscapeTriggered()));
		//popup.exec(QCursor::pos());
	} else if (event->button() == Qt::LeftButton) {
		// Only MultiView projectors handle left click
		//if (this->type != ProjectorType::Multiview)
		//	return;

		if (!mouseSwitching)
			return;

		QPoint pos = event->pos();
		OBSSource src =
			multiview->GetSourceByPosition(pos.x(), pos.y());
		if (!src)
			return;

		OBSBasic *main = (OBSBasic *)obs_frontend_get_main_window();
		if (main->GetCurrentSceneSource() != src)
			main->SetCurrentScene(src, false);
	}
}

void OBSProjectorMultiview::UpdateMultiview()
{
	MultiviewLayout multiviewLayout = static_cast<MultiviewLayout>(
		config_get_int(GetGlobalConfig(), "BasicWindow", "MultiviewLayout"));

	bool drawLabel = config_get_bool(GetGlobalConfig(), "BasicWindow", "MultiviewDrawNames");
	bool drawSafeArea = config_get_bool(GetGlobalConfig(), "BasicWindow", "MultiviewDrawAreas");
	mouseSwitching = config_get_bool(GetGlobalConfig(), "BasicWindow", "MultiviewMouseSwitch");
	transitionOnDoubleClick = config_get_bool(GetGlobalConfig(), "BasicWindow", "TransitionOnDoubleClick");
	multiview->Update(multiviewLayout, drawLabel, drawSafeArea);
}

void OBSProjectorMultiview::OpenFullScreenProjector()
{
	if (!isFullScreen())
		prevGeometry = geometry();
}

void OBSProjectorMultiview::OpenWindowedProjector()
{
	showFullScreen();
	showNormal();
	setCursor(Qt::ArrowCursor);

	if (!prevGeometry.isNull())
		setGeometry(prevGeometry);
	else
		resize(480, 270);
}

void OBSProjectorMultiview::ResizeToContent()
{
	OBSSource source = GetSource();
	uint32_t targetCX;
	uint32_t targetCY;
	int x, y, newX, newY;
	float scale;

	if (source) {
		targetCX = std::max(obs_source_get_width(source), 1u);
		targetCY = std::max(obs_source_get_height(source), 1u);
	} else {
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		targetCX = ovi.base_width;
		targetCY = ovi.base_height;
	}

	QSize size = this->size();
	GetScaleAndCenterPos(targetCX, targetCY, size.width(), size.height(), x,
			     y, scale);

	newX = size.width() - (x * 2);
	newY = size.height() - (y * 2);
	resize(newX, newY);
}

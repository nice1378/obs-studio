#include "obs-frontend-api/obs-frontend-api.h"

#include "window-basic-multiview.hpp"
#include "window-basic-main.hpp"
#include "platform.hpp"
#include "obs-app.hpp"
#include "qt-wrappers.hpp"

#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScreen>

#include <string>

#define TIMER_INTERVAL 2000
#define REC_TIME_LEFT_INTERVAL 30000

static OBSBasicMultiview *multiviewProjector;
static bool updatingMultiview = false, mouseSwitching, transitionOnDoubleClick;

void OBSBasicMultiview::OBSBasicMultiviewEvent(enum obs_frontend_event event, void *ptr)
{
	OBSBasicMultiview *thisMultiview = reinterpret_cast<OBSBasicMultiview *>(ptr);
	switch (event)
	{
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		break;
	default:
		break;
	}
}

void OBSBasicMultiview::OBSRenderMultiview(void *data, uint32_t cx,
					       uint32_t cy)
{
	OBSBasicMultiview *window = (OBSBasicMultiview *)data;

	if (updatingMultiview || !window->ready)
		return;

	window->multiview->Render(cx, cy);
}

void OBSBasicMultiview::OBSSourceDestroyed(void *data, calldata_t *)
{
	OBSBasicMultiview *window = reinterpret_cast<OBSBasicMultiview *>(data);
	QMetaObject::invokeMethod(window, "EscapeTriggered");
}

OBSBasicMultiview::OBSBasicMultiview(QWidget *parent, bool closeable)
	: OBSQTDisplay(parent)
{
	obs_frontend_add_event_callback(OBSBasicMultiviewEvent, this);

	preCreate(640, 400, "Basic.Multiview");
	initControl();

	//create tiemr
	QObject::connect(&timerUpdate, &QTimer::timeout, this, &OBSBasicMultiview::funcUpdate);
	timerUpdate.setInterval(TIMER_INTERVAL);
	if (isVisible())
		timerUpdate.start();

	//multiview
	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(GetDisplay(), OBSRenderMultiview, this);
		obs_display_set_background_color(GetDisplay(), 0x000000);
	};

	connect(this, &OBSBasicMultiview::DisplayCreated, addDrawCallback);

	multiview = new Multiview();
	UpdateMultiview();

	multiviewProjector = this;

	ready = true;
}

OBSBasicMultiview::~OBSBasicMultiview()
{
	obs_frontend_remove_event_callback(OBSBasicMultiviewEvent, this);

	//multiview
	obs_display_remove_draw_callback(GetDisplay(), OBSRenderMultiview,
					 this);
	OBSSource source = GetSource();
	if (source)
		obs_source_dec_showing(source);

	delete multiview;
}

void OBSBasicMultiview::closeEvent(QCloseEvent *event)
{
	//OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	//if (isVisible()) {
	//	config_set_string(main->Config(), "Stats", "geometry",  saveGeometry().toBase64().constData());
	//	config_save_safe(main->Config(), "tmp", nullptr);
	//}

	QWidget::closeEvent(event);
}

void OBSBasicMultiview::showEvent(QShowEvent *)
{
	timerUpdate.start(TIMER_INTERVAL);
}

void OBSBasicMultiview::hideEvent(QHideEvent *)
{
	timerUpdate.stop();
}


void OBSBasicMultiview::preCreate(int w, int h, const char *titleName)
{
	resize(w, h);

	setWindowTitle(titleName);

#ifdef __APPLE__
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif

	setWindowModality(Qt::NonModal);
	setAttribute(Qt::WA_DeleteOnClose, true);
}

void OBSBasicMultiview::initControl()
{
	outputLayout = new QGridLayout();

	QVBoxLayout *mainLayout = new QVBoxLayout();
	QGridLayout *topLayout = new QGridLayout();

	int row = 0;
	auto newStatBare = [&](QString name, QWidget *label, int col) {
		QLabel *typeLabel = new QLabel(name, this);
		topLayout->addWidget(typeLabel, row, col);
		topLayout->addWidget(label, row++, col + 1);
	};

	auto newStat = [&](const char *strLoc, QWidget *label, int col) {
		std::string str = "Multiview.";
		str += strLoc;
		newStatBare(QTStr(str.c_str()), label, col);
	};

	labelTest = new QLabel(this);
	newStat("LabelTest", labelTest, 0);

	//AddOutputLabels(QTStr("Basic.Stats.Output.Stream"));
}


void OBSBasicMultiview::funcUpdate() {
}


void OBSBasicMultiview::AddOutputLabels(QString name)
{
	//OutputLabels ol;
	//ol.name = new QLabel(name, this);
	//ol.status = new QLabel(this);
	//ol.droppedFrames = new QLabel(this);
	//ol.megabytesSent = new QLabel(this);
	//ol.bitrate = new QLabel(this);

	//int col = 0;
	//int row = outputLabels.size() + 1;
	//outputLayout->addWidget(ol.name, row, col++);
	//outputLayout->addWidget(ol.status, row, col++);
	//outputLayout->addWidget(ol.droppedFrames, row, col++);
	//outputLayout->addWidget(ol.megabytesSent, row, col++);
	//outputLayout->addWidget(ol.bitrate, row, col++);
	//outputLabels.push_back(ol);
}


void OBSBasicMultiview::SetSource(obs_source_t* source_)
{
	weakSource = OBSGetWeakRef(source_);

	OBSSource source = GetSource();
	if (source) {
		destroyedSignal.Connect(obs_source_get_signal_handler(source),
					"destroy", OBSSourceDestroyed, this);
		obs_source_inc_showing(source);
	}
}

OBSSource OBSBasicMultiview::GetSource()
{
	return OBSGetStrongRef(weakSource);
}

void OBSBasicMultiview::ClearSource()
{
}

void OBSBasicMultiview::UpdateMultiview()
{
	if (updatingMultiview || !ready)
		return;

	MultiviewLayout multiviewLayout = static_cast<MultiviewLayout>(
		config_get_int(GetGlobalConfig(), "BasicWindow",
			       "MultiviewLayout"));

	bool drawLabel = config_get_bool(GetGlobalConfig(), "BasicWindow",
					 "MultiviewDrawNames");

	bool drawSafeArea = config_get_bool(GetGlobalConfig(), "BasicWindow",
					    "MultiviewDrawAreas");

	mouseSwitching = config_get_bool(GetGlobalConfig(), "BasicWindow",
					 "MultiviewMouseSwitch");

	transitionOnDoubleClick = config_get_bool(
		GetGlobalConfig(), "BasicWindow", "TransitionOnDoubleClick");

	multiview->Update(multiviewLayout, drawLabel, drawSafeArea);
}

void OBSBasicMultiview::UpdateMultiviewProjectors()
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

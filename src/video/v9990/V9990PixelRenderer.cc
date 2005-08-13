// $Id$

#include "V9990.hh"
#include "V9990VRAM.hh"
#include "V9990DisplayTiming.hh"
#include "V9990PixelRenderer.hh"
#include "V9990Rasterizer.hh"
#include "Scheduler.hh"
#include "Display.hh"
#include "VideoSystem.hh"
#include "FinishFrameEvent.hh"
#include "RealTime.hh"
#include "Timer.hh"
#include "EventDistributor.hh"
#include "RenderSettings.hh"
#include "IntegerSetting.hh"
#include "openmsx.hh"


namespace openmsx {

V9990PixelRenderer::V9990PixelRenderer(V9990& vdp_)
	: vdp(vdp_)
	, rasterizer(Display::instance().getVideoSystem()
	                                .createV9990Rasterizer(vdp_))
{
	frameSkipCounter = 999; // force drawing of frame;
	finishFrameDuration = 0;
	drawFrame = false; // don't draw before frameStart is called

	reset(Scheduler::instance().getCurrentTime());

	settings.getMaxFrameSkip().addListener(this);
	settings.getMinFrameSkip().addListener(this);
}

V9990PixelRenderer::~V9990PixelRenderer()
{
	settings.getMaxFrameSkip().removeListener(this);
	settings.getMinFrameSkip().removeListener(this);
}

void V9990PixelRenderer::reset(const EmuTime& time)
{
	PRT_DEBUG("V9990PixelRenderer::reset");

	displayEnabled = vdp.isDisplayEnabled();
	setDisplayMode(vdp.getDisplayMode(), time);
	setColorMode(vdp.getColorMode(), time);

	rasterizer->reset();
}

void V9990PixelRenderer::frameStart(const EmuTime& time)
{
	if (!rasterizer->isActive()) {
		frameSkipCounter = 0;
		drawFrame = false;
	} else if (frameSkipCounter < settings.getMinFrameSkip().getValue()) {
		++frameSkipCounter;
		drawFrame = false;
	} else if (frameSkipCounter >= settings.getMaxFrameSkip().getValue()) {
		frameSkipCounter = 0;
		drawFrame = true;
	} else {
		++frameSkipCounter;
		drawFrame = RealTime::instance().timeLeft(
			(unsigned)finishFrameDuration, time);
		if (drawFrame) {
			frameSkipCounter = 0;
		}
	}
	if (!drawFrame) return;

	accuracy = settings.getAccuracy().getValue();
	lastX = 0;
	lastY = 0;

	// Make sure that the correct timing is used
	setDisplayMode(vdp.getDisplayMode(), time);
	rasterizer->frameStart();
}

void V9990PixelRenderer::frameEnd(const EmuTime& time)
{
	PRT_DEBUG("V9990PixelRenderer::frameEnd");

	if (!drawFrame) return;

	// Render last changes in this frame before starting a new frame
	sync(time, true);

	unsigned long long time1 = Timer::getTime();
	rasterizer->frameEnd();
	unsigned long long time2 = Timer::getTime();
	unsigned long long current = time2 - time1;
	const double ALPHA = 0.2;
	finishFrameDuration = finishFrameDuration * (1 - ALPHA) +
	                      current * ALPHA;

	FinishFrameEvent* f = new FinishFrameEvent(VIDEO_GFX9000);
	EventDistributor::instance().distributeEvent(f);
}

void V9990PixelRenderer::sync(const EmuTime& time, bool force)
{
	if (!drawFrame) return;

	if (accuracy != RenderSettings::ACC_SCREEN || force) {
		vdp.getVRAM().sync(time);
		renderUntil(time);
	}
}

void V9990PixelRenderer::renderUntil(const EmuTime& time)
{
	const V9990DisplayPeriod& horTiming = vdp.getHorizontalTiming();
	const V9990DisplayPeriod& verTiming = vdp.getVerticalTiming();

	// Translate time to pixel position
	int limitTicks = vdp.getUCTicksThisFrame(time);
	assert(limitTicks <=
	       V9990DisplayTiming::getUCTicksPerFrame(vdp.isPalTiming()));
	int toX, toY;
	switch (accuracy) {
	case RenderSettings::ACC_PIXEL:
		toX = limitTicks % V9990DisplayTiming::UC_TICKS_PER_LINE -
		      horTiming.blank;
		toY = limitTicks / V9990DisplayTiming::UC_TICKS_PER_LINE -
		      verTiming.blank;
		break;
	case RenderSettings::ACC_LINE:
	case RenderSettings::ACC_SCREEN:
		// TODO figure out rounding point
		toX = 0;
		toY = (limitTicks + V9990DisplayTiming::UC_TICKS_PER_LINE - 400) /
		             V9990DisplayTiming::UC_TICKS_PER_LINE -
		      verTiming.blank;
		break;
	default:
		assert(false);
		toX = toY = 0; // avoid warning
	}

	if ((toX == lastX) && (toY == lastY)) return;

	// edges of the DISPLAY part of the vdp output
	int left       = horTiming.border1;
	int right      = left   + horTiming.display;
	int rightEdge  = right  + horTiming.border2;

	if (displayEnabled) {
		// left border
		subdivide(lastX, lastY, toX, toY, 0, left, DRAW_BORDER);
		// display area
		subdivide(lastX, lastY, toX, toY, left, right, DRAW_DISPLAY);
		// right border
		subdivide(lastX, lastY, toX, toY, right, rightEdge, DRAW_BORDER);
	} else {
		// complete screen
		subdivide(lastX, lastY, toX, toY, 0, rightEdge, DRAW_BORDER);
	}

	lastX = toX;
	lastY = toY;
}

void V9990PixelRenderer::subdivide(int fromX, int fromY, int toX, int toY,
                                   int clipL, int clipR, DrawType drawType)
{
	// partial first line
	if (fromX > clipL) {
		if (fromX < clipR) {
			bool atEnd = (fromY != toY) || (toX >= clipR);
			draw(fromX, fromY, (atEnd ? clipR : toX), fromY + 1,
			     drawType);
		}
		if (fromY == toY) return;
		fromY++;
	}

	bool drawLast = false;
	if (toX >= clipR) {
		toY++;
	} else if (toX > clipL) {
		drawLast = true;
	}
	// full middle lines
	if (fromY < toY) {
		draw(clipL, fromY, clipR, toY, drawType);
	}

	// partial last line
	if (drawLast) draw(clipL, toY, toX, toY + 1, drawType);
}

void V9990PixelRenderer::draw(int fromX, int fromY, int toX, int toY,
                              DrawType type)
{
	PRT_DEBUG("V9990PixelRenderer::draw(" << std::dec <<
	          fromX << "," << fromY << "," << toX << "," << toY << "," <<
	          ((type == DRAW_BORDER)? "BORDER": "DISPLAY") << ")");

	if (type == DRAW_BORDER) {
		rasterizer->drawBorder(fromX, fromY, toX, toY);

	} else {
		assert(type == DRAW_DISPLAY);

		const V9990DisplayPeriod& horTiming = vdp.getHorizontalTiming();
		const V9990DisplayPeriod& verTiming = vdp.getVerticalTiming();

		int displayX = fromX - horTiming.border1;
		int displayY = fromY - verTiming.border1;
		int displayWidth = toX - fromX;
		int displayHeight = toY - fromY;

		rasterizer->drawDisplay(fromX, fromY, displayX, displayY,
		                        displayWidth, displayHeight);
	}
}

void V9990PixelRenderer::updateDisplayEnabled(bool enabled, const EmuTime& time)
{
	sync(time, true);
	displayEnabled = enabled;
}

void V9990PixelRenderer::setDisplayMode(V9990DisplayMode mode, const EmuTime& time)
{
	sync(time);
	rasterizer->setDisplayMode(mode);
}

void V9990PixelRenderer::updatePalette(int index, byte r, byte g, byte b,
                                       const EmuTime& time)
{
	if (displayEnabled) {
		sync(time);
	} else {
		// TODO only sync if border color changed
		sync(time);
	}
	rasterizer->setPalette(index, r, g, b);
}
void V9990PixelRenderer::setColorMode(V9990ColorMode mode, const EmuTime& time)
{
	sync(time);
	rasterizer->setColorMode(mode);
}

void V9990PixelRenderer::updateBackgroundColor(int /*index*/, const EmuTime& time)
{
	sync(time);
}

void V9990PixelRenderer::setImageWidth(int width)
{
	if (displayEnabled) rasterizer->setImageWidth(width);
}

void V9990PixelRenderer::updateScrollAX(const EmuTime& time)
{
	if (displayEnabled) sync(time);
}
void V9990PixelRenderer::updateScrollAY(const EmuTime& time)
{
	if (displayEnabled) sync(time);
}
void V9990PixelRenderer::updateScrollBX(const EmuTime& time)
{
	// TODO only in P1 mode
	if (displayEnabled) sync(time);
}
void V9990PixelRenderer::updateScrollBY(const EmuTime& time)
{
	// TODO only in P1 mode
	if (displayEnabled) sync(time);
}

void V9990PixelRenderer::update(const Setting* setting)
{
	if (setting == &settings.getMinFrameSkip() ||
	    setting == &settings.getMaxFrameSkip()) {
		// Force drawing of frame
		frameSkipCounter = 999;
	} else {
		assert(false);
	}
}

} // namespace openmsx

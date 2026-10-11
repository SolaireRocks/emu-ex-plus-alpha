/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include "PlaceVControlsView.hh"
#include <emuframework/EmuApp.hh>
import imagine;

namespace EmuEx
{

[[maybe_unused]] constexpr SystemLogger log{"PlaceVControlsView"};
constexpr std::array snapPxSizes{0, 2, 4, 8, 16, 32, 64};
// sizeBarQuads layout: selection outline, bar background, item backgrounds
constexpr size_t outlineQuads = 4;
constexpr size_t barBgQuadIdx = outlineQuads;
constexpr size_t itemBgQuadIdx = barBgQuadIdx + 1;

PlaceVControlsView::PlaceVControlsView(ViewAttachParams attach, VController &vController_):
	View{attach},
	exitText{attach.rendererTask, "Exit", &defaultFace()},
	snapText{attach.rendererTask, "Snap: 0px", &defaultFace()},
	sizeBarText
	{{
		{attach.rendererTask, "Group", &defaultFace()},
		{attach.rendererTask, "-", &defaultFace()},
		{attach.rendererTask, "100%", &defaultFace()},
		{attach.rendererTask, "+", &defaultFace()},
		{attach.rendererTask, "Reset", &defaultFace()},
		{attach.rendererTask, "Done", &defaultFace()},
	}},
	vController{vController_},
	gridIdxs{attach.rendererTask, 2, 2},
	quads{attach.rendererTask, {.size = 4}, gridIdxs},
	sizeBarQuads{attach.rendererTask, {.size = itemBgQuadIdx + sizeBarItems}}
{
	app().applyOSNavStyle(appContext(), true);
}

PlaceVControlsView::~PlaceVControlsView()
{
	app().applyOSNavStyle(appContext(), false);
}

void PlaceVControlsView::place()
{
	dragTracker.reset();
	// small dead zone so a tap that selects an element doesn't also nudge it
	dragTracker.setDragStartPixels(window().widthMMInPixels(1.));
	exitText.compile();
	snapText.compile();
	exitBtnRect = WRect{{}, exitText.pixelSize()} + (viewRect().pos(C2DO) - exitText.pixelSize() / 2) + WPt{0, exitText.height()};
	snapBtnRect = WRect{{}, snapText.pixelSize()} + (viewRect().pos(C2DO) - snapText.pixelSize() / 2) - WPt{0, exitText.height()};
	const int lineSize = 1;
	auto snapPxSize = snapPxSizes[snapPxIdx];
	size_t hLines = snapPxSize >= 16 ? std::max(viewRect().ySize() / snapPxSize, 1) : 1;
	size_t vLines = snapPxSize >= 16 ? std::max(viewRect().xSize() / snapPxSize, 1) : 1;
	quads.reset({.size = 2 + hLines + vLines});
	using Quad = decltype(quads)::Type;
	auto map = quads.map();
	Quad{{.bounds = exitBtnRect.as<int16_t>()}}.write(map, 0);
	Quad{{.bounds = snapBtnRect.as<int16_t>()}}.write(map, 1);
	if(hLines == 1 && vLines == 1) // center lines
	{
		Quad{{.bounds = WRect{{viewRect().x, viewRect().yCenter()},
			{viewRect().x2, viewRect().yCenter() + lineSize}}.as<int16_t>()}}.write(map, 2);
		Quad{{.bounds = WRect{{viewRect().xCenter(), viewRect().y},
			{viewRect().xCenter() + lineSize, viewRect().y2}}.as<int16_t>()}}.write(map, 3);
	}
	else // full grid
	{
		for(auto i: iotaCount(hLines))
		{
			int yPos = viewRect().y + ((i + 1) * snapPxSize);
			Quad{{.bounds = WRect{{viewRect().x, yPos},
				{viewRect().x2, yPos + lineSize}}.as<int16_t>()}}.write(map, 2 + i);
		}
		for(auto i: iotaCount(vLines))
		{
			int xPos = viewRect().x + ((i + 1) * snapPxSize);
			Quad{{.bounds = WRect{{xPos, viewRect().y},
				{xPos + lineSize, viewRect().y2}}.as<int16_t>()}}.write(map, 2 + hLines + i);
		}
	}
	gridIdxs.reset(2 + hLines + vLines);
	placeSizeBar();
}

void PlaceVControlsView::select(VControllerElement *elem, int btnIdx)
{
	selElem = elem;
	selBtnIdx = btnIdx;
	// a tapped button gets resized by itself, unless it's alone in its group
	sizeButton = selectedButton() && selElem->buttons().size() > 1;
	placeSizeBar();
}

VControllerButton *PlaceVControlsView::selectedButton() const
{
	if(!selElem || selBtnIdx < 0)
		return {};
	auto btns = selElem->buttons();
	return size_t(selBtnIdx) < btns.size() ? &btns[selBtnIdx] : nullptr;
}

WRect PlaceVControlsView::selectionBounds() const
{
	if(auto btn = selectedButton(); sizeButton && btn)
		return btn->bounds();
	return selElem->bounds();
}

int PlaceVControlsView::sizePercent() const
{
	if(auto btn = selectedButton(); sizeButton && btn)
		return btn->sizePercent;
	return selElem->sizePercent;
}

void PlaceVControlsView::setSizePercent(int percent)
{
	if(auto btn = selectedButton(); sizeButton && btn)
		vController.setButtonSizePercent(*btn, percent);
	else
		vController.setElementSizePercent(*selElem, percent);
	app().viewController().placeEmuViews();
	placeSizeBar();
	postDraw();
}

void PlaceVControlsView::placeSizeBar()
{
	if(!selElem)
		return;
	sizeBarText[size_t(SizeBarItem::target)].resetString(sizeButton ? "Button" : selElem->dPad() ? "D-Pad" : "Group");
	sizeBarText[size_t(SizeBarItem::percent)].resetString(std::format("{}%", sizePercent()));
	const int textHeight = exitText.height();
	auto itemWidth = [&](const Gfx::Text &t) { return std::max(t.width() + textHeight, textHeight * 2); };
	int itemsWidth{};
	for(auto &t : sizeBarText)
	{
		t.compile();
		itemsWidth += itemWidth(t);
	}
	const auto rect = viewRect();
	const int barHeight = textHeight * 3;
	const int gap = std::max((rect.xSize() - itemsWidth) / int(sizeBarItems + 1), 0);
	// keep the bar at the bottom unless it would cover what's being resized
	sizeBarRect = {{rect.x, rect.y2 - barHeight}, {rect.x2, rect.y2}};
	auto selBounds = selectionBounds();
	if(sizeBarRect.overlaps(selBounds))
		sizeBarRect = {{rect.x, rect.y}, {rect.x2, rect.y + barHeight}};
	const int itemYPadding = textHeight / 2;
	int x = rect.x + gap;
	for(auto &&[i, t] : enumerate(sizeBarText))
	{
		int w = itemWidth(t);
		sizeBarItemRects[i] = {{x, sizeBarRect.y + itemYPadding}, {x + w, sizeBarRect.y2 - itemYPadding}};
		x += w + gap;
	}
	using Quad = decltype(sizeBarQuads)::Type;
	auto map = sizeBarQuads.map();
	const int t = std::max(window().widthMMInPixels(.5), 2);
	const auto &b = selBounds;
	Quad{{.bounds = WRect{{b.x - t, b.y - t}, {b.x2 + t, b.y}}.as<int16_t>()}}.write(map, 0);
	Quad{{.bounds = WRect{{b.x - t, b.y2}, {b.x2 + t, b.y2 + t}}.as<int16_t>()}}.write(map, 1);
	Quad{{.bounds = WRect{{b.x - t, b.y}, {b.x, b.y2}}.as<int16_t>()}}.write(map, 2);
	Quad{{.bounds = WRect{{b.x2, b.y}, {b.x2 + t, b.y2}}.as<int16_t>()}}.write(map, 3);
	Quad{{.bounds = sizeBarRect.as<int16_t>()}}.write(map, barBgQuadIdx);
	for(auto &&[i, r] : enumerate(sizeBarItemRects))
	{
		Quad{{.bounds = r.as<int16_t>()}}.write(map, itemBgQuadIdx + i);
	}
}

std::optional<PlaceVControlsView::SizeBarItem> PlaceVControlsView::sizeBarItemAt(WPt pos) const
{
	if(!selElem)
		return {};
	for(auto &&[i, r] : enumerate(sizeBarItemRects))
	{
		if(r.overlaps(pos))
			return SizeBarItem(i);
	}
	return {};
}

void PlaceVControlsView::onSizeBarItem(SizeBarItem item)
{
	switch(item)
	{
		case SizeBarItem::target:
			if(!selectedButton())
				return;
			sizeButton = !sizeButton;
			placeSizeBar();
			postDraw();
			return;
		case SizeBarItem::minus: return setSizePercent(sizePercent() - sizePercentStep);
		case SizeBarItem::plus: return setSizePercent(sizePercent() + sizePercentStep);
		case SizeBarItem::reset: return setSizePercent(100);
		case SizeBarItem::done:
			select(nullptr, -1);
			postDraw();
			return;
		case SizeBarItem::percent: return;
	}
}

bool PlaceVControlsView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	return e.visit(overloaded
	{
		[&](const Input::KeyEvent &e)
		{
			if(e.pushed())
			{
				dismiss();
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &e)
		{
			size_t layoutIdx = window().isPortrait();
			dragTracker.inputEvent(e,
				[&](Input::DragTrackerState, DragData &d)
				{
					if(d.elem)
						return;
					if(selElem && sizeBarRect.overlaps(e.pos()))
						return; // handled by the size bar on release
					auto tryGrabElement = [&](VControllerElement &elem)
					{
						if(elem.state == VControllerState::OFF || !elem.bounds().contains(e.pos()))
							return false;
						for(const auto &state : dragTracker.stateList())
						{
							if(state.data.elem == &elem)
								return false; // element already grabbed
						}
						d.elem = &elem;
						d.startPos = elem.bounds().pos(C2DO);
						d.btnIdx = -1;
						for(auto &&[i, b] : enumerate(elem.buttons()))
						{
							if(b.enabled && b.bounds().contains(e.pos()))
							{
								d.btnIdx = int(i);
								break;
							}
						}
						return true;
					};
					for(auto &elem : vController.deviceElements())
					{
						if(tryGrabElement(elem))
							return;
					}
					for(auto &elem : vController.guiElements())
					{
						if(tryGrabElement(elem))
							return;
					}
				},
				[&](Input::DragTrackerState state, Input::DragTrackerState, DragData &d)
				{
					if(d.elem && state.isDragging())
					{
						auto newPos = d.startPos + state.downPosDiff();
						if(snapPxIdx)
						{
							auto snapPx = snapPxSizes[snapPxIdx];
							auto remainder = newPos % snapPx;
							if(remainder.x)
								newPos.x = newPos.x + snapPx - remainder.x;
							if(remainder.y)
								newPos.y = newPos.y + snapPx - remainder.y;
						}
						auto bounds = window().bounds();
						d.elem->setPos(newPos, bounds);
						auto layoutPos = VControllerLayoutPosition::fromPixelPos(d.elem->bounds().pos(C2DO), d.elem->bounds().size(), bounds);
						//log.info("set pos {},{} from {},{}", layoutPos.pos.x, layoutPos.pos.y, layoutPos.origin.xScaler(), layoutPos.origin.yScaler());
						auto &vCtrlLayoutPos = d.elem->layoutPos[layoutIdx];
						vCtrlLayoutPos.origin = layoutPos.origin;
						vCtrlLayoutPos.pos = layoutPos.pos;
						app().viewController().placeEmuViews();
						if(d.elem == selElem)
							placeSizeBar();
						postDraw();
					}
				},
				[&](Input::DragTrackerState state, DragData &d)
				{
					if(d.elem)
					{
						if(!state.isDragging())
						{
							select(d.elem, d.btnIdx);
							postDraw();
						}
						return;
					}
					if(selElem && sizeBarRect.overlaps(state.downPos()))
					{
						if(auto item = sizeBarItemAt(state.pos()); item && item == sizeBarItemAt(state.downPos()))
							onSizeBarItem(*item);
						return;
					}
					if(exitBtnRect.overlaps(state.pos()) && exitBtnRect.overlaps(state.downPos()))
					{
						dismiss();
					}
					else if(snapBtnRect.overlaps(state.pos()) && snapBtnRect.overlaps(state.downPos()))
					{
						snapPxIdx = (snapPxIdx + 1) % snapPxSizes.size();
						snapText.resetString(std::format("Snap: {}px", snapPxSizes[snapPxIdx]));
						place();
						postDraw();
					}
					else if(selElem && !state.isDragging())
					{
						// tapping an empty spot closes the size bar
						select(nullptr, -1);
						postDraw();
					}
				});
			return true;
		}
	});
}

void PlaceVControlsView::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
{
	using namespace Gfx;
	cmds.setColor({.5, .5, .5});
	auto &basicEffect = cmds.basicEffect();
	basicEffect.disableTexture(cmds);
	cmds.drawQuads<uint16_t>(quads, 2, quads.size() - 2); // grid
	vController.draw(cmds, true);
	cmds.setColor({0, 0, 0, .5});
	basicEffect.disableTexture(cmds);
	cmds.drawQuads<uint16_t>(quads, 0, 2); // button bg
	if(selElem)
	{
		cmds.setColor(Color{ColorName::YELLOW});
		cmds.drawQuads(sizeBarQuads, 0, outlineQuads);
		cmds.setColor({0, 0, 0, .75});
		cmds.drawQuads(sizeBarQuads, barBgQuadIdx, 1);
		cmds.setColor({.3, .3, .3, 1.});
		cmds.drawQuads(sizeBarQuads, itemBgQuadIdx, sizeBarItems);
	}
	basicEffect.enableAlphaTexture(cmds);
	exitText.draw(cmds, exitBtnRect.pos(C2DO), C2DO, ColorName::WHITE);
	snapText.draw(cmds, snapBtnRect.pos(C2DO), C2DO, ColorName::WHITE);
	if(selElem)
	{
		for(auto &&[i, t] : enumerate(sizeBarText))
		{
			// the Group/Button switch is only usable after tapping a button
			bool isInactiveTarget = SizeBarItem(i) == SizeBarItem::target && !selectedButton();
			t.draw(cmds, sizeBarItemRects[i].pos(C2DO), C2DO, isInactiveTarget ? Color{.6f, .6f, .6f} : Color{ColorName::WHITE});
		}
	}
}

void PlaceVControlsView::onShow()
{
	vController.applyButtonAlpha(.75);
}

}

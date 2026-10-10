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

#include "ExtrasOptionView.hh"
#include <emuframework/EmuApp.hh>
#include <emuframework/viewUtils.hh>
import imagine;

namespace EmuEx
{

FastSpeedOption::FastSpeedOption(ExtrasOptionView &view_, ViewAttachParams attach, AltSpeedMode mode_, const char *name):
	view{view_},
	mode{mode_},
	items
	{
		{"1.5x", attach, {.id = 150}},
		{"2x",   attach, {.id = 200}},
		{"3x",   attach, {.id = 300}},
		{"4x",   attach, {.id = 400}},
		{"8x",   attach, {.id = 800}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueRangeInputView<float, 1, 20>(view.attachParams(), e, "Input above 1.0 to 20.0", "",
					[this](CollectTextInputView &, auto val)
					{
						auto valAsInt = int(std::round(val * 100.f));
						if(!view.app().setAltSpeed(mode, valAsInt))
						{
							view.app().postErrorMessage("Value not in range");
							return false;
						}
						choice.setSelected(MenuId{valAsInt}, view);
						view.dismissPrevious();
						view.app().defaultVController().place(); // refresh speed labels
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	choice
	{
		name, attach,
		MenuId{view_.app().altSpeed(mode_)},
		items,
		{
			.onSetDisplayString = [this](auto, Gfx::Text &t)
			{
				t.resetString(std::format("{:g}x", view.app().altSpeedAsDouble(mode)));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item)
			{
				view.app().setAltSpeed(mode, item.id);
				view.app().defaultVController().place(); // refresh speed labels
			}
		},
	} {}

ExtrasOptionView::ExtrasOptionView(ViewAttachParams attach):
	TableView{"Extras", attach, item},
	speedHeading{"Fast-forward Speeds", attach},
	fastSpeed1{*this, attach, AltSpeedMode::fast, "Speed 1 (Standard Button)"},
	fastSpeed2{*this, attach, AltSpeedMode::fast2, "Speed 2"},
	fastSpeed3{*this, attach, AltSpeedMode::fast3, "Speed 3"},
	rollingHeading{"Rolling Save States", attach},
	rollingStatesItems
	{
		{"Off", attach, {.id = 0}},
		{"3",   attach, {.id = 3}},
		{"5",   attach, {.id = 5}},
		{"10",  attach, {.id = 10}},
		{"20",  attach, {.id = 20}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueRangeInputView<int, 0, RollingSaveManager::maxStatesLimit>(attachParams(), e, "Input 0 to 50", "",
					[this](CollectTextInputView &, auto val)
					{
						app().rollingSaveManager.setMaxStates(val);
						rollingStates.setSelected(MenuId{val}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	rollingStates
	{
		"States To Keep", attach,
		MenuId{app().rollingSaveManager.maxStates()},
		rollingStatesItems,
		{
			.onSetDisplayString = [this](auto, Gfx::Text &t)
			{
				auto states = app().rollingSaveManager.maxStates();
				t.resetString(states ? std::format("{}", states) : std::string{"Off"});
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().rollingSaveManager.setMaxStates(item.id); }
		},
	},
	rollingIntervalItems
	{
		{"30s",  attach, {.id = 30}},
		{"1min", attach, {.id = 60}},
		{"3min", attach, {.id = 180}},
		{"5min", attach, {.id = 300}},
		{"10min", attach, {.id = 600}},
		{"Custom Value", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueRangeInputView<int, 10, 3600>(attachParams(), e, "Input 10 to 3600 seconds", "",
					[this](CollectTextInputView &, auto val)
					{
						app().rollingSaveManager.setInterval(Seconds{val});
						rollingInterval.setSelected(MenuId{val}, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, {.id = defaultMenuId}
		},
	},
	rollingInterval
	{
		"Save Every", attach,
		MenuId{int(app().rollingSaveManager.interval().count())},
		rollingIntervalItems,
		{
			.onSetDisplayString = [this](auto, Gfx::Text &t)
			{
				t.resetString(std::format("{}s", app().rollingSaveManager.interval().count()));
				return true;
			},
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().rollingSaveManager.setInterval(Seconds{item.id}); }
		},
	},
	screenshotHeading{"Screenshots", attach},
	screenshotScaleItems
	{
		{"1x (Native)", attach, {.id = 1}},
		{"2x", attach, {.id = 2}},
		{"3x", attach, {.id = 3}},
		{"4x", attach, {.id = 4}},
		{"6x", attach, {.id = 6}},
		{"8x", attach, {.id = 8}},
	},
	screenshotScale
	{
		"Screenshot Resolution", attach,
		MenuId{app().screenshotScale.value()},
		screenshotScaleItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().screenshotScale = int8_t(item.id); }
		},
	},
	screenshotAspect
	{
		"Screenshot Aspect Ratio", attach,
		app().screenshotAspectCorrect.value(),
		"Native Pixels", "Stretch To 4:3",
		[this](BoolMenuItem &item) { app().screenshotAspectCorrect = item.flipBoolValue(*this); }
	},
	doubleTapScreenshot
	{
		"Double-Tap Game For Screenshot", attach,
		app().defaultVController().doubleTapScreenshot,
		[this](BoolMenuItem &item) { app().defaultVController().doubleTapScreenshot = item.flipBoolValue(*this); }
	},
	recordingHeading{"Video Recording & Instant Replay", attach},
	recordingScaleItems
	{
		{"1x (Native)", attach, {.id = 1}},
		{"2x", attach, {.id = 2}},
		{"3x", attach, {.id = 3}},
		{"4x", attach, {.id = 4}},
	},
	recordingScale
	{
		"Video Resolution", attach,
		MenuId{app().mediaRecorder.scale},
		recordingScaleItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().mediaRecorder.scale = int8_t(item.id); }
		},
	},
	replayModeItems
	{
		{"Off", attach, {.id = ReplayBufferMode::off}},
		{"When Replay Button Is On Screen", attach, {.id = ReplayBufferMode::whenButtonPresent}},
		{"Always", attach, {.id = ReplayBufferMode::always}},
	},
	replayMode
	{
		"Instant Replay Buffer", attach,
		MenuId{app().mediaRecorder.replayMode},
		replayModeItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().mediaRecorder.replayMode = ReplayBufferMode(item.id.val); }
		},
	},
	replaySecondsItems
	{
		{"10s", attach, {.id = 10}},
		{"15s", attach, {.id = 15}},
		{"30s", attach, {.id = 30}},
		{"60s", attach, {.id = 60}},
		{"2min", attach, {.id = 120}},
	},
	replaySeconds
	{
		"Instant Replay Length", attach,
		MenuId{app().mediaRecorder.replaySeconds},
		replaySecondsItems,
		{
			.defaultItemOnSelect = [this](TextMenuItem &item) { app().mediaRecorder.replaySeconds = int16_t(item.id); }
		},
	},
	buttonsHeading{"On-screen Buttons", attach},
	turboStyle
	{
		"Turbo Button Style", attach,
		app().defaultVController().turboButtonStyle() == VControllerTurboStyle::outline,
		"Classic", "Outline + Badge",
		[this](BoolMenuItem &item)
		{
			app().defaultVController().setTurboButtonStyle(item.flipBoolValue(*this) ? VControllerTurboStyle::outline : VControllerTurboStyle::classic);
		}
	}
{
	item.emplace_back(&speedHeading);
	item.emplace_back(&fastSpeed1.choice);
	item.emplace_back(&fastSpeed2.choice);
	item.emplace_back(&fastSpeed3.choice);
	item.emplace_back(&rollingHeading);
	item.emplace_back(&rollingStates);
	item.emplace_back(&rollingInterval);
	item.emplace_back(&screenshotHeading);
	item.emplace_back(&screenshotScale);
	item.emplace_back(&screenshotAspect);
	item.emplace_back(&doubleTapScreenshot);
	if(Config::envIsAndroid)
	{
		item.emplace_back(&recordingHeading);
		item.emplace_back(&recordingScale);
		item.emplace_back(&replayMode);
		item.emplace_back(&replaySeconds);
	}
	item.emplace_back(&buttonsHeading);
	item.emplace_back(&turboStyle);
}

}

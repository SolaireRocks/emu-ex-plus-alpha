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

#include "MacroView.hh"
#include <emuframework/EmuApp.hh>
#include <emuframework/viewUtils.hh>
import imagine;

namespace EmuEx
{

static std::string macroSummary(const Macro &m)
{
	if(m.empty())
		return "Empty";
	return std::format("{} step{}, {:.2f}s{}", m.steps.size(), m.steps.size() == 1 ? "" : "s",
		m.totalFrames() / 60., m.loop ? ", loops" : "");
}

// MacroListView

MacroListView::MacroListView(ViewAttachParams attach):
	TableView{"Macros", attach, item},
	helpHeading{"Add \"Macro Buttons\" in On-screen Input Setup", attach}
{
	reloadItems();
}

void MacroListView::onShow()
{
	TableView::onShow();
	reloadItems();
	place();
}

void MacroListView::reloadItems()
{
	macroItems.clear();
	macroItems.reserve(macroCount);
	item.clear();
	for(int i = 0; i < macroCount; i++)
	{
		auto &menuItem = macroItems.emplace_back(
			std::format("Macro {}: {}", i + 1, macroSummary(app().macroManager.macros[i])), attachParams(),
			[this](TextMenuItem &item, const Input::Event &e)
			{
				pushAndShow(makeView<MacroEditView>(int(item.id.val)), e);
			});
		menuItem.id = i;
		item.emplace_back(&menuItem);
	}
	item.emplace_back(&helpHeading);
}

// MacroEditView

MacroEditView::MacroEditView(ViewAttachParams attach, int macroIdx_):
	TableView{std::format("Macro {}", macroIdx_ + 1), attach, item},
	macroIdx{macroIdx_},
	playNow
	{
		"Play Now", attach,
		[this]
		{
			if(!system().hasContent())
			{
				app().postMessage("Load a game first");
				return;
			}
			app().showEmulation();
			app().macroManager.play(app(), macroIdx);
		}
	},
	recordNow
	{
		"Record From Gameplay", attach,
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
			{
				app().postMessage("Load a game first");
				return;
			}
			auto doRecord = [this]
			{
				app().macroManager.startRecording(app(), macroIdx);
				app().showEmulation();
				app().postMessage(5, false, std::format("Recording Macro {}: play your inputs, then open the menu or press Record Macro to finish", macroIdx + 1));
			};
			if(macro().empty())
			{
				doRecord();
				return;
			}
			pushAndShowModal(makeView<YesNoAlertView>("Replace this macro's steps with a new recording?",
				YesNoAlertView::Delegates{.onYes = doRecord}), e);
		}
	},
	loop
	{
		"Repeat While Active (Tap Button Again To Stop)", attach,
		app().macroManager.macros[macroIdx_].loop,
		[this](BoolMenuItem &item) { macro().loop = item.flipBoolValue(*this); }
	},
	addStep
	{
		"Add Step", attach,
		[this](const Input::Event &e)
		{
			if(macro().steps.size() >= Macro::maxSteps)
			{
				app().postMessage("Macro step limit reached");
				return;
			}
			macro().steps.emplace_back(MacroStep{.frames = 6});
			pushAndShow(makeView<MacroStepEditView>(macroIdx, int(macro().steps.size() - 1)), e);
		}
	},
	clearSteps
	{
		"Clear All Steps", attach,
		[this](const Input::Event &e)
		{
			if(macro().empty())
				return;
			pushAndShowModal(makeView<YesNoAlertView>("Really clear all steps?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						macro().steps.clear();
						reloadItems();
						place();
					}
				}), e);
		}
	},
	stepsHeading{"Steps (1 Frame = 1/60s)", attach}
{
	reloadItems();
}

Macro &MacroEditView::macro() { return app().macroManager.macros[macroIdx]; }

void MacroEditView::onShow()
{
	TableView::onShow();
	reloadItems();
	place();
}

void MacroEditView::reloadItems()
{
	auto &m = macro();
	stepItems.clear();
	stepItems.reserve(m.steps.size());
	item.clear();
	item.emplace_back(&playNow);
	item.emplace_back(&recordNow);
	item.emplace_back(&loop);
	item.emplace_back(&addStep);
	item.emplace_back(&clearSteps);
	item.emplace_back(&stepsHeading);
	for(size_t i = 0; i < m.steps.size(); i++)
	{
		auto &s = m.steps[i];
		auto &menuItem = stepItems.emplace_back(
			std::format("{}. {} ({} frame{})", i + 1, app().macroManager.describe(app().inputManager, s), s.frames, s.frames == 1 ? "" : "s"),
			attachParams(),
			[this](TextMenuItem &item, const Input::Event &e)
			{
				pushAndShow(makeView<MacroStepEditView>(macroIdx, int(item.id.val)), e);
			});
		menuItem.id = int(i);
		item.emplace_back(&menuItem);
	}
}

// MacroStepEditView

MacroStepEditView::MacroStepEditView(ViewAttachParams attach, int macroIdx_, int stepIdx_):
	TableView{std::format("Macro {} Step {}", macroIdx_ + 1, stepIdx_ + 1), attach, item},
	macroIdx{macroIdx_},
	stepIdx{stepIdx_},
	frames
	{
		"Hold For (Frames)", std::to_string(app().macroManager.macros[macroIdx_].steps[stepIdx_].frames), attach,
		[this](const Input::Event &e)
		{
			if(!stepIsValid())
				return;
			pushAndShowNewCollectValueRangeInputView<int, 1, 3600>(attachParams(), e,
				"Input 1 to 3600 (60 = 1 second)", std::to_string(step().frames),
				[this](CollectTextInputView &, auto val)
				{
					if(!stepIsValid())
						return true;
					step().frames = val;
					frames.set2ndName(std::to_string(val));
					frames.place2nd();
					return true;
				});
		}
	},
	insertAfter
	{
		"Insert New Step After This One", attach,
		[this](const Input::Event &e)
		{
			auto &steps = app().macroManager.macros[macroIdx].steps;
			if(!stepIsValid() || steps.size() >= Macro::maxSteps)
				return;
			steps.insert(steps.begin() + stepIdx + 1, MacroStep{.frames = 6});
			// the step list refreshes when it's shown again
			pushAndShow(makeView<MacroStepEditView>(macroIdx, stepIdx + 1), e);
		}
	},
	moveUp
	{
		"Move Step Up", attach,
		[this]
		{
			auto &steps = app().macroManager.macros[macroIdx].steps;
			if(!stepIsValid() || stepIdx == 0)
				return;
			std::swap(steps[stepIdx], steps[stepIdx - 1]);
			dismiss();
		}
	},
	remove
	{
		"Delete Step", attach,
		[this]
		{
			auto &steps = app().macroManager.macros[macroIdx].steps;
			if(!stepIsValid())
				return;
			steps.erase(steps.begin() + stepIdx);
			dismiss();
		}
	},
	keysHeading{"Buttons Held During This Step", attach}
{
	item.emplace_back(&frames);
	item.emplace_back(&keysHeading);
	// list each distinct system key once (skip turbo variants & combos)
	auto &cat = AppMeta::keyCategories()[0];
	for(auto &k : cat.keys)
	{
		if(k.flags.turbo || k.flags.toggle || k.codes[1] || std::ranges::contains(keyCodes, k.codes[0]))
			continue;
		keyCodes.emplace_back(k.codes[0]);
	}
	keyItems.reserve(keyCodes.size());
	for(auto code : keyCodes)
	{
		auto &keyItem = keyItems.emplace_back(app().inputManager.toString(code, KeyFlags{}), attach, step().hasKey(code),
			[this, code](BoolMenuItem &item)
			{
				if(!stepIsValid())
					return;
				bool on = item.flipBoolValue(*this);
				if(on)
				{
					if(!step().addKey(code))
					{
						item.setBoolValue(false, *this);
						app().postMessage(std::format("A step can hold at most {} buttons", MacroStep::maxKeys));
					}
				}
				else
				{
					step().removeKey(code);
				}
			});
		item.emplace_back(&keyItem);
	}
	item.emplace_back(&insertAfter);
	item.emplace_back(&moveUp);
	item.emplace_back(&remove);
}

bool MacroStepEditView::stepIsValid()
{
	return stepIdx >= 0 && size_t(stepIdx) < app().macroManager.macros[macroIdx].steps.size();
}

MacroStep &MacroStepEditView::step() { return app().macroManager.macros[macroIdx].steps[stepIdx]; }

}

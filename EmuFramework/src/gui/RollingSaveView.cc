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

#include "RollingSaveView.hh"
#include <emuframework/EmuApp.hh>
import imagine;

namespace EmuEx
{

RollingSaveView::RollingSaveView(ViewAttachParams attach):
	TableView{"Rolling Save States", attach, item},
	saveNow
	{
		"Save Rolling State Now", attach,
		[this]
		{
			if(!app().rollingSaveManager.maxStates())
			{
				app().postMessage("Rolling save states are off, enable them in Options ➔ Extras");
				return;
			}
			if(app().rollingSaveManager.save(RollingSaveKind::manual, true))
			{
				reloadItems();
				place();
			}
		}
	},
	statesHeading{"Tap A State To Load It (Newest First)", attach}
{
	reloadItems();
}

void RollingSaveView::onShow()
{
	TableView::onShow();
	reloadItems();
	place();
}

void RollingSaveView::reloadItems()
{
	entries = app().rollingSaveManager.entries();
	entryItems.clear();
	entryItems.reserve(entries.size());
	item.clear();
	item.emplace_back(&saveNow);
	item.emplace_back(&statesHeading);
	auto ctx = appContext();
	for(size_t i = 0; i < entries.size(); i++)
	{
		auto &e = entries[i];
		auto &menuItem = entryItems.emplace_back(
			std::format("{} {} - {}", e.kind == RollingSaveKind::autoSave ? "Auto" : "Manual", e.index + 1, ctx.formatDateAndTime(e.time)), attachParams(),
			[this](TextMenuItem &item, const Input::Event &ev)
			{
				auto idx = size_t(item.id.val);
				if(idx >= entries.size())
					return;
				pushAndShowModal(makeView<YesNoAlertView>("Load this state?",
					YesNoAlertView::Delegates
					{
						.onYes = [this, idx]
						{
							if(idx < entries.size() && app().rollingSaveManager.load(entries[idx]))
								app().showEmulation();
						}
					}), ev);
			});
		menuItem.id = int(i);
		item.emplace_back(&menuItem);
	}
	if(entries.empty())
		statesHeading.compile("No Rolling States Saved Yet");
	else
		statesHeading.compile("Tap A State To Load It (Newest First)");
}

}

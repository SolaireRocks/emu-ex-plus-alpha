#pragma once

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

#include <emuframework/EmuAppHelper.hh>
#include <emuframework/MacroManager.hh>
#ifndef IG_USE_MODULE_IMAGINE
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#endif

namespace EmuEx
{

using namespace IG;

// Lists all macros
class MacroListView : public TableView, public EmuAppHelper
{
public:
	MacroListView(ViewAttachParams attach);
	void onShow() final;

private:
	TextHeadingMenuItem helpHeading;
	std::vector<TextMenuItem> macroItems;
	std::vector<MenuItem*> item;

	void reloadItems();
};

// Edit the steps of one macro
class MacroEditView : public TableView, public EmuAppHelper
{
public:
	MacroEditView(ViewAttachParams attach, int macroIdx);
	void onShow() final;
	Macro &macro();

private:
	int macroIdx;
	TextMenuItem playNow;
	TextMenuItem recordNow;
	BoolMenuItem loop;
	TextMenuItem addStep;
	TextMenuItem clearSteps;
	TextHeadingMenuItem stepsHeading;
	std::vector<TextMenuItem> stepItems;
	std::vector<MenuItem*> item;

	void reloadItems();
};

// Edit a single step: which keys are held and for how long
class MacroStepEditView : public TableView, public EmuAppHelper
{
public:
	MacroStepEditView(ViewAttachParams attach, int macroIdx, int stepIdx);

private:
	int macroIdx;
	int stepIdx;
	DualTextMenuItem frames;
	TextMenuItem insertAfter;
	TextMenuItem moveUp;
	TextMenuItem remove;
	TextHeadingMenuItem keysHeading;
	std::vector<KeyCode> keyCodes;
	std::vector<BoolMenuItem> keyItems;
	std::vector<MenuItem*> item;

	MacroStep &step();
	bool stepIsValid();
};

}

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
#ifndef IG_USE_MODULE_IMAGINE
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#endif

namespace EmuEx
{

using namespace IG;
enum class AltSpeedMode;

class ExtrasOptionView;

struct FastSpeedOption
{
	ExtrasOptionView &view;
	AltSpeedMode mode;
	TextMenuItem items[6];
	MultiChoiceMenuItem choice;

	FastSpeedOption(ExtrasOptionView &, ViewAttachParams, AltSpeedMode, const char *name);
};

// Options for features added in the custom build
class ExtrasOptionView : public TableView, public EmuAppHelper
{
public:
	ExtrasOptionView(ViewAttachParams attach);

protected:
	TextHeadingMenuItem speedHeading;
	FastSpeedOption fastSpeed1;
	FastSpeedOption fastSpeed2;
	FastSpeedOption fastSpeed3;
	TextHeadingMenuItem rollingHeading;
	TextMenuItem rollingStatesItems[6];
	MultiChoiceMenuItem rollingStates;
	TextMenuItem rollingIntervalItems[6];
	MultiChoiceMenuItem rollingInterval;
	TextHeadingMenuItem screenshotHeading;
	TextMenuItem screenshotScaleItems[6];
	MultiChoiceMenuItem screenshotScale;
	BoolMenuItem screenshotAspect;
	BoolMenuItem doubleTapScreenshot;
	TextHeadingMenuItem recordingHeading;
	TextMenuItem recordingScaleItems[4];
	MultiChoiceMenuItem recordingScale;
	TextMenuItem replayModeItems[3];
	MultiChoiceMenuItem replayMode;
	TextMenuItem replaySecondsItems[5];
	MultiChoiceMenuItem replaySeconds;
	TextHeadingMenuItem buttonsHeading;
	BoolMenuItem turboStyle;
	std::vector<MenuItem*> item;
};

}

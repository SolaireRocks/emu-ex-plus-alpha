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

#include <emuframework/defs.hh>
#ifndef IG_USE_MODULE_IMAGINE
#include <imagine/base/PausableTimer.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/time/Time.hh>

namespace IG
{
class MapIO;
class FileIO;
}
#endif
#ifndef IG_USE_MODULE_STD
#include <vector>
#endif

namespace EmuEx
{

using namespace IG;

class EmuApp;

// Saves states to a fixed number of files, overwriting the oldest one each time.
// Manual saves (button or menu) and timer auto saves use separate sets of files,
// so an auto save never overwrites a manual one.
enum class RollingSaveKind : uint8_t
{
	manual, autoSave
};

class RollingSaveManager
{
public:
	struct Entry
	{
		FS::PathString path;
		WallClockTimePoint time;
		int index{};
		RollingSaveKind kind{};
	};

	static constexpr int maxStatesLimit = 50;
	static constexpr Seconds defaultInterval{180};
	static constexpr uint8_t defaultMaxStates = 5;

	RollingSaveManager(EmuApp &);
	bool save(RollingSaveKind, bool notify = false);
	bool loadNewest();
	bool load(const Entry &);
	std::vector<Entry> entries() const; // existing states of both kinds, newest first
	void startTimer();
	void pauseTimer();
	void cancelTimer();
	void setMaxStates(int);
	int maxStates() const { return maxStates_; }
	void setInterval(Seconds);
	Seconds interval() const { return saveTimer.frequency; }
	void setAutoSave(bool);
	bool autoSave() const { return autoSave_; }
	bool readConfig(MapIO &, unsigned key);
	void writeConfig(FileIO &) const;

private:
	EmuApp &app;
	PausableTimer<Seconds> saveTimer;
	uint8_t maxStates_{defaultMaxStates};
	bool autoSave_{true};

	FS::PathString statePath(RollingSaveKind, int idx) const;
	void restartTimer();
};

}

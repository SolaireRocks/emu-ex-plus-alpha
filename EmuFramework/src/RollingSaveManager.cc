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

#include <emuframework/RollingSaveManager.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
import imagine;

namespace EmuEx
{

using namespace IG;

constexpr SystemLogger log{"RollingSaveMgr"};

RollingSaveManager::RollingSaveManager(EmuApp &app_):
	app{app_},
	saveTimer
	{
		defaultInterval,
		{.debugLabel = "RollingSaveManager::saveTimer"},
		[this]
		{
			log.debug("running rolling save state timer");
			save(RollingSaveKind::autoSave);
			saveTimer.update();
			return true;
		}
	} {}

FS::PathString RollingSaveManager::statePath(RollingSaveKind kind, int idx) const
{
	auto &sys = app.system();
	auto name = std::format("{}.{}{:02}", sys.contentName(), kind == RollingSaveKind::autoSave ? "autorolling" : "rolling", idx);
	return sys.statePath(sys.stateFilename(name));
}

bool RollingSaveManager::save(RollingSaveKind kind, bool notify)
{
	auto &sys = app.system();
	if(!maxStates_ || !sys.hasContent())
		return false;
	auto ctx = app.appContext();
	// use the first free slot or overwrite the oldest one
	int slotToUse = 0;
	WallClockTimePoint oldestTime{WallClockTimePoint::max()};
	for(auto i: iotaCount(int(maxStates_)))
	{
		auto path = statePath(kind, i);
		if(!ctx.fileUriExists(path))
		{
			slotToUse = i;
			break;
		}
		auto time = ctx.fileUriLastWriteTime(path);
		if(time < oldestTime)
		{
			oldestTime = time;
			slotToUse = i;
		}
	}
	bool isAuto = kind == RollingSaveKind::autoSave;
	log.info("saving {} rolling state to slot:{}", isAuto ? "auto" : "manual", slotToUse);
	bool success = app.saveState(statePath(kind, slotToUse), false);
	if(success && notify)
		app.postMessage(1, false, std::format("Saved {} Rolling State {}", isAuto ? "Auto" : "Manual", slotToUse + 1));
	return success;
}

std::vector<RollingSaveManager::Entry> RollingSaveManager::entries() const
{
	std::vector<Entry> list;
	if(!app.system().hasContent())
		return list;
	auto ctx = app.appContext();
	for(auto kind : {RollingSaveKind::manual, RollingSaveKind::autoSave})
	{
		for(auto i: iotaCount(int(maxStatesLimit)))
		{
			auto path = statePath(kind, i);
			if(!ctx.fileUriExists(path))
				continue;
			list.emplace_back(path, ctx.fileUriLastWriteTime(path), i, kind);
		}
	}
	std::ranges::sort(list, [](auto &a, auto &b){ return a.time > b.time; });
	return list;
}

bool RollingSaveManager::load(const Entry &e)
{
	if(!app.loadState(e.path))
		return false;
	restartTimer();
	return true;
}

void RollingSaveManager::restartTimer()
{
	saveTimer.cancel();
	if(app.system().isActive())
		startTimer();
}

bool RollingSaveManager::loadNewest()
{
	auto list = entries();
	if(list.empty())
	{
		app.postMessage("No rolling save states yet");
		return false;
	}
	if(!load(list.front()))
		return false;
	auto &e = list.front();
	app.postMessage(1, false, std::format("Loaded {} Rolling State {}", e.kind == RollingSaveKind::autoSave ? "Auto" : "Manual", e.index + 1));
	return true;
}

void RollingSaveManager::startTimer()
{
	if(!maxStates_ || !autoSave_)
		return;
	saveTimer.start();
}

void RollingSaveManager::pauseTimer()
{
	saveTimer.pause();
}

void RollingSaveManager::cancelTimer()
{
	saveTimer.cancel();
}

void RollingSaveManager::setMaxStates(int max)
{
	maxStates_ = std::clamp(max, 0, maxStatesLimit);
	if(!maxStates_)
		saveTimer.cancel();
}

void RollingSaveManager::setAutoSave(bool on)
{
	autoSave_ = on;
	if(!on)
		saveTimer.cancel();
	else if(app.system().hasContent())
		restartTimer();
}

void RollingSaveManager::setInterval(Seconds secs)
{
	saveTimer.frequency = std::clamp(secs, Seconds{10}, Seconds{3600});
}

bool RollingSaveManager::readConfig(MapIO &io, unsigned key)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_ROLLING_SAVE_STATES: return readOptionValue<uint8_t>(io, [&](auto v){ setMaxStates(v); });
		case CFGKEY_ROLLING_SAVE_INTERVAL_SECS: return readOptionValue<int16_t>(io, [&](auto s){ setInterval(Seconds{s}); });
		case CFGKEY_ROLLING_SAVE_AUTO: return readOptionValue(io, autoSave_);
	}
}

void RollingSaveManager::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_ROLLING_SAVE_STATES, maxStates_, defaultMaxStates);
	writeOptionValueIfNotDefault(io, CFGKEY_ROLLING_SAVE_AUTO, autoSave_, true);
	writeOptionValueIfNotDefault(io, CFGKEY_ROLLING_SAVE_INTERVAL_SECS, int16_t(saveTimer.frequency.count()), int16_t(defaultInterval.count()));
}

}

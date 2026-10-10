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

#include <emuframework/MacroManager.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
import imagine;

namespace EmuEx
{

using namespace IG;

constexpr SystemLogger log{"MacroManager"};

bool MacroStep::hasKey(KeyCode c) const
{
	return c && std::ranges::contains(keys, c);
}

bool MacroStep::addKey(KeyCode c)
{
	if(!c || hasKey(c))
		return true;
	for(auto &k : keys)
	{
		if(!k)
		{
			k = c;
			return true;
		}
	}
	return false;
}

void MacroStep::removeKey(KeyCode c)
{
	if(!c)
		return;
	for(auto &k : keys)
	{
		if(k == c)
			k = 0;
	}
	// keep used keys packed at the front
	std::ranges::stable_partition(keys, [](KeyCode k){ return k != 0; });
}

int MacroStep::keyCount() const
{
	return std::ranges::count_if(keys, [](KeyCode k){ return k != 0; });
}

int Macro::totalFrames() const
{
	int frames{};
	for(auto &s : steps) { frames += s.frames; }
	return frames;
}

void MacroManager::play(EmuApp &app, int idx)
{
	if(idx < 0 || idx >= macroCount)
		return;
	auto &macro = macros[idx];
	if(macro.empty())
	{
		app.postMessage(3, false, std::format("Macro {} is empty, set it up in System Actions ➔ Macros", idx + 1));
		return;
	}
	if(isRecording())
	{
		app.postMessage("Can't play a macro while recording one");
		return;
	}
	auto suspendCtx = app.suspendEmulationThread();
	if(isPlaying() && playIdx == idx)
	{
		// pushing the button of a playing looped macro stops it, otherwise restart
		if(macro.loop)
		{
			releaseAll(app);
			playing = false;
			playIdx = -1;
			log.info("stopped looping macro:{}", idx);
			return;
		}
	}
	releaseAll(app);
	playIdx = idx;
	stepIdx = 0;
	frameInStep = 0;
	playing = true;
	log.info("playing macro:{} with {} steps", idx, macro.steps.size());
}

void MacroManager::stop(EmuApp &app)
{
	if(!isPlaying())
		return;
	auto suspendCtx = app.suspendEmulationThread();
	releaseAll(app);
	playing = false;
	playIdx = -1;
}

void MacroManager::releaseAll(EmuApp &app)
{
	for(auto k : pressedKeys)
	{
		app.handleSystemKeyInput(k, Input::Action::RELEASED, 0, {.allowTurboModifier = false});
	}
	pressedKeys.clear();
}

void MacroManager::applyKeys(EmuApp &app, const MacroStep &step)
{
	auto player = app.defaultVController().inputPlayer();
	auto makeKey = [&](KeyCode c)
	{
		KeyInfo k{c};
		return app.inputManager.transpose(k, player);
	};
	// release keys not held in the new step
	for(auto it = pressedKeys.begin(); it != pressedKeys.end();)
	{
		if(!step.hasKey(it->codes[0]))
		{
			app.handleSystemKeyInput(*it, Input::Action::RELEASED, 0, {.allowTurboModifier = false});
			it = pressedKeys.erase(it);
		}
		else
		{
			++it;
		}
	}
	// push newly held keys
	for(auto c : step.keys)
	{
		if(!c || std::ranges::any_of(pressedKeys, [&](auto &k){ return k.codes[0] == c; }))
			continue;
		auto k = makeKey(c);
		app.handleSystemKeyInput(k, Input::Action::PUSHED, 0, {.allowTurboModifier = false});
		pressedKeys.emplace_back(k);
	}
}

void MacroManager::onFrame(EmuApp &app)
{
	if(isRecording())
	{
		recordFrame.fetch_add(1, std::memory_order_relaxed);
	}
	if(!isPlaying())
		return;
	auto &macro = macros[playIdx];
	if(stepIdx >= macro.steps.size())
	{
		if(macro.loop && !macro.steps.empty())
		{
			stepIdx = 0;
		}
		else
		{
			releaseAll(app);
			playing = false;
			playIdx = -1;
			return;
		}
	}
	auto &step = macro.steps[stepIdx];
	if(frameInStep == 0)
		applyKeys(app, step);
	if(++frameInStep >= std::max(int(step.frames), 1))
	{
		frameInStep = 0;
		stepIdx++;
	}
}

void MacroManager::startRecording(EmuApp &app, int idx)
{
	if(idx < 0 || idx >= macroCount)
		return;
	stop(app);
	auto suspendCtx = app.suspendEmulationThread();
	recordTarget = idx;
	recordHeld = {};
	recordedEvents.clear();
	recordFrame = 0;
	recording = true;
	log.info("started recording macro:{}", idx);
}

void MacroManager::stopRecording(EmuApp &app, bool keep)
{
	if(!isRecording())
		return;
	recording = false;
	auto endFrame = recordFrame.load();
	if(!keep)
	{
		recordedEvents.clear();
		return;
	}
	// convert the key state changes into timed steps
	Macro newMacro{.loop = macros[recordTarget].loop};
	for(size_t i = 0; i < recordedEvents.size(); i++)
	{
		auto &e = recordedEvents[i];
		auto nextFrame = i + 1 < recordedEvents.size() ? recordedEvents[i + 1].frame : endFrame;
		bool isLast = i + 1 == recordedEvents.size();
		bool keysHeld = std::ranges::any_of(e.held, [](KeyCode k){ return k != 0; });
		if(isLast && !keysHeld)
			break; // trailing idle time isn't useful
		MacroStep step{.keys = e.held, .frames = uint16_t(std::clamp(int(nextFrame - e.frame), 1, 0xFFFF))};
		if(newMacro.steps.size() && newMacro.steps.back().keys == step.keys)
		{
			newMacro.steps.back().frames = std::min(newMacro.steps.back().frames + step.frames, 0xFFFF);
			continue;
		}
		if(newMacro.steps.size() == Macro::maxSteps)
		{
			log.warn("macro step limit reached");
			break;
		}
		newMacro.steps.emplace_back(step);
	}
	recordedEvents.clear();
	if(newMacro.steps.empty())
	{
		app.postMessage("No input recorded, macro unchanged");
		return;
	}
	macros[recordTarget] = std::move(newMacro);
	log.info("recorded macro:{} with {} steps", recordTarget, macros[recordTarget].steps.size());
	app.postMessage(3, false, std::format("Saved Macro {} ({} steps, {:.2f}s)", recordTarget + 1,
		macros[recordTarget].steps.size(), macros[recordTarget].totalFrames() / 60.));
}

void MacroManager::toggleRecording(EmuApp &app)
{
	if(isRecording())
	{
		stopRecording(app);
	}
	else
	{
		startRecording(app, recordTarget);
		app.postMessage(3, false, std::format("Recording Macro {}, press Record Macro again or open the menu to finish", recordTarget + 1));
	}
}

void MacroManager::recordKeyEvent(KeyInfo key, bool pushed)
{
	if(!isRecording() || key.isAppKey())
		return;
	bool changed{};
	for(auto c : key.codes)
	{
		if(!c)
			continue;
		if(pushed)
		{
			if(std::ranges::contains(recordHeld, c))
				continue;
			for(auto &h : recordHeld)
			{
				if(!h)
				{
					h = c;
					changed = true;
					break;
				}
			}
		}
		else
		{
			for(auto &h : recordHeld)
			{
				if(h == c)
				{
					h = 0;
					changed = true;
				}
			}
		}
	}
	if(!changed)
		return;
	std::ranges::stable_partition(recordHeld, [](KeyCode k){ return k != 0; });
	if(recordedEvents.empty() && std::ranges::all_of(recordHeld, [](KeyCode k){ return k == 0; }))
		return; // don't start the macro with a release
	auto frame = recordFrame.load(std::memory_order_relaxed);
	if(recordedEvents.size() && recordedEvents.back().frame == frame)
	{
		// multiple changes during the same frame, keep the latest state but make sure a quick tap still lasts one frame
		if(std::ranges::all_of(recordHeld, [](KeyCode k){ return k == 0; }))
			recordedEvents.emplace_back(frame + 1, recordHeld);
		else
			recordedEvents.back().held = recordHeld;
		return;
	}
	if(recordedEvents.size() > Macro::maxSteps * 2)
		return;
	recordedEvents.emplace_back(frame, recordHeld);
}

std::string MacroManager::describe(const InputManager &mgr, const MacroStep &step) const
{
	std::string s;
	for(auto c : step.keys)
	{
		if(!c)
			continue;
		if(s.size())
			s += " + ";
		s += mgr.toString(c, KeyFlags{});
	}
	if(s.empty())
		s = "Wait";
	return s;
}

bool MacroManager::readConfig(MapIO &io, unsigned key)
{
	if(key != CFGKEY_MACROS)
		return false;
	auto count = std::min(int(io.get<uint8_t>()), macroCount);
	for(auto i: iotaCount(count))
	{
		auto &m = macros[i];
		auto flags = io.get<uint8_t>();
		m.loop = flags & 1;
		auto steps = std::min(size_t(io.get<uint16_t>()), Macro::maxSteps);
		m.steps.clear();
		m.steps.reserve(steps);
		for([[maybe_unused]] auto s: iotaCount(steps))
		{
			MacroStep step;
			for(auto &k : step.keys) { k = io.get<KeyCode>(); }
			step.frames = std::max(io.get<uint16_t>(), uint16_t(1));
			m.steps.emplace_back(step);
		}
	}
	log.info("read {} macros", count);
	return true;
}

void MacroManager::writeConfig(FileIO &io) const
{
	if(std::ranges::all_of(macros, [](auto &m){ return m.empty() && !m.loop; }))
		return;
	size_t bytes = 1;
	for(auto &m : macros)
	{
		bytes += 1 + 2 + m.steps.size() * (MacroStep::maxKeys + 2);
	}
	if(bytes > 0xFFF0)
	{
		log.error("macro data too large:{} bytes", bytes);
		return;
	}
	writeOptionValueHeader(io, CFGKEY_MACROS, bytes);
	io.put(uint8_t(macroCount));
	for(auto &m : macros)
	{
		io.put(uint8_t(m.loop ? 1 : 0));
		io.put(uint16_t(m.steps.size()));
		for(auto &s : m.steps)
		{
			for(auto k : s.keys) { io.put(KeyCode(k)); }
			io.put(uint16_t(s.frames));
		}
	}
}

}

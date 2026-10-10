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

#include <emuframework/inputDefs.hh>
#ifndef IG_USE_MODULE_STD
#include <array>
#include <vector>
#include <atomic>
#include <string>
#endif

#ifndef IG_USE_MODULE_IMAGINE
namespace IG
{
class MapIO;
class FileIO;
}
#endif

namespace EmuEx
{

using namespace IG;

class EmuApp;
class InputManager;

struct MacroStep
{
	static constexpr size_t maxKeys = 4;
	std::array<KeyCode, maxKeys> keys{}; // system key codes held during this step, 0 = unused
	uint16_t frames{1}; // how many emulated frames to hold this step

	bool hasKey(KeyCode c) const;
	bool addKey(KeyCode c);
	void removeKey(KeyCode c);
	int keyCount() const;
	constexpr bool operator==(const MacroStep&) const = default;
};

struct Macro
{
	static constexpr size_t maxSteps = 600;
	std::vector<MacroStep> steps{};
	bool loop{};

	bool empty() const { return steps.empty(); }
	int totalFrames() const;
};

inline constexpr int macroCount = 8;

class MacroManager
{
public:
	std::array<Macro, macroCount> macros;
	int recordTarget{};

	// main thread API
	void play(EmuApp &, int idx);
	void stop(EmuApp &);
	void startRecording(EmuApp &, int idx);
	void stopRecording(EmuApp &, bool keep = true);
	void toggleRecording(EmuApp &);
	bool isPlaying() const { return playing.load(std::memory_order_relaxed); }
	int playingIndex() const { return playIdx; }
	bool isRecording() const { return recording.load(std::memory_order_relaxed); }
	void recordKeyEvent(KeyInfo, bool pushed);

	// emulation thread API, called before every emulated frame when active
	bool needsPerFrameUpdates() const { return isPlaying() || isRecording(); }
	void onFrame(EmuApp &);

	std::string describe(const InputManager &, const MacroStep &) const;
	bool readConfig(MapIO &, unsigned key);
	void writeConfig(FileIO &) const;

private:
	struct RecordedEvent
	{
		uint32_t frame;
		std::array<KeyCode, MacroStep::maxKeys> held;
	};

	// playback state
	std::atomic_bool playing{};
	int playIdx{-1};
	size_t stepIdx{};
	int frameInStep{};
	std::vector<KeyInfo> pressedKeys;

	// recording state
	std::atomic_bool recording{};
	std::atomic<uint32_t> recordFrame{};
	std::array<KeyCode, MacroStep::maxKeys> recordHeld{};
	std::vector<RecordedEvent> recordedEvents;

	void applyKeys(EmuApp &, const MacroStep &);
	void releaseAll(EmuApp &);
};

}

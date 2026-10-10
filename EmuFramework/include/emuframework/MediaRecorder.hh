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
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/audio/Format.hh>

namespace IG
{
class MapIO;
class FileIO;
}
#endif
#ifndef IG_USE_MODULE_STD
#include <atomic>
#include <memory>
#endif

namespace EmuEx
{

using namespace IG;

class EmuApp;
struct MediaEncoderSession;

enum class ReplayBufferMode : uint8_t
{
	off,
	whenButtonPresent, // only buffer when an Instant Replay button is in the on-screen layout
	always,
};

// Records only the emulated video & audio (no on-screen controls or menus) to MP4 files with
// the device's hardware encoder. Supports a normal start/stop recording and an "instant replay"
// ring buffer that can save the last N seconds of gameplay at any time.
class MediaRecorder
{
public:
	static constexpr int16_t defaultReplaySeconds = 30;
	static constexpr int8_t defaultScale = 2;

	MediaRecorder(EmuApp &);
	~MediaRecorder();

	// main thread API
	bool isSupported() const;
	void toggleRecording();
	bool startRecording();
	void stopRecording();
	bool isRecording() const { return recordingRequested; }
	bool saveReplay();
	bool replayBufferWanted() const;
	void updateSessionState(); // start or stop the encoder depending on current options
	void onSystemClosed();
	bool readConfig(MapIO &, unsigned key);
	void writeConfig(FileIO &) const;

	// emulation thread API
	bool isActive() const { return active.load(std::memory_order_relaxed); }
	void onVideoFrame(PixmapView);
	void onAudioFrames(const void *samples, size_t frames, Audio::Format);
	void endFrame();

	int8_t scale{defaultScale};
	int16_t replaySeconds{defaultReplaySeconds};
	ReplayBufferMode replayMode{ReplayBufferMode::whenButtonPresent};

private:
	EmuApp &app;
	std::unique_ptr<MediaEncoderSession> session;
	std::atomic_bool active{};
	bool recordingRequested{};

	void startSession();
	void endSession();
};

}

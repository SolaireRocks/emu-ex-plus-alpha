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

#include <emuframework/MediaRecorder.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/AppKeyCode.hh>
#include <emuframework/Option.hh>
#ifdef __ANDROID__
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#endif
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <variant>
#include <memory>
#include <vector>
import imagine;

namespace EmuEx
{

using namespace IG;

constexpr SystemLogger log{"MediaRecorder"};

// Audio is always encoded as 48kHz stereo AAC, incoming audio is converted if needed
constexpr int encodeSampleRate = 48000;
constexpr int encodeChannels = 2;

static void postFromThread(EmuApp &app, std::string text, bool error)
{
	struct Msg { std::string text; bool error; };
	auto msg = new Msg{std::move(text), error};
	app.runOnMainThread([appPtr = &app, msg](ApplicationContext)
	{
		appPtr->postMessage(4, msg->error, msg->text);
		delete msg;
	});
}

#ifdef __ANDROID__

// Minimal NDK media API declarations, loaded at runtime from libmediandk.so (Android 5.0+)
// so the app still starts on older devices that don't have it.
namespace NdkMedia
{

struct AMediaCodec;
struct AMediaFormat;
struct AMediaMuxer;

struct BufferInfo
{
	int32_t offset;
	int32_t size;
	int64_t presentationTimeUs;
	uint32_t flags;
};

using Status = int;

constexpr uint32_t CONFIGURE_FLAG_ENCODE = 1;
constexpr uint32_t BUFFER_FLAG_KEY_FRAME = 1;
constexpr uint32_t BUFFER_FLAG_CODEC_CONFIG = 2;
constexpr ssize_t INFO_OUTPUT_FORMAT_CHANGED = -2;
constexpr ssize_t INFO_OUTPUT_BUFFERS_CHANGED = -3;
constexpr int MUXER_OUTPUT_FORMAT_MPEG_4 = 0;
constexpr int32_t COLOR_FormatYUV420Planar = 19;
constexpr int32_t COLOR_FormatYUV420SemiPlanar = 21;
constexpr int32_t COLOR_FormatYUV420Flexible = 0x7F420888;
constexpr int32_t AACObjectLC = 2;

struct Api
{
	void *lib{};
	AMediaCodec* (*codecCreateEncoderByType)(const char*){};
	Status (*codecConfigure)(AMediaCodec*, const AMediaFormat*, void *surface, void *crypto, uint32_t flags){};
	Status (*codecStart)(AMediaCodec*){};
	Status (*codecStop)(AMediaCodec*){};
	Status (*codecDelete)(AMediaCodec*){};
	ssize_t (*codecDequeueInputBuffer)(AMediaCodec*, int64_t timeoutUs){};
	uint8_t* (*codecGetInputBuffer)(AMediaCodec*, size_t idx, size_t *outSize){};
	Status (*codecQueueInputBuffer)(AMediaCodec*, size_t idx, long offset, size_t size, uint64_t time, uint32_t flags){};
	ssize_t (*codecDequeueOutputBuffer)(AMediaCodec*, BufferInfo*, int64_t timeoutUs){};
	uint8_t* (*codecGetOutputBuffer)(AMediaCodec*, size_t idx, size_t *outSize){};
	Status (*codecReleaseOutputBuffer)(AMediaCodec*, size_t idx, bool render){};
	AMediaFormat* (*codecGetOutputFormat)(AMediaCodec*){};
	AMediaFormat* (*codecGetInputFormat)(AMediaCodec*){}; // API 28+, optional
	Status (*codecSetParameters)(AMediaCodec*, const AMediaFormat*){}; // API 26+, optional
	AMediaFormat* (*formatNew)(){};
	Status (*formatDelete)(AMediaFormat*){};
	void (*formatSetInt32)(AMediaFormat*, const char *name, int32_t value){};
	void (*formatSetString)(AMediaFormat*, const char *name, const char *value){};
	bool (*formatGetInt32)(AMediaFormat*, const char *name, int32_t *out){};
	AMediaMuxer* (*muxerNew)(int fd, int format){};
	Status (*muxerDelete)(AMediaMuxer*){};
	ssize_t (*muxerAddTrack)(AMediaMuxer*, const AMediaFormat*){};
	Status (*muxerStart)(AMediaMuxer*){};
	Status (*muxerStop)(AMediaMuxer*){};
	Status (*muxerWriteSampleData)(AMediaMuxer*, size_t trackIdx, const uint8_t *data, const BufferInfo*){};

	bool load()
	{
		if(lib)
			return true;
		lib = dlopen("libmediandk.so", RTLD_NOW);
		if(!lib)
		{
			log.error("libmediandk.so not available");
			return false;
		}
		bool ok = true;
		auto loadSym = [&](auto &fn, const char *name, bool required = true)
		{
			fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(dlsym(lib, name));
			if(!fn && required)
			{
				log.error("missing symbol:{}", name);
				ok = false;
			}
		};
		loadSym(codecCreateEncoderByType, "AMediaCodec_createEncoderByType");
		loadSym(codecConfigure, "AMediaCodec_configure");
		loadSym(codecStart, "AMediaCodec_start");
		loadSym(codecStop, "AMediaCodec_stop");
		loadSym(codecDelete, "AMediaCodec_delete");
		loadSym(codecDequeueInputBuffer, "AMediaCodec_dequeueInputBuffer");
		loadSym(codecGetInputBuffer, "AMediaCodec_getInputBuffer");
		loadSym(codecQueueInputBuffer, "AMediaCodec_queueInputBuffer");
		loadSym(codecDequeueOutputBuffer, "AMediaCodec_dequeueOutputBuffer");
		loadSym(codecGetOutputBuffer, "AMediaCodec_getOutputBuffer");
		loadSym(codecReleaseOutputBuffer, "AMediaCodec_releaseOutputBuffer");
		loadSym(codecGetOutputFormat, "AMediaCodec_getOutputFormat");
		loadSym(codecGetInputFormat, "AMediaCodec_getInputFormat", false);
		loadSym(codecSetParameters, "AMediaCodec_setParameters", false);
		loadSym(formatNew, "AMediaFormat_new");
		loadSym(formatDelete, "AMediaFormat_delete");
		loadSym(formatSetInt32, "AMediaFormat_setInt32");
		loadSym(formatSetString, "AMediaFormat_setString");
		loadSym(formatGetInt32, "AMediaFormat_getInt32");
		loadSym(muxerNew, "AMediaMuxer_new");
		loadSym(muxerDelete, "AMediaMuxer_delete");
		loadSym(muxerAddTrack, "AMediaMuxer_addTrack");
		loadSym(muxerStart, "AMediaMuxer_start");
		loadSym(muxerStop, "AMediaMuxer_stop");
		loadSym(muxerWriteSampleData, "AMediaMuxer_writeSampleData");
		if(!ok)
		{
			dlclose(lib);
			lib = {};
		}
		return ok;
	}
};

static Api &api()
{
	static Api a;
	return a;
}

}

using namespace NdkMedia;

// tells the main thread a file is finished, so it can delete it if empty and drop it from the unfinished file list
static void postFileDone(EmuApp &app, std::string path, bool keepFile, std::string text, bool error)
{
	struct Msg { std::string path, text; bool keepFile, error; };
	auto msg = new Msg{std::move(path), std::move(text), keepFile, error};
	app.runOnMainThread([appPtr = &app, msg](ApplicationContext)
	{
		appPtr->mediaRecorder.onFileDone(msg->path, msg->keepFile);
		appPtr->postMessage(4, msg->error, msg->text);
		delete msg;
	});
}

struct EncodedPacket
{
	std::vector<uint8_t> data{};
	int64_t ptsUs{};
	uint32_t flags{};
	bool isAudio{};

	bool isKeyFrame() const { return !isAudio && (flags & BUFFER_FLAG_KEY_FRAME); }
};

using PacketPtr = std::shared_ptr<const EncodedPacket>;
using FormatPtr = std::shared_ptr<AMediaFormat>;

static FormatPtr makeFormatPtr(AMediaFormat *f)
{
	return FormatPtr{f, [](AMediaFormat *f){ if(f) api().formatDelete(f); }};
}

// Writes packets to an MP4 file, started from a video key frame
class Mp4Writer
{
public:
	Mp4Writer(int fd): fd{fd} {}

	~Mp4Writer()
	{
		finish();
	}

	bool write(const EncodedPacket &p, const FormatPtr &videoFmt, const FormatPtr &audioFmt)
	{
		if(failed)
			return false;
		if(!muxer)
		{
			if(!p.isKeyFrame() || !videoFmt || !audioFmt)
				return false; // wait for a key frame and both track formats
			muxer = api().muxerNew(fd, MUXER_OUTPUT_FORMAT_MPEG_4);
			if(!muxer)
			{
				log.error("error creating muxer");
				failed = true;
				return false;
			}
			videoTrack = api().muxerAddTrack(muxer, videoFmt.get());
			audioTrack = api().muxerAddTrack(muxer, audioFmt.get());
			if(videoTrack < 0 || audioTrack < 0 || api().muxerStart(muxer) != 0)
			{
				log.error("error starting muxer");
				failed = true;
				return false;
			}
			startPts = p.ptsUs;
			started = true;
		}
		if(p.ptsUs < startPts)
			return false;
		auto &lastPts = p.isAudio ? lastAudioPts : lastVideoPts;
		if(p.ptsUs - startPts <= lastPts)
			return false; // timestamps must increase
		lastPts = p.ptsUs - startPts;
		BufferInfo info{.offset = 0, .size = int32_t(p.data.size()), .presentationTimeUs = lastPts, .flags = p.flags & BUFFER_FLAG_KEY_FRAME};
		api().muxerWriteSampleData(muxer, p.isAudio ? audioTrack : videoTrack, p.data.data(), &info);
		if(!p.isAudio)
			durationUs = lastPts;
		return true;
	}

	bool finish()
	{
		bool hadData = started;
		if(muxer)
		{
			if(started)
				api().muxerStop(muxer);
			api().muxerDelete(muxer);
			muxer = {};
		}
		if(fd != -1)
		{
			::close(fd);
			fd = -1;
		}
		started = false;
		return hadData;
	}

	bool hasStarted() const { return started; }
	int64_t duration() const { return durationUs; }

private:
	int fd{-1};
	AMediaMuxer *muxer{};
	ssize_t videoTrack{-1};
	ssize_t audioTrack{-1};
	int64_t startPts{};
	int64_t lastVideoPts{-1};
	int64_t lastAudioPts{-1};
	int64_t durationUs{};
	bool started{};
	bool failed{};
};

struct VideoFrameItem
{
	std::vector<char> pixels{};
	PixmapDesc desc{};
	int pitchBytes{};
	int64_t ptsUs{};
};

struct AudioItem
{
	std::vector<int16_t> samples{}; // interleaved stereo
};

struct StartFileItem { int fd; std::string path; };
struct StopFileItem {};
struct SaveReplayItem { int fd; std::string path; };

using WorkItem = std::variant<VideoFrameItem, AudioItem, StartFileItem, StopFileItem, SaveReplayItem>;

struct MediaEncoderSession
{
	EmuApp &app;
	int scale;
	int64_t replayDurationUs; // 0 = no instant replay buffer
	std::thread thread;
	std::mutex mutex;
	std::condition_variable cond;
	std::condition_variable idleCond;
	std::deque<WorkItem> queue;
	bool quit{};
	bool busy{}; // worker is handling an item

	// emulation thread state
	int64_t emuAudioSamples{}; // total samples (at encodeSampleRate) given to the encoder, used as the clock
	int frameAudioSamples{};
	double silenceRemainder{};
	std::vector<int16_t> audioScratch;

	// worker thread state
	AMediaCodec *videoCodec{};
	AMediaCodec *audioCodec{};
	FormatPtr videoFormat, audioFormat;
	bool encoderError{};
	int width{}, height{}, stride{}, sliceHeight{};
	int32_t colorFormat{};
	std::vector<int> xMap;
	std::deque<int16_t> pendingPcm;
	int64_t audioSamplesQueued{};
	std::deque<PacketPtr> ring;
	std::unique_ptr<Mp4Writer> file;
	std::string filePath;

	MediaEncoderSession(EmuApp &app, int scale, int64_t replayDurationUs):
		app{app}, scale{scale}, replayDurationUs{replayDurationUs}
	{
		thread = std::thread{[this]{ run(); }};
	}

	~MediaEncoderSession()
	{
		{
			std::lock_guard lock{mutex};
			quit = true;
		}
		cond.notify_one();
		thread.join();
	}

	void push(WorkItem item)
	{
		{
			std::lock_guard lock{mutex};
			queue.emplace_back(std::move(item));
		}
		cond.notify_one();
	}

	size_t queueSize()
	{
		std::lock_guard lock{mutex};
		return queue.size();
	}

	// returns false if the worker is still busy after the timeout
	bool waitUntilIdle(std::chrono::milliseconds timeout)
	{
		std::unique_lock lock{mutex};
		return idleCond.wait_for(lock, timeout, [&]{ return queue.empty() && !busy; });
	}

	void run()
	{
		log.info("encoder thread started");
		while(true)
		{
			WorkItem item;
			{
				std::unique_lock lock{mutex};
				cond.wait(lock, [&]{ return quit || !queue.empty(); });
				if(queue.empty())
					break; // quit requested with no more work
				item = std::move(queue.front());
				queue.pop_front();
				busy = true;
			}
			process(item);
			drain();
			{
				std::lock_guard lock{mutex};
				busy = false;
			}
			idleCond.notify_all();
		}
		if(file)
		{
			drain();
			finishFile();
		}
		shutdownCodecs();
		log.info("encoder thread finished");
	}

	void process(WorkItem &item)
	{
		std::visit([&](auto &i){ handle(i); }, item);
	}

	void handle(VideoFrameItem &f)
	{
		if(encoderError)
			return;
		if(!videoCodec && !initEncoders(f.desc.w(), f.desc.h()))
		{
			encoderError = true;
			postFromThread(app, "Video recording isn't supported on this device", true);
			return;
		}
		auto idx = api().codecDequeueInputBuffer(videoCodec, 10000);
		if(idx < 0)
		{
			log.debug("dropped video frame, encoder busy");
			return;
		}
		size_t bufSize{};
		auto buf = api().codecGetInputBuffer(videoCodec, idx, &bufSize);
		size_t frameBytes = size_t(stride) * sliceHeight * 3 / 2;
		if(!buf || bufSize < frameBytes)
		{
			log.error("encoder input buffer too small:{} need:{}", bufSize, frameBytes);
			api().codecQueueInputBuffer(videoCodec, idx, 0, 0, f.ptsUs, 0);
			return;
		}
		convertToYUV(f, buf);
		api().codecQueueInputBuffer(videoCodec, idx, 0, frameBytes, f.ptsUs, 0);
	}

	void handle(AudioItem &a)
	{
		pendingPcm.insert(pendingPcm.end(), a.samples.begin(), a.samples.end());
		feedAudio();
	}

	void handle(StartFileItem &s)
	{
		if(file)
			finishFile();
		file = std::make_unique<Mp4Writer>(s.fd);
		filePath = std::move(s.path);
		requestKeyFrame();
		log.info("started recording to file");
	}

	void handle(StopFileItem &)
	{
		finishFile();
	}

	void handle(SaveReplayItem &s)
	{
		if(ring.empty() || !videoFormat || !audioFormat)
		{
			::close(s.fd);
			postFileDone(app, std::move(s.path), false, "Nothing to save yet, play for a moment first", true);
			return;
		}
		// copy the packet list so the file can be written without blocking the encoder
		std::vector<PacketPtr> packets{ring.begin(), ring.end()};
		makeDetachedThread([appPtr = &app, fd = s.fd, path = std::move(s.path), packets = std::move(packets),
			vFmt = videoFormat, aFmt = audioFormat, durationUs = replayDurationUs]()
		{
			writeReplay(*appPtr, fd, path, packets, vFmt, aFmt, durationUs);
		});
	}

	static void writeReplay(EmuApp &app, int fd, std::string path, const std::vector<PacketPtr> &packets,
		FormatPtr vFmt, FormatPtr aFmt, int64_t durationUs)
	{
		int64_t newestPts{};
		for(auto &p : packets) { if(!p->isAudio) newestPts = std::max(newestPts, p->ptsUs); }
		auto cutoff = newestPts - durationUs;
		// start from the newest key frame at or before the cutoff time, or the oldest one available
		size_t startIdx = packets.size();
		for(size_t i = 0; i < packets.size(); i++)
		{
			if(!packets[i]->isKeyFrame())
				continue;
			if(startIdx == packets.size() || packets[i]->ptsUs <= cutoff)
				startIdx = i;
			if(packets[i]->ptsUs > cutoff)
				break;
		}
		if(startIdx == packets.size())
		{
			::close(fd);
			postFileDone(app, std::move(path), false, "Nothing to save yet, play for a moment first", true);
			return;
		}
		Mp4Writer writer{fd};
		auto startPts = packets[startIdx]->ptsUs;
		writer.write(*packets[startIdx], vFmt, aFmt);
		for(auto &p : packets)
		{
			if(p == packets[startIdx] || p->ptsUs < startPts)
				continue;
			writer.write(*p, vFmt, aFmt);
		}
		auto secs = writer.duration() / 1000000.;
		if(writer.finish())
			postFileDone(app, std::move(path), true, std::format("Saved instant replay ({:.0f}s)", secs), false);
		else
			postFileDone(app, std::move(path), false, "Couldn't save the instant replay", true);
	}

	void finishFile()
	{
		if(!file)
			return;
		auto secs = file->duration() / 1000000.;
		bool hadData = file->finish();
		file.reset();
		if(hadData)
			postFileDone(app, std::move(filePath), true, std::format("Saved video recording ({:.0f}s)", secs), false);
		else
			postFileDone(app, std::move(filePath), false, "Recording stopped before any video was captured", true);
		filePath.clear();
	}

	void requestKeyFrame()
	{
		if(!videoCodec || !api().codecSetParameters)
			return;
		auto params = api().formatNew();
		api().formatSetInt32(params, "request-sync", 0);
		api().codecSetParameters(videoCodec, params);
		api().formatDelete(params);
	}

	AMediaCodec *makeVideoCodec(int32_t colorFmt, int bitrate)
	{
		auto codec = api().codecCreateEncoderByType("video/avc");
		if(!codec)
			return nullptr;
		auto fmt = api().formatNew();
		api().formatSetString(fmt, "mime", "video/avc");
		api().formatSetInt32(fmt, "width", width);
		api().formatSetInt32(fmt, "height", height);
		api().formatSetInt32(fmt, "color-format", colorFmt);
		api().formatSetInt32(fmt, "bitrate", bitrate);
		api().formatSetInt32(fmt, "frame-rate", 60);
		api().formatSetInt32(fmt, "i-frame-interval", 1);
		bool ok = api().codecConfigure(codec, fmt, nullptr, nullptr, CONFIGURE_FLAG_ENCODE) == 0
			&& api().codecStart(codec) == 0;
		api().formatDelete(fmt);
		if(!ok)
		{
			api().codecDelete(codec);
			return nullptr;
		}
		return codec;
	}

	bool initEncoders(int srcW, int srcH)
	{
		if(!api().load())
			return false;
		auto align16 = [](int v){ return (v + 15) & ~15; };
		width = align16(std::max(srcW * scale, 16));
		height = align16(std::max(srcH * scale, 16));
		int bitrate = std::clamp(int(width * height * 60 * 0.25), 1500000, 16000000);
		for(auto fmt : {COLOR_FormatYUV420SemiPlanar, COLOR_FormatYUV420Planar, COLOR_FormatYUV420Flexible})
		{
			videoCodec = makeVideoCodec(fmt, bitrate);
			if(videoCodec)
			{
				colorFormat = fmt;
				break;
			}
		}
		if(!videoCodec)
		{
			log.error("no usable AVC encoder for {}x{}", width, height);
			return false;
		}
		stride = width;
		sliceHeight = height;
		if(api().codecGetInputFormat)
		{
			if(auto inFmt = api().codecGetInputFormat(videoCodec); inFmt)
			{
				int32_t v{};
				if(api().formatGetInt32(inFmt, "stride", &v) && v >= width)
					stride = v;
				if(api().formatGetInt32(inFmt, "slice-height", &v) && v >= height)
					sliceHeight = v;
				if(colorFormat == COLOR_FormatYUV420Flexible && api().formatGetInt32(inFmt, "color-format", &v))
					colorFormat = v;
				api().formatDelete(inFmt);
			}
		}
		log.info("video encoder {}x{} (stride:{} slice:{}) color format:{} bitrate:{}", width, height, stride, sliceHeight, colorFormat, bitrate);
		// audio
		audioCodec = api().codecCreateEncoderByType("audio/mp4a-latm");
		if(audioCodec)
		{
			auto fmt = api().formatNew();
			api().formatSetString(fmt, "mime", "audio/mp4a-latm");
			api().formatSetInt32(fmt, "sample-rate", encodeSampleRate);
			api().formatSetInt32(fmt, "channel-count", encodeChannels);
			api().formatSetInt32(fmt, "bitrate", 160000);
			api().formatSetInt32(fmt, "aac-profile", AACObjectLC);
			api().formatSetInt32(fmt, "max-input-size", 16384);
			bool ok = api().codecConfigure(audioCodec, fmt, nullptr, nullptr, CONFIGURE_FLAG_ENCODE) == 0
				&& api().codecStart(audioCodec) == 0;
			api().formatDelete(fmt);
			if(!ok)
			{
				api().codecDelete(audioCodec);
				audioCodec = {};
			}
		}
		if(!audioCodec)
		{
			log.error("no usable AAC encoder");
			shutdownCodecs();
			return false;
		}
		return true;
	}

	void shutdownCodecs()
	{
		for(auto codec : {videoCodec, audioCodec})
		{
			if(!codec)
				continue;
			api().codecStop(codec);
			api().codecDelete(codec);
		}
		videoCodec = audioCodec = {};
	}

	static void readRGB(const char *p, PixelFormat fmt, int &r, int &g, int &b)
	{
		if(fmt == PixelFmtRGB565)
		{
			uint16_t v;
			std::memcpy(&v, p, 2);
			r = (v >> 11) & 0x1F; r = (r << 3) | (r >> 2);
			g = (v >> 5) & 0x3F; g = (g << 2) | (g >> 4);
			b = v & 0x1F; b = (b << 3) | (b >> 2);
		}
		else if(fmt == PixelFmtBGRA8888)
		{
			b = uint8_t(p[0]); g = uint8_t(p[1]); r = uint8_t(p[2]);
		}
		else
		{
			r = uint8_t(p[0]); g = uint8_t(p[1]); b = uint8_t(p[2]);
		}
	}

	void convertToYUV(const VideoFrameItem &f, uint8_t *out)
	{
		const int srcW = f.desc.w(), srcH = f.desc.h();
		const auto fmt = f.desc.format;
		const int bpp = fmt.bytesPerPixel();
		if(int(xMap.size()) != width || (xMap.size() && xMap.back() != ((width - 1) * srcW) / width))
		{
			xMap.resize(width);
			for(int x = 0; x < width; x++) { xMap[x] = (x * srcW) / width; }
		}
		uint8_t *yPlane = out;
		uint8_t *uvPlane = out + size_t(stride) * sliceHeight;
		const bool planar = colorFormat == COLOR_FormatYUV420Planar;
		uint8_t *uPlane = uvPlane;
		uint8_t *vPlane = uvPlane + size_t(stride / 2) * (sliceHeight / 2);
		for(int y = 0; y < height; y++)
		{
			int srcY = std::min((y * srcH) / height, srcH - 1);
			const char *srcRow = f.pixels.data() + size_t(srcY) * f.pitchBytes;
			uint8_t *yRow = yPlane + size_t(y) * stride;
			bool chromaRow = !(y & 1);
			for(int x = 0; x < width; x++)
			{
				int r, g, b;
				readRGB(srcRow + xMap[x] * bpp, fmt, r, g, b);
				yRow[x] = uint8_t(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
				if(chromaRow && !(x & 1))
				{
					auto u = uint8_t(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
					auto v = uint8_t(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
					if(planar)
					{
						size_t cIdx = size_t(y / 2) * (stride / 2) + x / 2;
						uPlane[cIdx] = u;
						vPlane[cIdx] = v;
					}
					else
					{
						size_t cIdx = size_t(y / 2) * stride + x;
						uvPlane[cIdx] = u;
						uvPlane[cIdx + 1] = v;
					}
				}
			}
		}
	}

	void feedAudio()
	{
		if(!audioCodec)
		{
			// encoders not ready yet, keep at most a few seconds of audio around
			while(pendingPcm.size() > size_t(encodeSampleRate * encodeChannels * 4))
				pendingPcm.pop_front();
			return;
		}
		while(pendingPcm.size() >= 1024 * encodeChannels)
		{
			auto idx = api().codecDequeueInputBuffer(audioCodec, 0);
			if(idx < 0)
				return;
			size_t bufSize{};
			auto buf = api().codecGetInputBuffer(audioCodec, idx, &bufSize);
			size_t samples = std::min(pendingPcm.size(), bufSize / sizeof(int16_t));
			samples -= samples % encodeChannels;
			auto out = reinterpret_cast<int16_t*>(buf);
			std::copy_n(pendingPcm.begin(), samples, out);
			pendingPcm.erase(pendingPcm.begin(), pendingPcm.begin() + samples);
			int64_t pts = audioSamplesQueued * 1000000 / encodeSampleRate;
			audioSamplesQueued += samples / encodeChannels;
			api().codecQueueInputBuffer(audioCodec, idx, 0, samples * sizeof(int16_t), pts, 0);
		}
	}

	void drainCodec(AMediaCodec *codec, bool isAudio)
	{
		while(true)
		{
			BufferInfo info{};
			auto idx = api().codecDequeueOutputBuffer(codec, &info, 0);
			if(idx == INFO_OUTPUT_FORMAT_CHANGED)
			{
				(isAudio ? audioFormat : videoFormat) = makeFormatPtr(api().codecGetOutputFormat(codec));
				log.info("{} output format ready", isAudio ? "audio" : "video");
				continue;
			}
			if(idx == INFO_OUTPUT_BUFFERS_CHANGED)
				continue;
			if(idx < 0)
				return;
			size_t size{};
			auto buf = api().codecGetOutputBuffer(codec, idx, &size);
			if(buf && info.size > 0 && !(info.flags & BUFFER_FLAG_CODEC_CONFIG))
			{
				auto packet = std::make_shared<EncodedPacket>();
				packet->data.assign(buf + info.offset, buf + info.offset + info.size);
				packet->ptsUs = info.presentationTimeUs;
				packet->flags = info.flags;
				packet->isAudio = isAudio;
				onPacket(std::move(packet));
			}
			api().codecReleaseOutputBuffer(codec, idx, false);
		}
	}

	void drain()
	{
		feedAudio();
		if(videoCodec)
			drainCodec(videoCodec, false);
		if(audioCodec)
			drainCodec(audioCodec, true);
	}

	void onPacket(PacketPtr p)
	{
		if(file)
			file->write(*p, videoFormat, audioFormat);
		if(replayDurationUs)
		{
			ring.emplace_back(std::move(p));
			trimRing();
		}
	}

	void trimRing()
	{
		int64_t newestPts{};
		for(auto it = ring.rbegin(); it != ring.rend(); ++it)
		{
			if(!(*it)->isAudio)
			{
				newestPts = (*it)->ptsUs;
				break;
			}
		}
		// keep a little extra so a key frame is always available at the start of the replay window
		auto cutoff = newestPts - replayDurationUs;
		while(true)
		{
			// find the 2nd key frame, if it's still before the cutoff everything before it can go
			size_t keyFrames{}, secondKeyIdx{};
			for(size_t i = 0; i < ring.size(); i++)
			{
				if(ring[i]->isKeyFrame() && ++keyFrames == 2)
				{
					secondKeyIdx = i;
					break;
				}
			}
			if(keyFrames < 2 || ring[secondKeyIdx]->ptsUs > cutoff)
				break;
			auto keepFromPts = ring[secondKeyIdx]->ptsUs;
			std::deque<PacketPtr> kept;
			for(size_t i = 0; i < secondKeyIdx; i++)
			{
				if(ring[i]->isAudio && ring[i]->ptsUs >= keepFromPts)
					kept.emplace_back(ring[i]);
			}
			ring.erase(ring.begin(), ring.begin() + secondKeyIdx);
			ring.insert(ring.begin(), kept.begin(), kept.end());
		}
	}

	// emulation thread
	void addAudio(const void *samples, size_t frames, Audio::Format fmt)
	{
		auto &buff = audioScratch;
		size_t outFrames = fmt.rate == encodeSampleRate ? frames : size_t(std::llround(double(frames) * encodeSampleRate / fmt.rate));
		if(!outFrames || !fmt.channels)
			return;
		buff.resize(outFrames * encodeChannels);
		auto sampleAt = [&](size_t frame, int ch) -> int16_t
		{
			int srcCh = std::min(ch, int(fmt.channels) - 1);
			size_t idx = frame * fmt.channels + srcCh;
			if(fmt.sample.isFloat())
				return int16_t(std::clamp(static_cast<const float*>(samples)[idx], -1.f, 1.f) * 32767.f);
			if(fmt.sample.bytes() == 2)
				return static_cast<const int16_t*>(samples)[idx];
			if(fmt.sample.bytes() == 4)
				return int16_t(static_cast<const int32_t*>(samples)[idx] >> 16);
			return int16_t((static_cast<const uint8_t*>(samples)[idx] - 128) << 8);
		};
		for(size_t i = 0; i < outFrames; i++)
		{
			size_t srcFrame = outFrames == frames ? i : std::min(frames - 1, (i * frames) / outFrames);
			for(int ch = 0; ch < encodeChannels; ch++)
				buff[i * encodeChannels + ch] = sampleAt(srcFrame, ch);
		}
		frameAudioSamples += outFrames;
		emuAudioSamples += outFrames;
		push(AudioItem{buff});
	}

	void endFrame(double fps)
	{
		if(!frameAudioSamples)
		{
			// no audio produced this frame (audio disabled, etc.), insert silence to keep A/V in sync
			double samples = encodeSampleRate / std::max(fps, 1.) + silenceRemainder;
			auto whole = int(samples);
			silenceRemainder = samples - whole;
			if(whole > 0)
			{
				push(AudioItem{std::vector<int16_t>(size_t(whole) * encodeChannels)});
				emuAudioSamples += whole;
			}
		}
		frameAudioSamples = 0;
	}

	int64_t clockUs() const { return emuAudioSamples * 1000000 / encodeSampleRate; }
};

static FS::PathString makeRecordingPath(EmuApp &app, std::string_view suffix)
{
	auto &sys = app.system();
	auto userPath = sys.userPath(app.userScreenshotPath);
	sys.createContentLocalDirectory(userPath, "recordings");
	return sys.contentLocalDirectory(userPath, "recordings",
		app.appContext().formatDateAndTimeAsFilename(WallClock::now()).append(suffix).append(".mp4"));
}

static int openRecordingFile(EmuApp &app, std::string_view suffix, std::string &pathOut)
{
	auto path = makeRecordingPath(app, suffix);
	auto fd = app.appContext().openFileUriFd(path, {.read = true, .write = true, .create = true, .truncate = true}).release();
	if(fd == -1)
	{
		log.error("error opening:{}", path);
		app.postErrorMessage(4, "Can't create video file, check Options ➔ File Paths ➔ Screenshots");
	}
	pathOut = std::string_view{path};
	return fd;
}

bool MediaRecorder::isSupported() const
{
	return api().load();
}

void MediaRecorder::startSession()
{
	if(session)
		return;
	auto suspendCtx = app.suspendEmulationThread();
	session = std::make_unique<MediaEncoderSession>(app, scale,
		replayBufferWanted() ? int64_t(replaySeconds) * 1000000 : 0);
	active = true;
	log.info("started encoder session");
}

void MediaRecorder::endSession()
{
	if(!session)
		return;
	auto suspendCtx = app.suspendEmulationThread();
	active = false;
	session.reset(); // waits for the encoder thread to finish writing any file
	log.info("ended encoder session");
}

void MediaRecorder::onVideoFrame(PixmapView pix)
{
	if(!session) [[unlikely]]
		return;
	if(session->queueSize() > 90)
		return; // encoder is falling behind, drop the frame
	VideoFrameItem item{.desc = pix.desc(), .pitchBytes = pix.pitchBytes(), .ptsUs = session->clockUs()};
	item.pixels.resize(size_t(pix.pitchBytes()) * pix.h());
	std::memcpy(item.pixels.data(), pix.data(), item.pixels.size());
	session->push(std::move(item));
}

void MediaRecorder::onAudioFrames(const void *samples, size_t frames, Audio::Format fmt)
{
	if(!session) [[unlikely]]
		return;
	session->addAudio(samples, frames, fmt);
}

void MediaRecorder::endFrame()
{
	if(!session) [[unlikely]]
		return;
	session->endFrame(app.system().frameRate().hz());
}

bool MediaRecorder::startRecording()
{
	if(recordingRequested)
		return true;
	if(!app.system().hasContent())
		return false;
	if(!isSupported())
	{
		app.postErrorMessage("Video recording needs Android 5.0 or newer");
		return false;
	}
	std::string path;
	auto fd = openRecordingFile(app, "", path);
	if(fd == -1)
		return false;
	trackFile(path);
	startSession();
	session->push(StartFileItem{fd, std::move(path)});
	recordingRequested = true;
	app.defaultVController().updateAppKeyHighlight(AppKeyCode::toggleVideoRecording, true);
	app.postMessage(2, false, "Recording started");
	return true;
}

void MediaRecorder::stopRecording()
{
	if(!recordingRequested)
		return;
	recordingRequested = false;
	app.defaultVController().updateAppKeyHighlight(AppKeyCode::toggleVideoRecording, false);
	if(session)
		session->push(StopFileItem{});
	updateSessionState();
}

#else

bool MediaRecorder::isSupported() const { return false; }
void MediaRecorder::startSession() {}
void MediaRecorder::endSession() {}
void MediaRecorder::onVideoFrame(PixmapView) {}
void MediaRecorder::onAudioFrames(const void*, size_t, Audio::Format) {}
void MediaRecorder::endFrame() {}

bool MediaRecorder::startRecording()
{
	app.postErrorMessage("Video recording isn't supported on this platform");
	return false;
}

void MediaRecorder::stopRecording() {}

struct MediaEncoderSession {};

#endif

MediaRecorder::MediaRecorder(EmuApp &app_): app{app_} {}

MediaRecorder::~MediaRecorder()
{
	active = false;
	session.reset();
}

void MediaRecorder::toggleRecording()
{
	if(recordingRequested)
		stopRecording();
	else
		startRecording();
}

bool MediaRecorder::replayBufferWanted() const
{
	switch(replayMode)
	{
		case ReplayBufferMode::off: return false;
		case ReplayBufferMode::always: return true;
		case ReplayBufferMode::whenButtonPresent:
		{
			auto &vCtrl = const_cast<EmuApp&>(app).defaultVController();
			auto hasReplayKey = [](std::span<VControllerElement> elems)
			{
				return std::ranges::any_of(elems, [](VControllerElement &e)
				{
					return std::ranges::any_of(e.buttons(), [](const VControllerButton &b)
					{
						return b.key.isAppKey() && AppKeyCode(b.key.codes[0]) == AppKeyCode::saveInstantReplay;
					});
				});
			};
			return hasReplayKey(vCtrl.guiElements()) || hasReplayKey(vCtrl.deviceElements());
		}
	}
	return false;
}

void MediaRecorder::updateSessionState()
{
	if(!isSupported())
		return;
	bool wantReplay = replayBufferWanted();
	bool wantSession = app.system().hasContent() && (recordingRequested || wantReplay);
	#ifdef __ANDROID__
	int64_t wantReplayUs = wantReplay ? int64_t(replaySeconds) * 1000000 : 0;
	bool settingsChanged = session && (session->scale != scale || session->replayDurationUs != wantReplayUs);
	#else
	bool settingsChanged = false;
	#endif
	// don't interrupt an active recording just because replay settings changed
	if(session && (!wantSession || (settingsChanged && !recordingRequested)))
		endSession();
	if(wantSession && !session)
		startSession();
}

bool MediaRecorder::saveReplay()
{
	if(!app.system().hasContent())
		return false;
	if(!isSupported())
	{
		app.postErrorMessage("Instant replay needs Android 5.0 or newer");
		return false;
	}
	if(!session || !replayBufferWanted())
	{
		app.postMessage(4, false, "Instant replay buffer isn't running, enable it in Options ➔ Extras");
		updateSessionState();
		return false;
	}
	#ifdef __ANDROID__
	std::string path;
	auto fd = openRecordingFile(app, "-replay", path);
	if(fd == -1)
		return false;
	trackFile(path);
	session->push(SaveReplayItem{fd, std::move(path)});
	app.postMessage(1, false, "Saving instant replay...");
	return true;
	#else
	return false;
	#endif
}

void MediaRecorder::onSystemClosed()
{
	if(recordingRequested)
		stopRecording();
	endSession();
}

void MediaRecorder::onAppBackgrounded()
{
	if(!recordingRequested)
		return;
	stopRecording();
	#ifdef __ANDROID__
	// finish the file now since Android may kill the app at any time while it's in the background
	if(session && !session->waitUntilIdle(std::chrono::seconds{3}))
		log.warn("timed out waiting for the recording to finish");
	#endif
}

// Files being written are listed in this file, so ones left unplayable by a crash
// or the app being killed can be found and deleted on the next launch
static FS::PathString openFileListPath(EmuApp &app)
{
	return FS::pathString(app.appContext().supportPath(), "recordingsInProgress");
}

// MediaMuxer only writes the MP4 index ('moov' box) when a file is closed properly
static bool isFinishedMp4(ApplicationContext ctx, CStringView uri)
{
	auto io = ctx.openFileUri(uri, {.test = true});
	if(!io)
		return false;
	const int64_t fileSize = io.size();
	int64_t pos{};
	while(pos + 8 <= fileSize)
	{
		uint8_t hdr[16];
		if(io.read(hdr, 8, off_t(pos)) != 8)
			return false;
		if(std::memcmp(hdr + 4, "moov", 4) == 0)
			return true;
		uint64_t boxSize = (uint32_t(hdr[0]) << 24) | (uint32_t(hdr[1]) << 16) | (uint32_t(hdr[2]) << 8) | hdr[3];
		if(boxSize == 1) // 64-bit size follows the type
		{
			if(io.read(hdr + 8, 8, off_t(pos + 8)) != 8)
				return false;
			boxSize = 0;
			for(int i = 8; i < 16; i++) { boxSize = (boxSize << 8) | hdr[i]; }
		}
		if(boxSize < 8 || boxSize > uint64_t(fileSize - pos))
			return false; // box runs to the end of the file or is cut off, no index after it
		pos += int64_t(boxSize);
	}
	return false;
}

void MediaRecorder::deleteUnfinishedFiles()
{
	auto ctx = app.appContext();
	auto listPath = openFileListPath(app);
	auto buff = FileUtils::bufferFromPath(listPath, {.test = true});
	if(!buff.size())
		return;
	for(auto line : std::views::split(buff.stringView(), '\n'))
	{
		std::string path(line.begin(), line.end());
		if(path.empty())
			continue;
		// the app could've been killed after finishing the file but before updating the list
		if(isFinishedMp4(ctx, path))
		{
			log.info("keeping finished recording:{}", path);
			continue;
		}
		log.info("deleting unfinished recording:{}", path);
		ctx.removeFileUri(path);
	}
	ctx.removeFileUri(listPath);
}

void MediaRecorder::trackFile(const std::string &path)
{
	openFiles.emplace_back(path);
	saveOpenFileList();
}

void MediaRecorder::onFileDone(const std::string &path, bool keepFile)
{
	if(!keepFile)
	{
		log.info("deleting empty recording:{}", path);
		app.appContext().removeFileUri(path);
	}
	std::erase(openFiles, path);
	saveOpenFileList();
}

void MediaRecorder::saveOpenFileList()
{
	auto listPath = openFileListPath(app);
	if(openFiles.empty())
	{
		app.appContext().removeFileUri(listPath);
		return;
	}
	std::string text;
	for(auto &p : openFiles) { text.append(p).append("\n"); }
	FileUtils::writeToPath(listPath, std::span{reinterpret_cast<const unsigned char*>(text.data()), text.size()});
}

bool MediaRecorder::readConfig(MapIO &io, unsigned key)
{
	switch(key)
	{
		default: return false;
		case CFGKEY_RECORDING_SCALE: return readOptionValue<int8_t>(io, [&](auto v){ scale = std::clamp(int(v), 1, 4); });
		case CFGKEY_REPLAY_SECONDS: return readOptionValue<int16_t>(io, [&](auto v){ replaySeconds = std::clamp(int(v), 5, 300); });
		case CFGKEY_REPLAY_MODE: return readOptionValue<ReplayBufferMode>(io, [&](auto v){ replayMode = v; },
			[](auto v){ return v <= ReplayBufferMode::always; });
	}
}

void MediaRecorder::writeConfig(FileIO &io) const
{
	writeOptionValueIfNotDefault(io, CFGKEY_RECORDING_SCALE, scale, defaultScale);
	writeOptionValueIfNotDefault(io, CFGKEY_REPLAY_SECONDS, replaySeconds, defaultReplaySeconds);
	writeOptionValueIfNotDefault(io, CFGKEY_REPLAY_MODE, replayMode, ReplayBufferMode::whenButtonPresent);
}

}

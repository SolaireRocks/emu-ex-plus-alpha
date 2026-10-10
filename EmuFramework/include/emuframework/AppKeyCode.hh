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
#include <emuframework/EmuSystem.hh>
#ifdef IG_USE_MODULES
import std;
#else
#include <array>
#include <string_view>
#endif

namespace EmuEx
{

using namespace IG;

enum class AppKeyCode : KeyCode
{
	openMenu = 1,
	openContent,
	openSystemActions,
	saveState,
	loadState,
	decStateSlot,
	incStateSlot,
	fastForward,
	toggleFastForward,
	slowMotion,
	toggleSlowMotion,
	takeScreenshot,
	turboModifier,
	exitApp,
	rewind,
	softReset,
	hardReset,
	resetMenu,
	closeContent,
	// Added in custom build, keep new values at the end since they are stored in config files
	fastForwardSpeed2,
	fastForwardSpeed3,
	toggleFastForwardSpeed2,
	toggleFastForwardSpeed3,
	saveRollingState,
	loadRollingState,
	toggleVideoRecording,
	saveInstantReplay,
	toggleMacroRecording,
	playMacro1,
	playMacro2,
	playMacro3,
	playMacro4,
	playMacro5,
	playMacro6,
	playMacro7,
	playMacro8,
};

inline constexpr int maxMacros = 8;

constexpr int macroIndex(AppKeyCode code)
{
	if(code >= AppKeyCode::playMacro1 && code <= AppKeyCode::playMacro8)
		return int(code) - int(AppKeyCode::playMacro1);
	return -1;
}

inline constexpr struct AppKeys
{
	KeyInfo
	openMenu = KeyInfo::appKey(AppKeyCode::openMenu),
	openContent = KeyInfo::appKey(AppKeyCode::openContent),
	closeContent = KeyInfo::appKey(AppKeyCode::closeContent),
	openSystemActions = KeyInfo::appKey(AppKeyCode::openSystemActions),
	saveState = KeyInfo::appKey(AppKeyCode::saveState),
	loadState = KeyInfo::appKey(AppKeyCode::loadState),
	decStateSlot = KeyInfo::appKey(AppKeyCode::decStateSlot),
	incStateSlot = KeyInfo::appKey(AppKeyCode::incStateSlot),
	fastForward = KeyInfo::appKey(AppKeyCode::fastForward),
	toggleFastForward = KeyInfo::appKey(AppKeyCode::toggleFastForward),
	slowMotion = KeyInfo::appKey(AppKeyCode::slowMotion),
	toggleSlowMotion = KeyInfo::appKey(AppKeyCode::toggleSlowMotion),
	rewind = KeyInfo::appKey(AppKeyCode::rewind),
	takeScreenshot = KeyInfo::appKey(AppKeyCode::takeScreenshot),
	turboModifier = KeyInfo::appKey(AppKeyCode::turboModifier),
	softReset = KeyInfo::appKey(AppKeyCode::softReset),
	hardReset = KeyInfo::appKey(AppKeyCode::hardReset),
	resetMenu = KeyInfo::appKey(AppKeyCode::resetMenu),
	fastForwardSpeed2 = KeyInfo::appKey(AppKeyCode::fastForwardSpeed2),
	fastForwardSpeed3 = KeyInfo::appKey(AppKeyCode::fastForwardSpeed3),
	toggleFastForwardSpeed2 = KeyInfo::appKey(AppKeyCode::toggleFastForwardSpeed2),
	toggleFastForwardSpeed3 = KeyInfo::appKey(AppKeyCode::toggleFastForwardSpeed3),
	saveRollingState = KeyInfo::appKey(AppKeyCode::saveRollingState),
	loadRollingState = KeyInfo::appKey(AppKeyCode::loadRollingState),
	toggleVideoRecording = KeyInfo::appKey(AppKeyCode::toggleVideoRecording),
	saveInstantReplay = KeyInfo::appKey(AppKeyCode::saveInstantReplay),
	toggleMacroRecording = KeyInfo::appKey(AppKeyCode::toggleMacroRecording),
	playMacro1 = KeyInfo::appKey(AppKeyCode::playMacro1),
	playMacro2 = KeyInfo::appKey(AppKeyCode::playMacro2),
	playMacro3 = KeyInfo::appKey(AppKeyCode::playMacro3),
	playMacro4 = KeyInfo::appKey(AppKeyCode::playMacro4),
	playMacro5 = KeyInfo::appKey(AppKeyCode::playMacro5),
	playMacro6 = KeyInfo::appKey(AppKeyCode::playMacro6),
	playMacro7 = KeyInfo::appKey(AppKeyCode::playMacro7),
	playMacro8 = KeyInfo::appKey(AppKeyCode::playMacro8),
	exitApp = KeyInfo::appKey(AppKeyCode::exitApp);

	constexpr const KeyInfo *data() const { return &openMenu; }
	static constexpr size_t size() { return sizeof(AppKeys) / sizeof(KeyInfo); }
} appKeys;

inline constexpr std::array genericGamepadAppKeyCodeMap
{
	KeyMapping{appKeys.openSystemActions, Input::Keycode::GAME_MODE},
	KeyMapping{appKeys.fastForward, Input::Keycode::GAME_R2},
	KeyMapping{appKeys.openMenu, Input::Keycode::BACK},
};

inline constexpr std::array genericGamepadModifierAppKeyCodeMap
{
	KeyMapping{appKeys.openSystemActions, Input::Keycode::GAME_MODE},
	KeyMapping{appKeys.fastForward, {Input::Keycode::GAME_R2, Input::Keycode::JS2_XAXIS_POS}},
	KeyMapping{appKeys.openMenu, Input::Keycode::BACK},
};

inline constexpr std::array genericWiimoteAppKeyCodeMap
{
	KeyMapping{appKeys.openSystemActions, Input::Keycode::GAME_MODE},
};

inline constexpr std::array genericKeyboardAppKeyCodeMap
{
	KeyMapping{appKeys.openContent, {Input::Keycode::LCTRL, Input::Keycode::_2}},
	KeyMapping{appKeys.openSystemActions, Input::Keycode::MENU},
	KeyMapping{appKeys.saveState, {Input::Keycode::LCTRL, Input::Keycode::_1}},
	KeyMapping{appKeys.loadState, {Input::Keycode::LCTRL, Input::Keycode::_4}},
	KeyMapping{appKeys.decStateSlot, {Input::Keycode::LCTRL, Input::Keycode::LEFT_BRACKET}},
	KeyMapping{appKeys.incStateSlot, {Input::Keycode::LCTRL, Input::Keycode::RIGHT_BRACKET}},
	KeyMapping{appKeys.fastForward, {Input::Keycode::LCTRL, Input::Keycode::GRAVE}},
	KeyMapping{appKeys.rewind, {Input::Keycode::LCTRL, Input::Keycode::R}},
	KeyMapping{appKeys.openMenu, Input::Keycode::BACK_KEY},
};

inline constexpr std::array rightUIKeys{appKeys.openMenu};
inline constexpr std::array leftUIKeys{appKeys.toggleFastForward, appKeys.rewind};

inline constexpr InputComponentDesc rightUIComponents{"Open Menu", rightUIKeys, InputComponent::ui, RT2DO};
inline constexpr InputComponentDesc leftUIComponents{"Fast-forward/Rewind", leftUIKeys, InputComponent::ui, LT2DO};

// Extra button groups from the custom build
inline constexpr std::array speedButtonKeys{appKeys.fastForwardSpeed2, appKeys.fastForwardSpeed3, appKeys.fastForward};
inline constexpr std::array speedToggleButtonKeys{appKeys.toggleFastForwardSpeed2, appKeys.toggleFastForwardSpeed3, appKeys.toggleFastForward};
inline constexpr std::array macroButtonKeys{appKeys.playMacro1, appKeys.playMacro2, appKeys.playMacro3, appKeys.playMacro4};
inline constexpr std::array captureButtonKeys{appKeys.takeScreenshot, appKeys.saveInstantReplay, appKeys.toggleVideoRecording};
inline constexpr std::array rollingSaveButtonKeys{appKeys.saveRollingState, appKeys.loadRollingState};

inline constexpr std::array extraUIComponents
{
	InputComponentDesc{"Fast-forward Speeds (Hold)", speedButtonKeys, InputComponent::ui, LT2DO},
	InputComponentDesc{"Fast-forward Speeds (Toggle)", speedToggleButtonKeys, InputComponent::ui, LT2DO},
	InputComponentDesc{"Screenshot/Instant Replay/Record", captureButtonKeys, InputComponent::ui, RT2DO},
	InputComponentDesc{"Rolling Save/Load", rollingSaveButtonKeys, InputComponent::ui, RT2DO},
};

inline constexpr InputComponentDesc macroButtonComponents{"Macro Buttons 1-4", macroButtonKeys, InputComponent::button, RB2DO, {.rowSize = 2}};
inline constexpr InputComponentDesc speedGamepadComponents{"Fast-forward Speed Buttons", speedButtonKeys, InputComponent::button, LB2DO, {.rowSize = 3}};

std::string_view toString(AppKeyCode);

inline constexpr std::array playerNumStrings{"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12"};

}

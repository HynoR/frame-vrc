// frame-vrc panel colors: frameclash-panel's graphite with a cyan accent. Only the helpers come from frameeyeosc's
// theme.h; its colors are not used by the frame-vrc screens.
// Copyright (c) 2026 FrameClash contributors. MIT License.
#pragma once

#include "third_party_frameeyeosc/theme.h"

constexpr Color cBg = hexColor(0x141414);       ///< page
constexpr Color cSide = hexColor(0x1A1A1A);     ///< sidebar
constexpr Color cCard = hexColor(0x1E1E1E);
constexpr Color cLine = hexColor(0x2C2C2C);     ///< card outlines, dividers
constexpr Color cCtl = hexColor(0x2A2A2A);      ///< buttons, rows
constexpr Color cCtlHot = hexColor(0x363636);   ///< ...under the pointer
constexpr Color cText = hexColor(0xECECEC);
constexpr Color cMuted = hexColor(0x8A8A8A);    ///< labels, hints (4.5:1+ on cards)
constexpr Color cDim = hexColor(0x5E5E5E);      ///< disabled
constexpr Color cAccent = hexColor(0x34C6F4);   ///< selection, the main action
constexpr Color cOnAccent = hexColor(0x141414);
constexpr Color cAccentTint = blendColor(cAccent, cCard, 0.14);
constexpr Color cGreen = hexColor(0x3DDC84);    ///< ok
constexpr Color cAmber = hexColor(0xFFB020);    ///< warnings
constexpr Color cRed = hexColor(0xFF5C5C);      ///< failures, destructive actions
constexpr Color cRedTint = blendColor(cRed, cCard, 0.14);

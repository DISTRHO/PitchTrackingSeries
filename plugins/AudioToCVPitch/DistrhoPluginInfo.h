/*
 * DISTRHO PitchTracking Series
 * Copyright (C) 2021-2022 Bram Giesen
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#pragma once

#define DISTRHO_PLUGIN_BRAND "DISTRHO"
#define DISTRHO_PLUGIN_NAME  "Audio to CV Pitch"
#define DISTRHO_PLUGIN_URI   "https://distrho.kx.studio/plugins/pitchtracking#cv"

#define DISTRHO_PLUGIN_HAS_UI           0
#define DISTRHO_PLUGIN_IS_RT_SAFE       1
#define DISTRHO_PLUGIN_NUM_INPUTS       1
#define DISTRHO_PLUGIN_NUM_OUTPUTS      2
#define DISTRHO_PLUGIN_WANT_LATENCY     1
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  0
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0
#define DISTRHO_PLUGIN_WANT_PROGRAMS    0
#define DISTRHO_PLUGIN_WANT_TIMEPOS     0

#ifdef __MOD_DEVICES__
#define DISTRHO_PLUGIN_LV2_CATEGORY "mod:ControlVoltagePlugin"
#define DISTRHO_PLUGIN_USES_MODGUI 1
#else
#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:UtilityPlugin"
#endif

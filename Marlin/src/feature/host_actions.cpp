/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../inc/MarlinConfig.h"

#if ENABLED(HOST_ACTION_COMMANDS)

//#define DEBUG_HOST_ACTIONS

#include "host_actions.h"

#include "pause.h"

#if ENABLED(ADVANCED_PAUSE_FEATURE)
  #include "../gcode/queue.h"
#endif

#if HAS_FILAMENT_SENSOR
  #include "runout.h"
#endif

HostUI hostui;

void HostUI::action(FSTR_P const fstr, const bool eol) {
  PORT_REDIRECT(SerialMask::All);
  SERIAL_ECHOPGM("//action:", fstr);
  if (eol) SERIAL_EOL();
}

#ifdef ACTION_ON_KILL
  void HostUI::kill() { action(F(ACTION_ON_KILL)); }
#endif
#ifdef ACTION_ON_PAUSE
  void HostUI::pause(const bool eol/*=true*/) { action(F(ACTION_ON_PAUSE), eol); }
#endif
#ifdef ACTION_ON_PAUSED
  void HostUI::paused(const bool eol/*=true*/) { action(F(ACTION_ON_PAUSED), eol); }
#endif
#ifdef ACTION_ON_RESUME
  void HostUI::resume() { action(F(ACTION_ON_RESUME)); }
#endif
#ifdef ACTION_ON_RESUMED
  void HostUI::resumed() { action(F(ACTION_ON_RESUMED)); }
#endif
#ifdef ACTION_ON_CANCEL
  void HostUI::cancel() { action(F(ACTION_ON_CANCEL)); }
#endif
#ifdef ACTION_ON_START
  void HostUI::start() { action(F(ACTION_ON_START)); }
#endif

#if ENABLED(G29_RETRY_AND_RECOVER)
  #ifdef ACTION_ON_G29_RECOVER
    void HostUI::g29_recover() { action(F(ACTION_ON_G29_RECOVER)); }
  #endif
  #ifdef ACTION_ON_G29_FAILURE
    void HostUI::g29_failure() { action(F(ACTION_ON_G29_FAILURE)); }
  #endif
#endif

#ifdef SHUTDOWN_ACTION
  void HostUI::shutdown() { action(F(SHUTDOWN_ACTION)); }
#endif

#if ENABLED(HOST_PROMPT_SUPPORT)

  PromptReason HostUI::host_prompt_reason = PROMPT_NOT_DEFINED;

  PGMSTR(CONTINUE_STR, "Continue");
  PGMSTR(DISMISS_STR, "Dismiss");

  #if HAS_RESUME_CONTINUE
    extern bool wait_for_user;
  #endif

  void HostUI::notify(const char * const cstr) {
    PORT_REDIRECT(SerialMask::All);
    action(F("notification "), false);
    SERIAL_ECHOLN(cstr);
  }

  void HostUI::notify_P(PGM_P const pstr) {
    PORT_REDIRECT(SerialMask::All);
    action(F("notification "), false);
    SERIAL_ECHOLNPGM_P(pstr);
  }

  void HostUI::prompt(FSTR_P const ptype, const bool eol/*=true*/) {
    PORT_REDIRECT(SerialMask::All);
    action(F("prompt_"), false);
    SERIAL_ECHO(ptype);
    if (eol) SERIAL_EOL();
  }

  void HostUI::prompt_plus(const bool pgm, FSTR_P const ptype, const char * const str, const char extra_char/*='\0'*/) {
    prompt(ptype, false);
    PORT_REDIRECT(SerialMask::All);
    SERIAL_CHAR(' ');
    if (pgm)
      SERIAL_ECHOPGM_P(str);
    else
      SERIAL_ECHO(str);
    if (extra_char != '\0') SERIAL_CHAR(extra_char);
    SERIAL_EOL();
  }

  void HostUI::prompt_begin(const PromptReason reason, FSTR_P const fstr, const char extra_char/*='\0'*/) {
    prompt_end();
    host_prompt_reason = reason;
    prompt_plus(F("begin"), fstr, extra_char);
  }
  void HostUI::prompt_begin(const PromptReason reason, const char * const cstr, const char extra_char/*='\0'*/) {
    prompt_end();
    host_prompt_reason = reason;
    prompt_plus(F("begin"), cstr, extra_char);
  }

  void HostUI::prompt_end() { prompt(F("end")); }
  void HostUI::prompt_show() { prompt(F("show")); }

  void HostUI::_prompt_show(FSTR_P const btn1, FSTR_P const btn2) {
    if (btn1) prompt_button(btn1);
    if (btn2) prompt_button(btn2);
    prompt_show();
  }

  void HostUI::prompt_button(FSTR_P const fstr) { prompt_plus(F("button"), fstr); }
  void HostUI::prompt_button(const char * const cstr) { prompt_plus(F("button"), cstr); }

  void HostUI::prompt_do(const PromptReason reason, FSTR_P const fstr, FSTR_P const btn1/*=nullptr*/, FSTR_P const btn2/*=nullptr*/) {
    prompt_begin(reason, fstr);
    _prompt_show(btn1, btn2);
  }
  void HostUI::prompt_do(const PromptReason reason, const char * const cstr, FSTR_P const btn1/*=nullptr*/, FSTR_P const btn2/*=nullptr*/) {
    prompt_begin(reason, cstr);
    _prompt_show(btn1, btn2);
  }
  void HostUI::prompt_do(const PromptReason reason, FSTR_P const fstr, const char extra_char, FSTR_P const btn1/*=nullptr*/, FSTR_P const btn2/*=nullptr*/) {
    prompt_begin(reason, fstr, extra_char);
    _prompt_show(btn1, btn2);
  }
  void HostUI::prompt_do(const PromptReason reason, const char * const cstr, const char extra_char, FSTR_P const btn1/*=nullptr*/, FSTR_P const btn2/*=nullptr*/) {
    prompt_begin(reason, cstr, extra_char);
    _prompt_show(btn1, btn2);
  }

  #if ENABLED(ADVANCED_PAUSE_FEATURE)
    void HostUI::filament_load_prompt() {
      const bool disable_to_continue = TERN0(HAS_FILAMENT_SENSOR, runout.filament_ran_out);
      prompt_do(PROMPT_FILAMENT_RUNOUT, F("Paused"), F("PurgeMore"),
        disable_to_continue ? F("DisableRunout") : FPSTR(CONTINUE_STR)
      );
    }
  #endif

  void HostUI::pause_prompt(const PauseMessage message) {
    FSTR_P fstr;
    switch (message) {
      case PAUSE_MESSAGE_PARKING:  fstr = GET_TEXT_F(MSG_HOST_PAUSE_PRINT_PARKING); break;
      case PAUSE_MESSAGE_CHANGING: fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_INIT); break;
      case PAUSE_MESSAGE_UNLOAD:   fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_UNLOAD); break;
      case PAUSE_MESSAGE_WAITING:  fstr = GET_TEXT_F(MSG_HOST_ADVANCED_PAUSE_WAITING); break;
      case PAUSE_MESSAGE_INSERT:   fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_INSERT); break;
      case PAUSE_MESSAGE_LOAD:     fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_LOAD); break;
      case PAUSE_MESSAGE_PURGE:
        fstr = GET_TEXT_F(TERN(ADVANCED_PAUSE_CONTINUOUS_PURGE, MSG_HOST_FILAMENT_CHANGE_CONT_PURGE, MSG_HOST_FILAMENT_CHANGE_PURGE));
        break;
      case PAUSE_MESSAGE_RESUME:   fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_RESUME); break;
      case PAUSE_MESSAGE_HEAT:     fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_HEAT); break;
      case PAUSE_MESSAGE_HEATING:  fstr = GET_TEXT_F(MSG_HOST_FILAMENT_CHANGE_HEATING); break;
      case PAUSE_MESSAGE_OPTION:   fstr = GET_TEXT_F(MSG_FILAMENT_CHANGE_OPTION_HEADER); break;
      #if ENABLED(MANUAL_SWITCHING_TOOLHEAD)
        case PAUSE_MESSAGE_TOOL_CHANGE:   fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE); break;
        case PAUSE_MESSAGE_TOOL_CHANGE_0: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_0); break;
        OPTCODE(HAS_TOOL_1, case PAUSE_MESSAGE_TOOL_CHANGE_1: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_1); break)
        OPTCODE(HAS_TOOL_2, case PAUSE_MESSAGE_TOOL_CHANGE_2: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_2); break)
        OPTCODE(HAS_TOOL_3, case PAUSE_MESSAGE_TOOL_CHANGE_3: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_3); break)
        OPTCODE(HAS_TOOL_4, case PAUSE_MESSAGE_TOOL_CHANGE_4: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_4); break)
        OPTCODE(HAS_TOOL_5, case PAUSE_MESSAGE_TOOL_CHANGE_5: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_5); break)
        OPTCODE(HAS_TOOL_6, case PAUSE_MESSAGE_TOOL_CHANGE_6: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_6); break)
        OPTCODE(HAS_TOOL_7, case PAUSE_MESSAGE_TOOL_CHANGE_7: fstr = GET_TEXT_F(MSG_HOST_PAUSE_TOOL_CHANGE_7); break)
      #endif
      case PAUSE_MESSAGE_STATUS:
      default: return;
    }
    hostui.prompt_do(PROMPT_USER_CONTINUE, fstr, FPSTR(CONTINUE_STR));
  }

  //
  // Handle responses from the host, such as:
  //  - Filament runout responses: Purge More, Continue
  //  - General "Continue" response
  //  - Resume Print response
  //  - Dismissal of info
  //
  void HostUI::handle_response(const uint8_t response) {
    const PromptReason hpr = host_prompt_reason;
    host_prompt_reason = PROMPT_NOT_DEFINED;  // Reset now ahead of logic
    switch (hpr) {
      case PROMPT_FILAMENT_RUNOUT:
        switch (response) {

          case 0: // "Purge More" button
            #if ENABLED(M600_PURGE_MORE_RESUMABLE)
              pause_menu_response = PAUSE_RESPONSE_EXTRUDE_MORE;  // Simulate menu selection (menu exits, doesn't extrude more)
            #endif
            break;

          case 1: // "Continue" / "Disable Runout" button
            #if ENABLED(M600_PURGE_MORE_RESUMABLE)
              pause_menu_response = PAUSE_RESPONSE_RESUME_PRINT;  // Simulate menu selection
            #endif
            #if HAS_FILAMENT_SENSOR
              if (runout.filament_ran_out) {                      // Disable a triggered sensor
                runout.enabled = false;
                runout.reset();
              }
            #endif
            break;
        }
        break;
      case PROMPT_USER_CONTINUE:
        TERN_(HAS_RESUME_CONTINUE, wait_for_user = false);
        break;
      case PROMPT_PAUSE_RESUME:
        #if ALL(ADVANCED_PAUSE_FEATURE, HAS_MEDIA)
          extern const char M24_STR[];
          queue.inject_P(M24_STR);
        #endif
        break;
      case PROMPT_INFO:
        break;
      default: break;
    }
  }

#endif // HOST_PROMPT_SUPPORT

#endif // HOST_ACTION_COMMANDS

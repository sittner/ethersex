/*
 *
 * Copyright(c) 2012 by Sascha Ittner <sascha.ittner@modusoft.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

#include "autoconf.h"
#include "config.h"
#include "core/debug.h"
#include "core/eeprom.h"
#include "services/heatctl/heatctl.h"

#include "protocols/ecmd/ecmd-base.h"

const char mode_str_manu[] PROGMEM = "manu";
const char mode_str_auto[] PROGMEM = "auto";
const char mode_str_hotw[] PROGMEM = "hotw";
const char mode_str_radi[] PROGMEM = "radi";
const char mode_str_serv[] PROGMEM = "serv";

const char *mode_str_tab[] PROGMEM = {
  mode_str_manu,
  mode_str_auto,
  mode_str_hotw,
  mode_str_radi,
  mode_str_serv,
  NULL
};

int16_t
parse_cmd_heatctl_param_show(char *cmd, char *output, uint16_t len)
{
  /* trick: use bytes on cmd as "connection specific static variables" */
  if (cmd[0] != ECMD_STATE_MAGIC) /* indicator flag: real invocation:  0 */
  {
    cmd[0] = ECMD_STATE_MAGIC;    /* continuing call: 23 */
    cmd[1] = 0;                   /* counter for sensors in list */
  }

  uint8_t i = cmd[1];
  cmd[1] = i + 1;

  switch (i)
  {
    case 0:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("manu_temp_hotwater: %d"),
                                   heatctl_eeprom.params.manu_temp_hotwater));
    case 1:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("manu_temp_radiator: %d"),
                                   heatctl_eeprom.params.manu_temp_radiator));
    case 2:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hyst_boiler: %d"),
                                   heatctl_eeprom.params.hyst_boiler));
    case 3:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hyst_hotwater: %d"),
                                   heatctl_eeprom.params.hyst_hotwater));
    case 4:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hyst_radiator: %d"),
                                   heatctl_eeprom.params.hyst_radiator));
    case 5:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("min_diff_hotwater: %d"),
                                   heatctl_eeprom.params.min_diff_hotwater));
    case 6:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater_offset: %d"),
                                   heatctl_eeprom.params.hotwater_offset));
    case 7:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("radiator_offset: %d"),
                                   heatctl_eeprom.params.radiator_offset));
#ifdef HEATCTL_CIRCPUMP_SUPPORT
    case 8:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("circpump_time: %d"),
                                   heatctl_eeprom.params.circpump_time));
#endif
    default:
      return ECMD_FINAL_OK;
  }
}

int16_t
parse_cmd_heatctl_param_set(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
    cmd++;

  char *valstr = strchr(cmd, ' ');
  if (valstr == NULL)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  *(valstr++) = 0;
  while (*valstr == ' ')
    valstr++;

  if (strcmp_P(cmd, PSTR("manu_temp_hotwater")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.manu_temp_hotwater = val;
  }
  else if (strcmp_P(cmd, PSTR("manu_temp_radiator")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.manu_temp_radiator = val;
  }
  else if (strcmp_P(cmd, PSTR("hyst_boiler")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.hyst_boiler = val;
  }
  else if (strcmp_P(cmd, PSTR("hyst_hotwater")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.hyst_hotwater = val;
  }
  else if (strcmp_P(cmd, PSTR("hyst_radiator")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.hyst_radiator = val;
  }
  else if (strcmp_P(cmd, PSTR("min_diff_hotwater")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.min_diff_hotwater = val;
  }
  else if (strcmp_P(cmd, PSTR("hotwater_offset")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < -1 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.hotwater_offset = val;
  }
  else if (strcmp_P(cmd, PSTR("radiator_offset")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_BOILER_MAX_TEMP)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.radiator_offset = val;
  }
#ifdef HEATCTL_CIRCPUMP_SUPPORT
  else if (strcmp_P(cmd, PSTR("circpump_time")) == 0)
  {
    int16_t val = atoi(valstr);
    if (val < 0 || val > HEATCTL_MAX_CIRCPUMP_TIME)
    {
      return ECMD_ERR_PARSE_ERROR;
    }
    heatctl_eeprom.params.circpump_time = val;
  }
#endif
  else
  {
    return ECMD_ERR_PARSE_ERROR;
  }

  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_param_save(char *cmd, char *output, uint16_t len)
{
  heatctl_save_params();
  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_mode(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
    cmd++;

  if (*cmd == 0)
  {
    return ECMD_FINAL(snprintf_P(output, len, PSTR("%S"),
                                 (const char *)
                                 pgm_read_word(&mode_str_tab
                                               [heatctl_eeprom.mode])));
  }

  uint8_t i;
  const char *s;
  for (i = 0; (s = (const char *) pgm_read_word(&mode_str_tab[i])); i++)
  {
    if (strcmp_P(cmd, s) == 0)
    {
      heatctl_set_mode(i);
      return ECMD_FINAL_OK;
    }
  }

  return ECMD_ERR_PARSE_ERROR;
}

int16_t
parse_cmd_heatctl_radi_show(char *cmd, char *output, uint16_t len)
{
  /* trick: use bytes on cmd as "connection specific static variables" */
  if (cmd[0] != ECMD_STATE_MAGIC) /* indicator flag: real invocation:  0 */
  {
    cmd[0] = ECMD_STATE_MAGIC;    /* continuing call: 23 */
    cmd[1] = 0;                   /* counter for sensors in list */
  }

  uint8_t i = cmd[1];
  cmd[1] = i + 1;

  if (i >= HEATCTL_RADIATOR_ITEM_COUNT)
  {
    return ECMD_FINAL_OK;
  }

  return ECMD_AGAIN(snprintf_P(output, len, PSTR("%d\t%d\t%d"), i,
                               heatctl_eeprom.radiator_items[i].threshold,
                               heatctl_eeprom.radiator_items[i].slew_rate));
}

int16_t
parse_cmd_heatctl_radi_set(char *cmd, char *output, uint16_t len)
{
  char *s;

  // get index
  while (*cmd == ' ')
    cmd++;
  s = strchr(cmd, ' ');
  if (s == NULL)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  *(s++) = 0;
  uint8_t idx = atoi(cmd);
  cmd = s;
  if (idx >= HEATCTL_RADIATOR_ITEM_COUNT)
  {
    return ECMD_ERR_PARSE_ERROR;
  }

  // get treshhold
  while (*cmd == ' ')
    cmd++;
  s = strchr(cmd, ' ');
  if (s == NULL)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  *(s++) = 0;
  int16_t threshold = atoi(cmd);
  cmd = s;
  if (threshold < -HEATCTL_BOILER_MAX_TEMP ||
      threshold > HEATCTL_BOILER_MAX_TEMP)
  {
    return ECMD_ERR_PARSE_ERROR;
  }

  // get slew rate
  int16_t slew_rate = atoi(cmd);
  if (slew_rate < 0 || slew_rate > HEATCTL_MAX_SLEW_RATE)
  {
    return ECMD_ERR_PARSE_ERROR;
  }

  // set values
  heatctl_eeprom.radiator_items[idx].threshold = threshold;
  heatctl_eeprom.radiator_items[idx].slew_rate = slew_rate;

  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_radi_save(char *cmd, char *output, uint16_t len)
{
  heatctl_save_radiator_items();
  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_radi_sel(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
    cmd++;

  // show index
  if (*cmd == 0)
  {
    return
      ECMD_FINAL(snprintf_P(output, len, PSTR("%d"), heatctl_radiator_index));
  }

  // set index
  int8_t idx = atoi(cmd);
  if (idx < -1 || idx >= HEATCTL_HOTWATER_ITEM_COUNT)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  heatctl_radiator_index = idx;

  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_hotw_show(char *cmd, char *output, uint16_t len)
{
  /* trick: use bytes on cmd as "connection specific static variables" */
  if (cmd[0] != ECMD_STATE_MAGIC) /* indicator flag: real invocation:  0 */
  {
    cmd[0] = ECMD_STATE_MAGIC;    /* continuing call: 23 */
    cmd[1] = 0;                   /* counter for sensors in list */
  }

  uint8_t i = cmd[1];
  cmd[1] = i + 1;

  if (i >= HEATCTL_HOTWATER_ITEM_COUNT)
  {
    return ECMD_FINAL_OK;
  }

  return ECMD_AGAIN(snprintf_P(output, len, PSTR("%d\t%d"), i,
                               heatctl_eeprom.hotwater_items[i].temp));
}

int16_t
parse_cmd_heatctl_hotw_set(char *cmd, char *output, uint16_t len)
{
  char *s;

  // get index
  while (*cmd == ' ')
    cmd++;
  s = strchr(cmd, ' ');
  if (s == NULL)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  *(s++) = 0;
  uint8_t idx = atoi(cmd);
  cmd = s;
  if (idx >= HEATCTL_HOTWATER_ITEM_COUNT)
  {
    return ECMD_ERR_PARSE_ERROR;
  }

  // get temp
  int16_t temp = atoi(cmd);
  if (temp < 0 || temp > HEATCTL_BOILER_MAX_TEMP)
  {
    return ECMD_ERR_PARSE_ERROR;
  }

  // set values
  heatctl_eeprom.hotwater_items[idx].temp = temp;

  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_hotw_save(char *cmd, char *output, uint16_t len)
{
  heatctl_save_hotwater_items();
  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_hotw_sel(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
    cmd++;

  // show index
  if (*cmd == 0)
  {
    return
      ECMD_FINAL(snprintf_P(output, len, PSTR("%d"), heatctl_hotwater_index));
  }

  // set index
  int8_t idx = atoi(cmd);
  if (idx < -1 || idx >= HEATCTL_HOTWATER_ITEM_COUNT)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  heatctl_hotwater_index = idx;

  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_hotw_req(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
    cmd++;

  // show index
  if (*cmd == 0)
  {
    return
      ECMD_FINAL(snprintf_P(output, len, PSTR("%d"), heatctl_hotwater_req));
  }

  // set index
  int8_t idx = atoi(cmd);
  if (idx < -1 || idx >= HEATCTL_HOTWATER_ITEM_COUNT)
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  heatctl_hotwater_req = idx;

  return ECMD_FINAL_OK;
}

int16_t
parse_cmd_heatctl_state(char *cmd, char *output, uint16_t len)
{
  ldiv_t dv_min;
  ldiv_t dv_hr;

  /* trick: use bytes on cmd as "connection specific static variables" */
  if (cmd[0] != ECMD_STATE_MAGIC) /* indicator flag: real invocation:  0 */
  {
    cmd[0] = ECMD_STATE_MAGIC;    /* continuing call: 23 */
    cmd[1] = 0;                   /* counter for sensors in list */
  }

  uint8_t i = cmd[1];
  cmd[1] = i + 1;

  switch (i)
  {
    case 0:
      return ECMD_AGAIN(snprintf_P(output, len, PSTR("mode: %S"),
                                   (const char *)
                                   pgm_read_word(&mode_str_tab
                                                 [heatctl_eeprom.mode])));
    case 1:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("boiler temp: %d"),
                                   heatctl_boiler_temp));
    case 2:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("boiler sp: %d"),
                                   heatctl_boiler_setpoint));
    case 3:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("burner state: %d"),
                                   heatctl_burner_on));
    case 4:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("outdoor temp: %d"),
                                   heatctl_outdoor_temp));
    case 5:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("radiator idx: %d"),
                                   heatctl_radiator_index));
    case 6:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("radiator sp: %d"),
                                   heatctl_radiator_setpoint));
    case 7:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("radiator state: %d"),
                                   heatctl_radiator_on));
    case 8:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater temp: %d"),
                                   heatctl_hotwater_temp));
    case 9:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater idx: %d"),
                                   heatctl_hotwater_index));
    case 10:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater req: %d"),
                                   heatctl_hotwater_req));
    case 11:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater sp: %d"),
                                   heatctl_hotwater_setpoint));
    case 12:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater state: %d"),
                                   heatctl_hotwater_on));
    case 13:
      dv_min = ldiv(clock_get_uptime(), 60);
      dv_hr = ldiv(dv_min.quot, 60);
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("ctrl on time: %lu:%02lu:%02lu"),
                                   dv_hr.quot, dv_hr.rem, dv_min.rem));
    case 14:
      dv_min = ldiv(heatctl_radiator_on_time, 60);
      dv_hr = ldiv(dv_min.quot, 60);
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("radiator on time: %lu:%02lu:%02lu"),
                                   dv_hr.quot, dv_hr.rem, dv_min.rem));
    case 15:
      dv_min = ldiv(heatctl_hotwater_on_time, 60);
      dv_hr = ldiv(dv_min.quot, 60);
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("hotwater on time: %lu:%02lu:%02lu"),
                                   dv_hr.quot, dv_hr.rem, dv_min.rem));
    case 16:
      dv_min = ldiv(heatctl_burner_on_time, 60);
      dv_hr = ldiv(dv_min.quot, 60);
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("burner on time: %lu:%02lu:%02lu"),
                                   dv_hr.quot, dv_hr.rem, dv_min.rem));
#ifdef HEATCTL_CIRCPUMP_SUPPORT
    case 17:
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("circpump state: %d"),
                                   heatctl_circpump_on));
    case 18:
      dv_min = ldiv(heatctl_circpump_on_time, 60);
      dv_hr = ldiv(dv_min.quot, 60);
      return ECMD_AGAIN(snprintf_P(output, len,
                                   PSTR("circpump on time: %lu:%02lu:%02lu"),
                                   dv_hr.quot, dv_hr.rem, dv_min.rem));
#endif
    default:
      return ECMD_FINAL_OK;
  }
}

#ifdef HEATCTL_CIRCPUMP_SUPPORT
int16_t
parse_cmd_heatctl_circ(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
    cmd++;

  if (strcmp_P(cmd, PSTR("time")) == 0)
  {
    heatctl_circpump_timer = heatctl_eeprom.params.circpump_time;
  }
  else if (strcmp_P(cmd, PSTR("on")) == 0)
  {
    heatctl_circpump_cmd = 1;
  }
  else if (strcmp_P(cmd, PSTR("off")) == 0)
  {
    heatctl_circpump_cmd = 0;
  }
  else
  {
    return ECMD_ERR_PARSE_ERROR;
  }
  return ECMD_FINAL_OK;
}
#endif

/*
  -- Ethersex META --
  ecmd_feature(heatctl_param_show, "hc param show",, show current heater control parameter settings.)
  ecmd_feature(heatctl_param_set, "hc param set", NAME VALUE, set heater control parameter.)
  ecmd_feature(heatctl_param_save, "hc param save",, write parameters to EEPROM.)
  ecmd_feature(heatctl_mode, "hc mode", [MODE], show/change mode.)
  ecmd_feature(heatctl_radi_show, "hc radi show",, show radiator value table.)
  ecmd_feature(heatctl_radi_set, "hc radi set", INDEX THOLD SRATE, set radiator value entry.)
  ecmd_feature(heatctl_radi_save, "hc radi save",, save radiator value table.)
  ecmd_feature(heatctl_radi_sel, "hc radi", [INDEX], select radiator table entry.)
  ecmd_feature(heatctl_hotw_show, "hc hotw show",, show hot water value table.)
  ecmd_feature(heatctl_hotw_set, "hc hotw set", INDEX TEMP, set hot water value entry.)
  ecmd_feature(heatctl_hotw_save, "hc hotw save",, save hot water value table.)
  ecmd_feature(heatctl_hotw_req, "hc hotw req", [INDEX], request hot water preparation.)
  ecmd_feature(heatctl_hotw_sel, "hc hotw", [INDEX], select hot water table entry.)
  ecmd_feature(heatctl_state, "hc state",, show current state.)
  ecmd_ifdef(HEATCTL_CIRCPUMP_SUPPORT)
    ecmd_feature(heatctl_circ, "hc circ", STATE, set circulation pump state)
  ecmd_endif()
*/

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
#include "services/clock/clock.h"
#include "services/heatctl/heatctl.h"
#include "hardware/onewire/onewire.h"

#ifdef TANKLEVEL_SUPPORT
#include "services/tanklevel/tanklevel.h"
#endif

heatctl_eeprom_t heatctl_eeprom;

int8_t heatctl_radiator_index;
int8_t heatctl_hotwater_index;
int8_t heatctl_hotwater_req = -1;

int16_t heatctl_boiler_temp;
int16_t heatctl_hotwater_temp;
int16_t heatctl_outdoor_temp;

int16_t heatctl_radiator_setpoint;
int16_t heatctl_hotwater_setpoint;
int16_t heatctl_boiler_setpoint;

uint8_t heatctl_radiator_on;
uint8_t heatctl_hotwater_on;
uint8_t heatctl_burner_on;

uint8_t heatctl_startup_delay = HEATCTL_STARTUP_DELAY;

timestamp_t heatctl_radiator_on_time;
timestamp_t heatctl_hotwater_on_time;
timestamp_t heatctl_burner_on_time;

timestamp_t heatctl_burner_min_on_timer;
timestamp_t heatctl_burner_min_off_timer;

timestamp_t heatctl_last_time;

#ifdef HEATCTL_CIRCPUMP_SUPPORT
uint8_t heatctl_circpump_on;
uint8_t heatctl_circpump_cmd;
timestamp_t heatctl_circpump_on_time;
uint16_t heatctl_circpump_timer;
#endif

#ifdef HEATCTL_RADIATOR_ACTIVE_LOW
#define HEATCTL_RADIATOR_ON  PIN_CLEAR(HEATCTL_RADIATOR)
#define HEATCTL_RADIATOR_OFF PIN_SET(HEATCTL_RADIATOR)
#else
#define HEATCTL_RADIATOR_ON  PIN_SET(HEATCTL_RADIATOR)
#define HEATCTL_RADIATOR_OFF PIN_CLEAR(HEATCTL_RADIATOR)
#endif

#ifdef HEATCTL_HOTWATER_ACTIVE_LOW
#define HEATCTL_HOTWATER_ON  PIN_CLEAR(HEATCTL_HOTWATER)
#define HEATCTL_HOTWATER_OFF PIN_SET(HEATCTL_HOTWATER)
#else
#define HEATCTL_HOTWATER_ON  PIN_SET(HEATCTL_HOTWATER)
#define HEATCTL_HOTWATER_OFF PIN_CLEAR(HEATCTL_HOTWATER)
#endif

#ifdef HEATCTL_BURNER_ACTIVE_LOW
#define HEATCTL_BURNER_ON  PIN_CLEAR(HEATCTL_BURNER)
#define HEATCTL_BURNER_OFF PIN_SET(HEATCTL_BURNER)
#else
#define HEATCTL_BURNER_ON  PIN_SET(HEATCTL_BURNER)
#define HEATCTL_BURNER_OFF PIN_CLEAR(HEATCTL_BURNER)
#endif

#ifdef HEATCTL_CIRCPUMP_ACTIVE_LOW
#define HEATCTL_CIRCPUMP_ON  PIN_CLEAR(HEATCTL_CIRCPUMP)
#define HEATCTL_CIRCPUMP_OFF PIN_SET(HEATCTL_CIRCPUMP)
#else
#define HEATCTL_CIRCPUMP_ON  PIN_SET(HEATCTL_CIRCPUMP)
#define HEATCTL_CIRCPUMP_OFF PIN_CLEAR(HEATCTL_CIRCPUMP)
#endif

#ifdef HEATCTL_FAULT_ACTIVE_LOW
#define HEATCTL_FAULT_IS_ACTIVE PIN_LOW(HEATCTL_FAULT)
#else
#define HEATCTL_FAULT_IS_ACTIVE PIN_HIGH(HEATCTL_FAULT)
#endif

#ifdef HEATCTL_WHM_ACTIVE_LOW
#define HEATCTL_WHM_IS_ACTIVE PIN_LOW(HEATCTL_WHM)
#else
#define HEATCTL_WHM_IS_ACTIVE PIN_HIGH(HEATCTL_WHM)
#endif

void
heatctl_init(void)
{
  // restore eepom data
  eeprom_restore(heatctl_eeprom, &heatctl_eeprom, sizeof(heatctl_eeprom_t));

  // init pins
  HEATCTL_RADIATOR_OFF;
  HEATCTL_HOTWATER_OFF;
  HEATCTL_BURNER_OFF;
#ifdef HEATCTL_CIRCPUMP_SUPPORT
  HEATCTL_CIRCPUMP_OFF;
#endif
}

void
heatctl_periodic(void)
{
  int16_t err, err_hotwater;

  // save last states
  uint8_t burner_on_last = heatctl_burner_on;

  // get time delta
  timestamp_t ts = clock_get_uptime();
  timestamp_t time_delta = ts - heatctl_last_time;
  heatctl_last_time = ts;

  // read temperatures
  heatctl_boiler_temp = ow_sensors[HEATCTL_OW_IDX_BOILER].temp;
  heatctl_hotwater_temp = ow_sensors[HEATCTL_OW_IDX_HOTWATER].temp;
  heatctl_outdoor_temp = ow_sensors[HEATCTL_OW_IDX_OUTDOOR].temp;

  // get setpoints
  uint8_t service = 0;
  int16_t radiator_threshold = 0;
  int16_t radiator_slew_rate = -1;
  heatctl_radiator_setpoint = 0;
  heatctl_hotwater_setpoint = 0;
  switch (heatctl_eeprom.mode)
  {
    case HEATCTL_MODE_SERV:
      service = 1;
      break;
    case HEATCTL_MODE_AUTO:
      if (heatctl_radiator_index >= 0)
      {
        radiator_threshold =
          heatctl_eeprom.radiator_items[heatctl_radiator_index].threshold;
        radiator_slew_rate =
          heatctl_eeprom.radiator_items[heatctl_radiator_index].slew_rate;
      }
      if (heatctl_hotwater_index >= 0)
      {
        heatctl_hotwater_setpoint =
          heatctl_eeprom.hotwater_items[heatctl_hotwater_index].temp;
      }
      break;
    case HEATCTL_MODE_HOTW:
      if (heatctl_hotwater_index >= 0)
      {
        heatctl_hotwater_setpoint =
          heatctl_eeprom.hotwater_items[heatctl_hotwater_index].temp;
      }
      break;
    case HEATCTL_MODE_RADI:
      if (heatctl_radiator_index >= 0)
      {
        radiator_threshold =
          heatctl_eeprom.radiator_items[heatctl_radiator_index].threshold;
        radiator_slew_rate =
          heatctl_eeprom.radiator_items[heatctl_radiator_index].slew_rate;
      }
      break;
    default:
      heatctl_radiator_setpoint = heatctl_eeprom.params.manu_temp_radiator;
      heatctl_hotwater_setpoint = heatctl_eeprom.params.manu_temp_hotwater;
  }

  // calculate radiator curve
  if (radiator_slew_rate >= 0)
  {
    err = radiator_threshold - heatctl_outdoor_temp;
    if (err > heatctl_eeprom.params.hyst_radiator)
    {
      heatctl_radiator_on = 1;
    }
    if (err < -heatctl_eeprom.params.hyst_radiator)
    {
      heatctl_radiator_on = 0;
    }
    if (heatctl_radiator_on)
    {
      int32_t offset = (int32_t) err * (int32_t) radiator_slew_rate;
      heatctl_radiator_setpoint =
        radiator_threshold + (int16_t) (offset / 1000) +
        heatctl_eeprom.params.radiator_offset;
    }
  }
  else
  {
    heatctl_radiator_on = (heatctl_radiator_setpoint > 0);
  }

  // hot water preparation request
  if (heatctl_hotwater_req >= 0)
  {
    heatctl_hotwater_setpoint =
      heatctl_eeprom.hotwater_items[heatctl_hotwater_req].temp;
  }

  // handle hot water
  err_hotwater = heatctl_hotwater_setpoint - heatctl_hotwater_temp;
  if (err_hotwater > heatctl_eeprom.params.hyst_hotwater || heatctl_hotwater_req >= 0)
  {
    heatctl_hotwater_on = 1;
  }
  if (err_hotwater < -heatctl_eeprom.params.hyst_hotwater)
  {
    heatctl_hotwater_on = 0;
    heatctl_hotwater_req = -1;
  }

#ifdef HEATCTL_HOTWATER_PRIO
  // hot water priority
  if (heatctl_hotwater_on)
  {
    heatctl_radiator_on = 0;
  }
#endif

  // handle boiler
  if (heatctl_hotwater_on)
  {
    if (heatctl_eeprom.params.hotwater_offset >= 0)
    {
      heatctl_boiler_setpoint =
        heatctl_hotwater_setpoint + heatctl_eeprom.params.hotwater_offset;
    }
    else
    {
      heatctl_boiler_setpoint =
        HEATCTL_BOILER_MAX_TEMP - heatctl_eeprom.params.hyst_boiler;
    }
  }
  else
  {
    heatctl_boiler_setpoint = heatctl_radiator_setpoint;
  }
  err = heatctl_boiler_setpoint - heatctl_boiler_temp;
  if (err > heatctl_eeprom.params.hyst_boiler)
  {
    heatctl_burner_on = 1;
  }
  if (err < -heatctl_eeprom.params.hyst_boiler)
  {
    heatctl_burner_on = 0;
  }

  // check hot water min diff if burner turns on
  if (heatctl_hotwater_on && !burner_on_last && heatctl_burner_on)
  {
    err_hotwater -= heatctl_eeprom.params.min_diff_hotwater;
    if (err_hotwater < -heatctl_eeprom.params.hyst_hotwater)
    {
      heatctl_hotwater_on = 0;
      heatctl_hotwater_req = -1;
      heatctl_burner_on = 0;
    }
  }

#ifdef HEATCTL_CIRCPUMP_SUPPORT
  // circulation pump
#ifdef HEATCTL_HOTWATER_REQ_CIRCPUMP
  if (heatctl_hotwater_req >= 0)
  {
    heatctl_circpump_timer = heatctl_eeprom.params.circpump_time;
  }
#endif

  heatctl_circpump_on = 0;
  if (heatctl_circpump_timer > 0)
  {
    heatctl_circpump_timer--;
    heatctl_circpump_on = 1;
  }
  if (heatctl_circpump_cmd)
  {
    heatctl_circpump_on = 1;
  }
#endif

  // startup delay
  if (heatctl_startup_delay)
  {
    heatctl_startup_delay--;
    heatctl_radiator_on = 0;
    heatctl_hotwater_on = 0;
    heatctl_burner_on = 0;
#ifdef HEATCTL_CIRCPUMP_SUPPORT
    heatctl_circpump_on = 0;
#endif
  }

  // check minimum on/off times
  if (heatctl_burner_min_on_timer > 0)
    heatctl_burner_min_on_timer--;
  if (heatctl_burner_min_off_timer > 0)
    heatctl_burner_min_off_timer--;

  if (heatctl_burner_on)
  {
    if (heatctl_burner_min_off_timer > 0)
      heatctl_burner_on = 0;
  }
  else
  {
    if (heatctl_burner_min_on_timer > 0)
      heatctl_burner_on = 1;
  }

  if (heatctl_burner_on && !burner_on_last)
    heatctl_burner_min_on_timer = HEATCTL_BURNER_MIN_ON_TIME;
  if (!heatctl_burner_on && burner_on_last)
    heatctl_burner_min_off_timer = HEATCTL_BURNER_MIN_OFF_TIME;

  // boiler temp limit
  if (heatctl_boiler_temp > HEATCTL_BOILER_MAX_TEMP)
  {
    heatctl_burner_on = 0;
  }

  // service mode
  if (service)
  {
    heatctl_radiator_on = 1;
    heatctl_hotwater_on = 0;
    heatctl_burner_on = 1;
  }

#ifdef TANKLEVEL_SUPPORT
  // set tanklevel lock
  tanklevel_set_lock(heatctl_burner_on);
#endif

  // update outputs
  if (heatctl_radiator_on)
  {
    heatctl_radiator_on_time += time_delta;
    HEATCTL_RADIATOR_ON;
  }
  else
  {
    HEATCTL_RADIATOR_OFF;
  }
  if (heatctl_hotwater_on)
  {
    heatctl_hotwater_on_time += time_delta;
    HEATCTL_HOTWATER_ON;
  }
  else
  {
    HEATCTL_HOTWATER_OFF;
  }
  if (heatctl_burner_on)
  {
    heatctl_burner_on_time += time_delta;
    HEATCTL_BURNER_ON;
  }
  else
  {
    HEATCTL_BURNER_OFF;
  }
#ifdef HEATCTL_CIRCPUMP_SUPPORT
  if (heatctl_circpump_on)
  {
    heatctl_circpump_on_time += time_delta;
    HEATCTL_CIRCPUMP_ON;
  }
  else
  {
    HEATCTL_CIRCPUMP_OFF;
  }
#endif
}

void
heatctl_save_params(void)
{
  eeprom_save(heatctl_eeprom.params, &heatctl_eeprom.params,
              sizeof(heatctl_params_t));
  eeprom_update_chksum();
}

void
heatctl_set_mode(uint8_t mode)
{
  heatctl_eeprom.mode = mode;
  eeprom_save_char(heatctl_eeprom.mode, mode);
  eeprom_update_chksum();
}

void
heatctl_save_radiator_items(void)
{
  eeprom_save(heatctl_eeprom.radiator_items, heatctl_eeprom.radiator_items,
              HEATCTL_RADIATOR_ITEM_COUNT * sizeof(heatctl_radiator_item_t));
  eeprom_update_chksum();
}

void
heatctl_save_hotwater_items(void)
{
  eeprom_save(heatctl_eeprom.hotwater_items, heatctl_eeprom.hotwater_items,
              HEATCTL_HOTWATER_ITEM_COUNT * sizeof(heatctl_hotwater_item_t));
  eeprom_update_chksum();
}

/*
  -- Ethersex META --
  header(services/heatctl/heatctl.h)
  init(heatctl_init)
  timer(50, heatctl_periodic())
*/

/*
 *
 * Copyright (c) 2019 by Sascha Ittner <sascha.ittner@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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


#include <string.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "config.h"
#include "core/debug.h"
#include "core/eeprom.h"
#include "core/bit-macros.h"
#include "core/util/fixedpoint.h"
#include "protocols/mqtt/mqtt.h"
#include "services/heatctl/heatctl.h"
#include "services/heatctl/heatctl_mqtt.h"


#define HEATCTL_PUBLISH_NAME_FORMAT     HEATCTL_MQTT_TOPIC "/%S"
#define HEATCTL_MQTT_RETAIN             false
#define TOPIC_LENGTH                    (32)
#define VALUE_LENGTH                    (32)

static const char publish_name_topic_format[] PROGMEM = HEATCTL_PUBLISH_NAME_FORMAT;

static const char mode_str_manu[] PROGMEM = "manual";
static const char mode_str_auto[] PROGMEM = "automatic";
static const char mode_str_hotw[] PROGMEM = "hotwater";
static const char mode_str_radi[] PROGMEM = "radiator";
static const char mode_str_serv[] PROGMEM = "service";

static const char * const mode_str_tab[] PROGMEM = {
  mode_str_manu,
  mode_str_auto,
  mode_str_hotw,
  mode_str_radi,
  mode_str_serv,
  NULL
};

static int8_t heatctl_mqtt_state = -1;

static void send_str(const char *name, const char *value)
{
  char topic[TOPIC_LENGTH];

  snprintf_P(topic, TOPIC_LENGTH, publish_name_topic_format, name);
  mqtt_construct_publish_packet(topic, value, strlen(value), HEATCTL_MQTT_RETAIN);

  heatctl_mqtt_state++;
}

static void send_temp(const char *name, int16_t value)
{
  char buf[8];

  itoa_fixedpoint(value, 2, buf, sizeof(buf));
  send_str(name, buf);
}

static void send_bool(const char *name, uint8_t value)
{
  char buf[8];

  strncpy_P(buf, value ? PSTR("true") : PSTR("false"), sizeof(buf));
  send_str(name, buf);
}

static void send_idx(const char *name, int8_t value)
{
  char buf[8];

  itoa(value, buf, 10);
  send_str(name, buf);
}

static void send_mode(const char *name, uint8_t value)
{
  char buf[8];

  strncpy_P(buf, mode_str_tab[value], sizeof(buf));
  send_str(name, buf);
}

static void send_timestamp(const char *name, timestamp_t value)
{
  ldiv_t dv_min;
  ldiv_t dv_hr;
  char buf[12];

  dv_min = ldiv(value, 60);
  dv_hr = ldiv(dv_min.quot, 60);
  snprintf_P(buf, sizeof(buf),
    PSTR("ctrl on time: %lu:%02lu:%02lu"),
    dv_hr.quot, dv_hr.rem, dv_min.rem);
  send_str(name, buf);
}

void
heatctl_poll_cb(void)
{
  if (heatctl_mqtt_state < 0)
    return;

  switch (heatctl_mqtt_state)
  {
    case 0:
      send_mode(PSTR("mode"), heatctl_eeprom.mode);
      break;
    case 1:
      send_temp(PSTR("boiler_temp"), heatctl_boiler_temp);
      break;
    case 2:
      send_temp(PSTR("boiler_setpoint"), heatctl_boiler_setpoint);
      break;
    case 3:
      send_bool(PSTR("burner_on"), heatctl_burner_on);
      break;
    case 4:
      send_temp(PSTR("outdoor_temp"), heatctl_outdoor_temp);
      break;
    case 5:
      send_idx(PSTR("radiator_index"), heatctl_radiator_index);
      break;
    case 6:
      send_temp(PSTR("radiator_setpoint"), heatctl_radiator_setpoint);
      break;
    case 7:
      send_bool(PSTR("radiator_on"), heatctl_radiator_on);
      break;
    case 8:
      send_temp(PSTR("hotwater_temp"), heatctl_hotwater_temp);
      break;
    case 9:
      send_idx(PSTR("hotwater_index"), heatctl_hotwater_index);
      break;
    case 10:
      send_idx(PSTR("hotwater_req"), heatctl_hotwater_req);
      break;
    case 11:
      send_temp(PSTR("hotwater_setpoint"), heatctl_hotwater_setpoint);
      break;
    case 12:
      send_bool(PSTR("hotwater_on"), heatctl_hotwater_on);
      break;
    case 13:
      send_timestamp(PSTR("uptime"), clock_get_uptime());
      break;
    case 14:
      send_timestamp(PSTR("radiator_on_time"), heatctl_radiator_on_time);
      break;
    case 15:
      send_timestamp(PSTR("hotwater_on_time"), heatctl_hotwater_on_time);
      break;
    case 16:
      send_timestamp(PSTR("burner_on_time"), heatctl_burner_on_time);
      break;
#ifdef HEATCTL_CIRCPUMP_SUPPORT
    case 17:
      send_bool(PSTR("circpump_on"), heatctl_circpump_on);
      break;
    case 18:
      send_timestamp(PSTR("circpump_on_time"), heatctl_circpump_on_time);
      break;
#endif
    default:
      heatctl_mqtt_state = -1;
  }
}

static const mqtt_callback_config_t mqtt_callback_config PROGMEM = {
  .connack_callback = NULL,
  .poll_callback = heatctl_poll_cb,
  .close_callback = NULL,
  .publish_callback = NULL,
};

void
heatctl_mqtt_init()
{
  mqtt_register_callback(&mqtt_callback_config);
}

void
heatctl_mqtt_periodic(void)
{
  if (heatctl_mqtt_state < 0) {
    heatctl_mqtt_state = 0;
  }
}

/*
  -- Ethersex META --
  header(services/heatctl/heatctl_mqtt.h)
  net_init(heatctl_mqtt_init)
  timer(500, heatctl_mqtt_periodic())
*/


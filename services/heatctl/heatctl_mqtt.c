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


#define MQTT_RETAIN             false

#define MQTT_STATE_START         0
#define MQTT_STATE_IDLE         -1
#define MQTT_STATE_DISABLED     -2

static const char topic_mode[] PROGMEM = HEATCTL_MQTT_TOPIC "/mode";
static const char topic_boiler_temp[] PROGMEM = HEATCTL_MQTT_TOPIC "/boiler_temp";
static const char topic_boiler_setpoint[] PROGMEM = HEATCTL_MQTT_TOPIC "/boiler_setpoint";
static const char topic_burner_on[] PROGMEM = HEATCTL_MQTT_TOPIC "/burner_on";
static const char topic_outdoor_temp[] PROGMEM = HEATCTL_MQTT_TOPIC "/outdoor_temp";
static const char topic_radiator_index[] PROGMEM = HEATCTL_MQTT_TOPIC "/radiator_index";
static const char topic_radiator_setpoint[] PROGMEM = HEATCTL_MQTT_TOPIC "/radiator_setpoint";
static const char topic_radiator_on[] PROGMEM = HEATCTL_MQTT_TOPIC "/radiator_on";
static const char topic_hotwater_temp[] PROGMEM = HEATCTL_MQTT_TOPIC "/hotwater_temp";
static const char topic_hotwater_index[] PROGMEM = HEATCTL_MQTT_TOPIC "/hotwater_index";
static const char topic_hotwater_req[] PROGMEM = HEATCTL_MQTT_TOPIC "/hotwater_req";
static const char topic_hotwater_setpoint[] PROGMEM = HEATCTL_MQTT_TOPIC "/hotwater_setpoint";
static const char topic_hotwater_on[] PROGMEM = HEATCTL_MQTT_TOPIC "/hotwater_on";
static const char topic_uptime[] PROGMEM = HEATCTL_MQTT_TOPIC "/uptime";
static const char topic_radiator_on_time[] PROGMEM = HEATCTL_MQTT_TOPIC "/radiator_on_time";
static const char topic_hotwater_on_time[] PROGMEM = HEATCTL_MQTT_TOPIC "/hotwater_on_time";
static const char topic_burner_on_time[] PROGMEM = HEATCTL_MQTT_TOPIC "/burner_on_time";
static const char topic_request_hotwater[] PROGMEM = HEATCTL_MQTT_TOPIC "/request_hotwater";
#ifdef HEATCTL_CIRCPUMP_SUPPORT
static const char topic_circpump_on[] PROGMEM = HEATCTL_MQTT_TOPIC "/circpump_on";
static const char topic_circpump_on_time[] PROGMEM = HEATCTL_MQTT_TOPIC "/circpump_on_time";
static const char topic_request_circpump[] PROGMEM = HEATCTL_MQTT_TOPIC "/request_circpump";
#endif

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

static int8_t mqtt_state = MQTT_STATE_DISABLED;

static bool send_str(PGM_P topic, const char *value)
{
  if (!mqtt_construct_publish_packet_P(topic, value, strlen(value), MQTT_RETAIN)) {
    return false;
  }

  mqtt_state++;
  return true;
}

static bool send_temp(PGM_P topic, int16_t value)
{
  char buf[8];

  itoa_fixedpoint(value, 2, buf, sizeof(buf));
  return send_str(topic, buf);
}

static bool send_bool(PGM_P topic, uint8_t value)
{
  char buf[8];

  strncpy_P(buf, value ? PSTR("true") : PSTR("false"), sizeof(buf));
  return send_str(topic, buf);
}

static bool send_idx(PGM_P topic, int8_t value)
{
  char buf[8];

  itoa(value, buf, 10);
  return send_str(topic, buf);
}

static bool send_mode(PGM_P topic, uint8_t value)
{
  char buf[16];

  strncpy_P(buf, (const char *) pgm_read_word(&mode_str_tab[value]), sizeof(buf));
  return send_str(topic, buf);
}

static bool send_timestamp(PGM_P topic, timestamp_t value)
{
  ldiv_t dv_min;
  ldiv_t dv_hr;
  char buf[16];

  dv_min = ldiv(value, 60);
  dv_hr = ldiv(dv_min.quot, 60);
  snprintf_P(buf, sizeof(buf),
    PSTR("%lu:%02lu:%02lu"),
    dv_hr.quot, dv_hr.rem, dv_min.rem);
  return send_str(topic, buf);
}

static void connack_cb(void)
{
  mqtt_state = MQTT_STATE_IDLE;
}

static void poll_cb(void)
{
  bool ok;

  if (mqtt_state < 0)
    return;

  do {
    switch (mqtt_state)
    {
      case 0:
        ok = send_mode(topic_mode, heatctl_eeprom.mode);
        break;
      case 1:
        ok = send_temp(topic_boiler_temp, heatctl_boiler_temp);
        break;
      case 2:
        ok = send_temp(topic_boiler_setpoint, heatctl_boiler_setpoint);
        break;
      case 3:
        ok = send_bool(topic_burner_on, heatctl_burner_on);
        break;
      case 4:
        ok = send_temp(topic_outdoor_temp, heatctl_outdoor_temp);
        break;
      case 5:
        ok = send_idx(topic_radiator_index, heatctl_radiator_index);
        break;
      case 6:
        ok = send_temp(topic_radiator_setpoint, heatctl_radiator_setpoint);
        break;
      case 7:
        ok = send_bool(topic_radiator_on, heatctl_radiator_on);
        break;
      case 8:
        ok = send_temp(topic_hotwater_temp, heatctl_hotwater_temp);
        break;
      case 9:
        ok = send_idx(topic_hotwater_index, heatctl_hotwater_index);
        break;
      case 10:
        ok = send_idx(topic_hotwater_req, heatctl_hotwater_req);
        break;
      case 11:
        ok = send_temp(topic_hotwater_setpoint, heatctl_hotwater_setpoint);
        break;
      case 12:
        ok = send_bool(topic_hotwater_on, heatctl_hotwater_on);
        break;
      case 13:
        ok = send_timestamp(topic_uptime, clock_get_uptime());
        break;
      case 14:
        ok = send_timestamp(topic_radiator_on_time, heatctl_radiator_on_time);
        break;
      case 15:
        ok = send_timestamp(topic_hotwater_on_time, heatctl_hotwater_on_time);
        break;
      case 16:
        ok = send_timestamp(topic_burner_on_time, heatctl_burner_on_time);
        break;
#ifdef HEATCTL_CIRCPUMP_SUPPORT
      case 17:
        ok = send_bool(topic_circpump_on, heatctl_circpump_on);
        break;
      case 18:
        ok = send_timestamp(topic_circpump_on_time, heatctl_circpump_on_time);
        break;
#endif
      default:
        mqtt_state = MQTT_STATE_IDLE;
        return;
    }
  } while (ok);
}

static void close_cb(void)
{
  mqtt_state = MQTT_STATE_DISABLED;
}

static void publish_cb(char const *topic, uint16_t topic_length,
                       const void *payload, uint16_t payload_length,
                       bool retained)
{
}

static const mqtt_callback_config_t mqtt_callback_config PROGMEM = {
  .connack_callback = connack_cb,
  .poll_callback = poll_cb,
  .close_callback = close_cb,
  .publish_callback = publish_cb
};

void heatctl_mqtt_init()
{
  mqtt_register_callback(&mqtt_callback_config);
}

void heatctl_mqtt_periodic(void)
{
  if (mqtt_state == MQTT_STATE_IDLE) {
    mqtt_state = MQTT_STATE_START;
  }
}

/*
  -- Ethersex META --
  header(services/heatctl/heatctl_mqtt.h)
  net_init(heatctl_mqtt_init)
  timer(500, heatctl_mqtt_periodic())
*/


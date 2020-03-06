/*
 *
 * Copyright(c) 2020 by Sascha Ittner <sascha.ittner@modusoft.de>
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

#include "heatctl.h"
#include "heatctl_homie.h"

#include "services/clock/clock.h"

static const char node_id[] PROGMEM = "heatctl";
static const char node_name[] PROGMEM = "Heating Control";

static const char mode_id[] PROGMEM = "mode";
static const char mode_name[] PROGMEM = "Operating mode";

static const char boiler_temp_id[] PROGMEM = "boil_temp";
static const char boiler_temp_name[] PROGMEM = "Boiler current temperature";

static const char boiler_setpoint_id[] PROGMEM = "boil_sp";
static const char boiler_setpoint_name[] PROGMEM = "Boiler temperature setpoint";

static const char burner_on_id[] PROGMEM = "burn_on";
static const char burner_on_name[] PROGMEM = "Burner is on";

static const char outdoor_temp_id[] PROGMEM = "out_temp";
static const char outdoor_temp_name[] PROGMEM = "Outdoor temperature";

static const char radiator_index_id[] PROGMEM = "rad_idx";
static const char radiator_index_name[] PROGMEM = "Radiator setpoint index";

static const char radiator_setpoint_id[] PROGMEM = "rad_sp";
static const char radiator_setpoint_name[] PROGMEM = "Radiator setpoint temperature";

static const char radiator_on_id[] PROGMEM = "rad_on";
static const char radiator_on_name[] PROGMEM = "Radiator is heating";

static const char hotwater_temp_id[] PROGMEM = "hw_temp";
static const char hotwater_temp_name[] PROGMEM = "Hotwater current temperature";

static const char hotwater_index_id[] PROGMEM = "hw_idx";
static const char hotwater_index_name[] PROGMEM = "Hotwater setpoint index";

static const char hotwater_req_id[] PROGMEM = "hw_req";
static const char hotwater_req_name[] PROGMEM = "Hotwater request index";

static const char hotwater_setpoint_id[] PROGMEM = "hw_sp";
static const char hotwater_setpoint_name[] PROGMEM = "Hotwater setpoint temperature";

static const char hotwater_on_id[] PROGMEM = "hw_on";
static const char hotwater_on_name[] PROGMEM = "Hotwater is heating";

static const char uptime_id[] PROGMEM = "run_time";
static const char uptime_name[] PROGMEM = "Controller uptime";

static const char radiator_on_time_id[] PROGMEM = "rad_time";
static const char radiator_on_time_name[] PROGMEM = "Radiator heating time";

static const char hotwater_on_time_id[] PROGMEM = "hw_time";
static const char hotwater_on_time_name[] PROGMEM = "Hotwater heating time";

static const char burner_on_time_id[] PROGMEM = "burn_time";
static const char burner_on_time_name[] PROGMEM = "Burner operating time";

static const char request_hotwater_id[] PROGMEM = "req_hw";
static const char request_hotwater_name[] PROGMEM = "Manual hotwater request";

#ifdef HEATCTL_CIRCPUMP_SUPPORT
static const char circpump_on_id[] PROGMEM = "cpmp_on";
static const char circpump_on_name[] PROGMEM = "Hotwater circulator pump is on";

static const char circpump_on_time_id[] PROGMEM = "cpmp_time";
static const char circpump_on_time_name[] PROGMEM = "Hotwater circulator pump operating time";

static const char request_circpump_id[] PROGMEM = "req_cpmp";
static const char request_circpump_name[] PROGMEM = "Manual hotwater circulator pump request";
#endif

static const char temp_unit[] PROGMEM = "°C";
static const char temp_format[] PROGMEM = "-55.0:125.0";

static const char time_unit[] PROGMEM = "h:mm:ss";

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

static bool node_init_callback(void);

static bool mode_format_callback(int8_t array_idx);
static bool radiator_index_format_callback(int8_t array_idx);
static bool hotwater_index_format_callback(int8_t array_idx);
static bool hotwater_req_format_callback(int8_t array_idx);

static bool mode_output_callback(int8_t array_idx);
static bool boiler_temp_output_callback(int8_t array_idx);
static bool boiler_setpoint_output_callback(int8_t array_idx);
static bool burner_on_output_callback(int8_t array_idx);
static bool outdoor_temp_output_callback(int8_t array_idx);
static bool radiator_index_output_callback(int8_t array_idx);
static bool radiator_setpoint_output_callback(int8_t array_idx);
static bool radiator_on_output_callback(int8_t array_idx);
static bool hotwater_temp_output_callback(int8_t array_idx);
static bool hotwater_index_output_callback(int8_t array_idx);
static bool hotwater_req_output_callback(int8_t array_idx);
static bool hotwater_setpoint_output_callback(int8_t array_idx);
static bool hotwater_on_output_callback(int8_t array_idx);
static bool uptime_output_callback(int8_t array_idx);
static bool radiator_on_time_output_callback(int8_t array_idx);
static bool hotwater_on_time_output_callback(int8_t array_idx);
static bool burner_on_time_output_callback(int8_t array_idx);
static void request_hotwater_input_callback(int8_t array_idx, const char *payload, uint16_t payload_length, bool retained);

#ifdef HEATCTL_CIRCPUMP_SUPPORT
static bool circpump_on_output_callback(int8_t array_idx);
static bool circpump_on_time_output_callback(int8_t array_idx);
static void request_circpump_input_callback(int8_t array_idx, const char *payload, uint16_t payload_length, bool retained);
#endif

static const mqtt_homie_property_t properties[] PROGMEM =
{
  {
    .id = mode_id,
    .name = mode_name,
    .datatype = HOMIE_DATATYPE_ENUM,
    .format_callback = mode_format_callback,
    .output_callback = mode_output_callback
  },
  {
    .id = boiler_temp_id,
    .name = boiler_temp_name,
    .unit = temp_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = temp_format,
    .output_callback = boiler_temp_output_callback
  },
  {
    .id = boiler_setpoint_id,
    .name = boiler_setpoint_name,
    .unit = temp_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = temp_format,
    .output_callback = boiler_setpoint_output_callback
  },
  {
    .id = burner_on_id,
    .name = burner_on_name,
    .datatype = HOMIE_DATATYPE_BOOL,
    .output_callback = burner_on_output_callback
  },
  {
    .id = outdoor_temp_id,
    .name = outdoor_temp_name,
    .unit = temp_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = temp_format,
    .output_callback = outdoor_temp_output_callback
  },
  {
    .id = radiator_index_id,
    .name = radiator_index_name,
    .datatype = HOMIE_DATATYPE_INTEGER,
    .format_callback = radiator_index_format_callback,
    .output_callback = radiator_index_output_callback
  },
  {
    .id = radiator_setpoint_id,
    .name = radiator_setpoint_name,
    .unit = temp_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = temp_format,
    .output_callback = radiator_setpoint_output_callback
  },
  {
    .id = radiator_on_id,
    .name = radiator_on_name,
    .datatype = HOMIE_DATATYPE_BOOL,
    .output_callback = radiator_on_output_callback
  },
  {
    .id = hotwater_temp_id,
    .name = hotwater_temp_name,
    .unit = temp_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = temp_format,
    .output_callback = hotwater_temp_output_callback
  },
  {
    .id = hotwater_index_id,
    .name = hotwater_index_name,
    .datatype = HOMIE_DATATYPE_INTEGER,
    .format_callback = hotwater_index_format_callback,
    .output_callback = hotwater_index_output_callback
  },
  {
    .id = hotwater_req_id,
    .name = hotwater_req_name,
    .datatype = HOMIE_DATATYPE_INTEGER,
    .format_callback = hotwater_req_format_callback,
    .output_callback = hotwater_req_output_callback
  },
  {
    .id = hotwater_setpoint_id,
    .name = hotwater_setpoint_name,
    .unit = temp_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = temp_format,
    .output_callback = hotwater_setpoint_output_callback
  },
  {
    .id = hotwater_on_id,
    .name = hotwater_on_name,
    .datatype = HOMIE_DATATYPE_BOOL,
    .output_callback = hotwater_on_output_callback
  },
  {
    .id = uptime_id,
    .name = uptime_name,
    .unit = time_unit,
    .datatype = HOMIE_DATATYPE_STRING,
    .output_callback = uptime_output_callback
  },
  {
    .id = radiator_on_time_id,
    .name = radiator_on_time_name,
    .unit = time_unit,
    .datatype = HOMIE_DATATYPE_STRING,
    .output_callback = radiator_on_time_output_callback
  },
  {
    .id = hotwater_on_time_id,
    .name = hotwater_on_time_name,
    .unit = time_unit,
    .datatype = HOMIE_DATATYPE_STRING,
    .output_callback = hotwater_on_time_output_callback
  },
  {
    .id = burner_on_time_id,
    .name = burner_on_time_name,
    .unit = time_unit,
    .datatype = HOMIE_DATATYPE_STRING,
    .output_callback = burner_on_time_output_callback
  },
  {
    .id = request_hotwater_id,
    .name = request_hotwater_name,
    .datatype = HOMIE_DATATYPE_BOOL,
    .input_callback = request_hotwater_input_callback,
  },
#ifdef HEATCTL_CIRCPUMP_SUPPORT
  {
    .id = circpump_on_id,
    .name = circpump_on_name,
    .datatype = HOMIE_DATATYPE_BOOL,
    .output_callback = circpump_on_output_callback
  },
  {
    .id = circpump_on_time_id,
    .name = circpump_on_time_name,
    .unit = time_unit,
    .datatype = HOMIE_DATATYPE_STRING,
    .output_callback = circpump_on_time_output_callback
  },
  {
    .id = request_circpump_id,
    .name = request_circpump_name,
    .datatype = HOMIE_DATATYPE_BOOL,
    .input_callback = request_circpump_input_callback,
  },
#endif

  { .id = NULL }
};

const mqtt_homie_node_t heatctl_homie_node PROGMEM =
{
  .id = node_id,
  .name = node_name,
  .init_callback = node_init_callback,
  .properties = properties
};

static uint8_t mode_old;

static int8_t radiator_index_old;
static int8_t hotwater_index_old;
static int8_t hotwater_req_old;

static int16_t boiler_temp_old;
static int16_t hotwater_temp_old;
static int16_t outdoor_temp_old;

static int16_t radiator_setpoint_old;
static int16_t hotwater_setpoint_old;
static int16_t boiler_setpoint_old;

static uint8_t radiator_on_old;
static uint8_t hotwater_on_old;
static uint8_t burner_on_old;

static bool send_uptime;
static bool send_radiator_on_time;
static bool send_hotwater_on_time;
static bool send_burner_on_time;

#ifdef HEATCTL_CIRCPUMP_SUPPORT
static uint8_t circpump_on_old;
static bool send_circpump_on_time;
#endif

static bool send_mode(PGM_P prop_id, uint8_t value, uint8_t *old);
static bool send_temp(PGM_P prop_id, int16_t value, int16_t *old);
static bool send_bool(PGM_P prop_id, uint8_t value, uint8_t *old);
static bool send_idx(PGM_P prop_id, int8_t value, int8_t *old);
static bool send_timestamp(PGM_P prop_id, timestamp_t value, bool *send_flag);

static bool
node_init_callback(void)
{
  mode_old = UINT8_MAX;

  radiator_index_old = INT8_MAX;
  hotwater_index_old = INT8_MAX;
  hotwater_req_old = INT8_MAX;

  boiler_temp_old = INT16_MAX;
  hotwater_temp_old = INT16_MAX;
  outdoor_temp_old = INT16_MAX;

  radiator_setpoint_old = INT16_MAX;
  hotwater_setpoint_old = INT16_MAX;
  boiler_setpoint_old = INT16_MAX;

  radiator_on_old = UINT8_MAX;
  hotwater_on_old = UINT8_MAX;
  burner_on_old = UINT8_MAX;

  send_uptime = true;
  send_radiator_on_time = true;
  send_hotwater_on_time = true;
  send_burner_on_time = true;

#ifdef HEATCTL_CIRCPUMP_SUPPORT
  circpump_on_old = UINT8_MAX;
  send_circpump_on_time = true;
#endif

  return true;
}

void
heatctl_homie_periodic(void)
{
  // set flags to send timestamps
  send_uptime = true;
  send_radiator_on_time = true;
  send_hotwater_on_time = true;
  send_burner_on_time = true;
#ifdef HEATCTL_CIRCPUMP_SUPPORT
  send_circpump_on_time = true;
#endif
}

static bool
mode_format_callback(int8_t array_idx)
{
  bool first;
  PGM_P const *p;
  PGM_P s;
  for (first = true, p = mode_str_tab; (s = (PGM_P) pgm_read_word(p)) != NULL; first = false, p++)
  {
    if (!mqtt_construct_publish_packet_payload(PSTR("%S%S"), first ? PSTR("") : PSTR(","), s))
      return false;
  }

  return true;
}

static bool
radiator_index_format_callback(int8_t array_idx)
{
  return mqtt_construct_publish_packet_payload(PSTR("0:%u"), HEATCTL_RADIATOR_ITEM_COUNT);
}

static bool
hotwater_index_format_callback(int8_t array_idx)
{
  return mqtt_construct_publish_packet_payload(PSTR("0:%u"), HEATCTL_HOTWATER_ITEM_COUNT);
}

static bool
hotwater_req_format_callback(int8_t array_idx)
{
  return mqtt_construct_publish_packet_payload(PSTR("-1:%u"), HEATCTL_HOTWATER_ITEM_COUNT);
}

static bool
send_mode(PGM_P prop_id, uint8_t value, uint8_t *old)
{
  if (value == *old)
    return true;

  mqtt_homie_header_prop_value(node_id, prop_id, HOMIE_ARRAY_FLAG_NOARR, true);
  mqtt_construct_publish_packet_payload(PSTR("%S"), (PGM_P) pgm_read_word(&mode_str_tab[value]));
  if (!mqtt_construct_publish_packet_fin())
    return false;

  *old = value;
  return true;
}

static bool
send_temp(PGM_P prop_id, int16_t value, int16_t *old)
{
  if (value == *old)
    return true;

  mqtt_homie_header_prop_value(node_id, prop_id, HOMIE_ARRAY_FLAG_NOARR, true);
  mqtt_construct_publish_packet_payload(PSTR("%d.%02d"), value / 100, abs(value % 100));
  if (!mqtt_construct_publish_packet_fin())
    return false;

  *old = value;
  return true;
}

static bool
send_bool(PGM_P prop_id, uint8_t value, uint8_t *old)
{
  if (value == *old)
    return true;

  mqtt_homie_header_prop_value(node_id, prop_id, HOMIE_ARRAY_FLAG_NOARR, true);
  mqtt_construct_publish_packet_payload(PSTR("%S"), mqtt_homie_bool(value));
  if (!mqtt_construct_publish_packet_fin())
    return false;

  *old = value;
  return true;
}

static bool
send_idx(PGM_P prop_id, int8_t value, int8_t *old)
{
  if (value == *old)
    return true;

  mqtt_homie_header_prop_value(node_id, prop_id, HOMIE_ARRAY_FLAG_NOARR, true);
  mqtt_construct_publish_packet_payload(PSTR("%d"), value);
  if (!mqtt_construct_publish_packet_fin())
    return false;

  *old = value;
  return true;
}

static bool
send_timestamp(PGM_P prop_id, timestamp_t value, bool *send_flag)
{
  if (! *send_flag)
    return true;

  ldiv_t dv_min = ldiv(value, 60);
  ldiv_t dv_hr = ldiv(dv_min.quot, 60);
  mqtt_homie_header_prop_value(node_id, prop_id, HOMIE_ARRAY_FLAG_NOARR, true);
  mqtt_construct_publish_packet_payload(PSTR("%lu:%02lu:%02lu"), dv_hr.quot, dv_hr.rem, dv_min.rem);
  if (!mqtt_construct_publish_packet_fin())
    return false;

  *send_flag = false;
  return true;
}


static bool
mode_output_callback(int8_t array_idx)
{
  return send_mode(mode_id, heatctl_eeprom.mode, &mode_old);
}

static bool
boiler_temp_output_callback(int8_t array_idx)
{
  return send_temp(boiler_temp_id, heatctl_boiler_temp, &boiler_temp_old);
}

static bool
boiler_setpoint_output_callback(int8_t array_idx)
{
  return send_temp(boiler_setpoint_id, heatctl_boiler_setpoint, &boiler_setpoint_old);
}

static bool
burner_on_output_callback(int8_t array_idx)
{
  return send_bool(burner_on_id, heatctl_burner_on, &burner_on_old);
}

static bool
outdoor_temp_output_callback(int8_t array_idx)
{
  return send_temp(outdoor_temp_id, heatctl_outdoor_temp, &outdoor_temp_old);
}

static bool
radiator_index_output_callback(int8_t array_idx)
{
  return send_idx(radiator_index_id, heatctl_radiator_index, &radiator_index_old);
}

static bool
radiator_setpoint_output_callback(int8_t array_idx)
{
  return send_temp(radiator_setpoint_id, heatctl_radiator_setpoint, &radiator_setpoint_old);
}

static bool
radiator_on_output_callback(int8_t array_idx)
{
  return send_bool(radiator_on_id, heatctl_radiator_on, &radiator_on_old);
}

static bool
hotwater_temp_output_callback(int8_t array_idx)
{
  return send_temp(hotwater_temp_id, heatctl_hotwater_temp, &hotwater_temp_old);
}

static bool
hotwater_index_output_callback(int8_t array_idx)
{
  return send_idx(hotwater_index_id, heatctl_hotwater_index, &hotwater_index_old);
}

static bool
hotwater_req_output_callback(int8_t array_idx)
{
  return send_idx(hotwater_req_id, heatctl_hotwater_req, &hotwater_req_old);
}

static bool
hotwater_setpoint_output_callback(int8_t array_idx)
{
  return send_temp(hotwater_setpoint_id, heatctl_hotwater_setpoint, &hotwater_setpoint_old);
}

static bool
hotwater_on_output_callback(int8_t array_idx)
{
  return send_bool(hotwater_on_id, heatctl_hotwater_on, &hotwater_on_old);
}

static bool
uptime_output_callback(int8_t array_idx)
{
  return send_timestamp(uptime_id, clock_get_uptime(), &send_uptime);
}

static bool
radiator_on_time_output_callback(int8_t array_idx)
{
  return send_timestamp(radiator_on_time_id, heatctl_radiator_on_time, &send_radiator_on_time);
}

static bool
hotwater_on_time_output_callback(int8_t array_idx)
{
  return send_timestamp(hotwater_on_time_id, heatctl_hotwater_on_time, &send_hotwater_on_time);
}

static bool
burner_on_time_output_callback(int8_t array_idx)
{
  return send_timestamp(burner_on_time_id, heatctl_burner_on_time, &send_burner_on_time);
}

static void
request_hotwater_input_callback(int8_t array_idx, const char *payload, uint16_t payload_length, bool retained)
{
  if (strncmp_P(payload, mqtt_homie_bool(true), payload_length) == 0)
  {
    heatctl_hotwater_req = 1;
    return;
  }

  if (strncmp_P(payload, mqtt_homie_bool(false), payload_length) == 0)
  {
    heatctl_hotwater_req = -1;
    return;
  }
}

#ifdef HEATCTL_CIRCPUMP_SUPPORT
static bool
circpump_on_output_callback(int8_t array_idx)
{
  return send_bool(circpump_on_id, heatctl_circpump_on, &circpump_on_old);
}

static bool
circpump_on_time_output_callback(int8_t array_idx)
{
  return send_timestamp(circpump_on_time_id, heatctl_circpump_on_time, &send_circpump_on_time);
}

static void
request_circpump_input_callback(int8_t array_idx, const char *payload, uint16_t payload_length, bool retained)
{
  if (strncmp_P(payload, mqtt_homie_bool(true), payload_length) == 0)
  {
    heatctl_circpump_cmd = 1;
    return;
  }

  if (strncmp_P(payload, mqtt_homie_bool(false), payload_length) == 0)
  {
    heatctl_circpump_cmd = 0;
    return;
  }
}
#endif

/*
  -- Ethersex META --
  header(services/heatctl/heatctl_homie.h)
  timer(500, heatctl_homie_periodic())
*/


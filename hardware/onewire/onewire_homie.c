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

#include "onewire.h"
#include "onewire_homie.h"

#include <stdlib.h>

static const char node_id[] PROGMEM = "ow";
static const char node_name[] PROGMEM = "OneWire Sensors";

static const char prop_id[] PROGMEM = "temp";
static const char prop_unit[] PROGMEM = "°C";
static const char prop_format[] PROGMEM = "-55.0:125.0";

static bool node_init_callback(void);
static int8_t prop_array_count_callback(void);
static bool prop_name_callback(int8_t array_idx);
static bool prop_output_callback(int8_t array_idx);

static const mqtt_homie_property_t properties[] PROGMEM =
{
  {
    .id = prop_id,
    .array_count_callback = prop_array_count_callback,
    .name_callback = prop_name_callback,
    .unit = prop_unit,
    .datatype = HOMIE_DATATYPE_FLOAT,
    .format = prop_format,
    .output_callback = prop_output_callback
  },

  { .id = NULL }
};

const mqtt_homie_node_t ow_homie_node PROGMEM =
{
  .id = node_id,
  .name = node_name,
  .init_callback = node_init_callback,
  .properties = properties
};

static int8_t array_count;
static uint8_t sensor_map[OW_SENSORS_COUNT];

static bool
node_init_callback(void)
{
  array_count = -1;
  return true;
}

static int8_t
prop_array_count_callback(void)
{
  if (array_count >= 0)
    return array_count;

  uint8_t i;
  for (array_count = 0, i = 0; i < OW_SENSORS_COUNT; i++)
  {
    if (ow_sensors[i].named)
      sensor_map[array_count++] = i;
  }

  return array_count;
}

static bool
prop_name_callback(int8_t array_idx)
{
  ow_sensor_t *sensor = &ow_sensors[sensor_map[array_idx]];
  return mqtt_construct_publish_packet_payload(PSTR("%s"), sensor->name);
}

static bool
prop_output_callback(int8_t array_idx)
{
  ow_sensor_t *sensor = &ow_sensors[sensor_map[array_idx]];

  if (!sensor->homie)
    return true;

  mqtt_homie_header_prop_value(node_id, prop_id, array_idx, true);

  if (sensor->temp.twodigits)
  {
    mqtt_construct_publish_packet_payload(PSTR("%d.%02d"), sensor->temp.val / 100, abs(sensor->temp.val % 100));
  } else {
    mqtt_construct_publish_packet_payload(PSTR("%d.%01d"), sensor->temp.val / 10, abs(sensor->temp.val % 10));
  }

  if (!mqtt_construct_publish_packet_fin())
    return false;

  sensor->homie = 0;
  return true;
}


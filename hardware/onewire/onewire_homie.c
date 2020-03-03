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

static const char prop_id[] PROGMEM = "temperature";
static const char prop_name[] PROGMEM = "Temperature";
static const char prop_unit[] PROGMEM = "°C";
static const char prop_format[] PROGMEM = "-55.0:125.0";

static bool node_init_callback(void);
static bool node_array_callback(void);
static bool prop_output_callback(void);

static const mqtt_homie_property_t properties[] PROGMEM =
{
  {
    .id = prop_id,
    .name = prop_name,
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
  .array_callback = node_array_callback,
  .properties = properties
};

static uint8_t sensor_index;
static uint8_t sensor_map[OW_SENSORS_COUNT];

static uint8_t node_count;
static uint8_t node_index;

static bool
node_init_callback(void)
{
  sensor_index = 0;
  node_count = 0;
  node_index = 0;

  return true;
}

static bool
node_array_callback(void)
{
  for (; sensor_index < OW_SENSORS_COUNT; sensor_index++)
  {
    if (!ow_sensors[sensor_index].named)
      continue;

    mqtt_homie_header_array_node_name(node_id, node_count);
    mqtt_construct_publish_packet_payload(PSTR("%s"), ow_sensors[sensor_index].name);
    if (!mqtt_construct_publish_packet_fin())
      return false;

    sensor_map[node_count] = sensor_index;
    node_count++;
  }

  mqtt_homie_header_array_range(node_id);
  mqtt_construct_publish_packet_payload(PSTR("0-%d"), node_count);
  return mqtt_construct_publish_packet_fin();
}

static bool
prop_output_callback(void)
{
  ow_sensor_t *sensor;

  for (; node_index < node_count; node_index++)
  {
    sensor = &ow_sensors[sensor_map[node_index]];

    // only send if new data available
    if (!sensor->homie)
      continue;

    mqtt_homie_header_array_prop_value(node_id, node_index, prop_id, true);
    if (sensor->temp.twodigits)
    {
      mqtt_construct_publish_packet_payload(PSTR("%d.%02d"), sensor->temp.val / 100, abs(sensor->temp.val % 100));
    } else {
      mqtt_construct_publish_packet_payload(PSTR("%d.%01d"), sensor->temp.val / 10, abs(sensor->temp.val % 10));
    }
    if (!mqtt_construct_publish_packet_fin())
      return false;

    sensor->homie = 0;
  }

  node_index = 0;
  return true;
}


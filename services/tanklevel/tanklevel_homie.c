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

#include "tanklevel.h"
#include "tanklevel_homie.h"

static const char node_id[] PROGMEM = "tanklevel";
static const char node_name[] PROGMEM = "Tank Level";

static const char level_id[] PROGMEM = "level";
static const char level_name[] PROGMEM = "Current Level";
static const char level_unit[] PROGMEM = "l";

static const char trig_id[] PROGMEM = "trigger";
static const char trig_name[] PROGMEM = "Start measurement trigger";

static bool node_init_callback(void);
static bool level_format_callback(int8_t array_idx);
static bool level_output_callback(int8_t array_idx);
static void trig_input_callback(int8_t array_idx, const char *payload, uint16_t payload_length, bool retained);

static const mqtt_homie_property_t properties[] PROGMEM =
{
  {
    .id = level_id,
    .name = level_name,
    .unit = level_unit,
    .datatype = HOMIE_DATATYPE_INTEGER,
    .format_callback = level_format_callback,
    .output_callback = level_output_callback
  },
  {
    .id = trig_id,
    .name = trig_name,
    .settable = true,
    .datatype = HOMIE_DATATYPE_BOOL,
    .input_callback = trig_input_callback
  },

  { .id = NULL }
};

const mqtt_homie_node_t tanklevel_homie_node PROGMEM =
{
  .id = node_id,
  .name = node_name,
  .init_callback = node_init_callback,
  .properties = properties
};

static bool init_trig;

static bool
node_init_callback(void)
{
  init_trig = true;
  return true;
}

static bool
level_format_callback(int8_t array_idx)
{
  return mqtt_construct_publish_packet_payload(PSTR("0:%u"), tanklevel_params_ram.ltr_full);
}

static bool
level_output_callback(int8_t array_idx)
{
  // only send if new data available
  if (!tanklevel_homie_valid)
    return true;

  mqtt_homie_header_prop_value(node_id, level_id, HOMIE_ARRAY_FLAG_NOARR, true);
  mqtt_construct_publish_packet_payload(PSTR("%u"), tanklevel_get());
  if (!mqtt_construct_publish_packet_fin())
    return false;

  tanklevel_homie_valid = false;
  return true;
}

static void trig_input_callback(int8_t array_idx, const char *payload, uint16_t payload_length, bool retained)
{
  if (strncmp_P(payload, mqtt_homie_bool(true), payload_length) == 0)
  {
    tanklevel_start();
  }
}


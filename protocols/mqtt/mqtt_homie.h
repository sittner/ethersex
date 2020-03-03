/*
 * Copyright (c) 2014 by Philip Matura <ike@tura-home.de>
 * Copyright (c) 2015 by Daniel Lindner <daniel.lindner@gmx.de>
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

#ifndef HAVE_MQTT_HOMIE_H
#define HAVE_MQTT_HOMIE_H

#include "protocols/mqtt/mqtt.h"

#include <stdint.h>
#include <stdbool.h>
#include <avr/pgmspace.h>

typedef bool (*mqtt_homie_output_callback_t) (void);

typedef enum
{
  HOMIE_DATATYPE_STRING,
  HOMIE_DATATYPE_INTEGER,
  HOMIE_DATATYPE_FLOAT,
  HOMIE_DATATYPE_BOOL,
  HOMIE_DATATYPE_ENUM,
  HOMIE_DATATYPE_COLOR
} mqtt_homie_datatype_t;

typedef struct
{
  PGM_P id;
  PGM_P name;
  uint8_t settable;
  PGM_P unit;
  uint8_t datatype;
  PGM_P format;
  mqtt_homie_output_callback_t format_callback;
  mqtt_homie_output_callback_t output_callback;
  publish_callback input_callback;
} mqtt_homie_property_t;

typedef struct
{
  PGM_P id;
  PGM_P name;
  PGM_P type;
  mqtt_homie_output_callback_t init_callback;
  mqtt_homie_output_callback_t array_callback;
  const mqtt_homie_property_t *properties;
} mqtt_homie_node_t;

void mqtt_homie_init(void);
void mqtt_homie_periodic(void);

bool mqtt_homie_header_array_node_name(PGM_P node_id, uint8_t node_index);
bool mqtt_homie_header_array_range(PGM_P node_id);
bool mqtt_homie_header_array_prop_value(PGM_P node_id, uint8_t node_index, PGM_P prop_id, bool retain);

bool mqtt_homie_header_prop_format(PGM_P node_id, PGM_P prop_id);
bool mqtt_homie_header_prop_value(PGM_P node_id, PGM_P prop_id, bool retain);

PGM_P mqtt_homie_bool(bool value);

#endif /* HAVE_MQTT_HOMIE_H */

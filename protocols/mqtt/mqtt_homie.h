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

typedef bool (*mqtt_homie_bool_void_callback_t) (void);
typedef int8_t (*mqtt_homie_s8_void_callback_t) (void);
typedef bool (*mqtt_homie_bool_s8_callback_t) (int8_t);
typedef void (*input_callback_t) (int8_t array_idx, const char *payload, uint16_t payload_length, bool retained);

#define HOMIE_ARRAY_FLAG_NOARR -1
#define HOMIE_ARRAY_FLAG_INIT  -2

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
  mqtt_homie_s8_void_callback_t array_count_callback;
  PGM_P name;
  mqtt_homie_bool_s8_callback_t name_callback;
  PGM_P unit;
  uint8_t datatype;
  PGM_P format;
  mqtt_homie_bool_s8_callback_t format_callback;
  mqtt_homie_bool_s8_callback_t output_callback;
  input_callback_t input_callback;
} mqtt_homie_property_t;

typedef struct
{
  PGM_P id;
  PGM_P name;
  PGM_P type;
  mqtt_homie_bool_void_callback_t init_callback;
  const mqtt_homie_property_t *properties;
} mqtt_homie_node_t;

void mqtt_homie_init(void);
void mqtt_homie_periodic(void);

bool mqtt_homie_header_prop_value(PGM_P node_id, PGM_P prop_id, int8_t array_idx, bool retain);

PGM_P mqtt_homie_bool(bool value);

#endif /* HAVE_MQTT_HOMIE_H */

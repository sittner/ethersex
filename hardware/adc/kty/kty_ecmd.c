/*
 *
 * Copyright (c) by Alexander Neumann <alexander@bumpern.de>
 * Copyright (c) 2007 by Stefan Siegl <stesie@brokenpipe.de>
 * Copyright (c) 2007 by Christian Dietrich <stettberger@dokucode.de>
 * Copyright (c) 2008 by Jochen Roessner <jochen@lugrot.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (either version 2 or
 * version 3) as published by the Free Software Foundation.
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
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "config.h"
#include "core/debug.h"
#include "core/eeprom.h"
#include "hardware/adc/temp2text.h"
#include "hardware/adc/kty/kty.h"

#include "protocols/ecmd/ecmd-base.h"


#define NIBBLE_TO_HEX(a) ((a) < 10 ? (a) + '0' : ((a) - 10 + 'A')) 


int16_t parse_cmd_kty_get(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
	cmd++;

  if (*cmd) {
    if ((*cmd >= '0') && (*cmd - '0' < KTY_CHANNEL_COUNT)) {
      int16_t temp = kty_get(*cmd - '0');

      temp2text(output, temp);
      return ECMD_FINAL(5);
    }
  }
  else {
    uint8_t channel;

    for (channel = 0; channel < KTY_CHANNEL_COUNT; channel ++) {
      int16_t temp = kty_get(channel);

      temp2text(output, temp);
      output[5] = ' ';
      output += 6;
    }
    *output = '\0';
    return ECMD_FINAL(6 * KTY_CHANNEL_COUNT);
  }
  return ECMD_ERR_PARSE_ERROR;
}

#ifdef KTY_CALIBRATION_SUPPORT
int16_t parse_cmd_kty_cal_get(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
	cmd++;

  if (*cmd) {
    if ((*cmd >= '0') && (*cmd - '0' < KTY_CHANNEL_COUNT)) {
      int8_t cal;
      eeprom_restore_char(kty_calibration[*cmd - '0'], &cal);

      itoa(cal, output, 10);
      return ECMD_FINAL(strlen(output));
    }
  }
  return ECMD_ERR_PARSE_ERROR;
}


int16_t parse_cmd_kty_calibration(char *cmd, char *output, uint16_t len)
{
  while (*cmd == ' ')
	cmd++;

  if (*cmd) {
    if ((*cmd >= '0') && (*cmd - '0' < KTY_CHANNEL_COUNT)) {
      if (kty_calibrate(*cmd - '0')) {
        return ECMD_FINAL_OK;
      }
      else {
        strcpy_P (output, PSTR("Out of range"));
        return ECMD_FINAL(12); /* = strlen("Out of range") */
      }
    }
  }
  return ECMD_ERR_PARSE_ERROR;
}
#endif

/*
  -- Ethersex META --
  block([[KTY]])
  ecmd_feature(kty_get, "kty get", [CHANNEL], Get the temperature in xxx.x °C of CHANNEL or if no channel set of all channels.)
  ecmd_ifdef(KTY_CALIBRATION_SUPPORT)
    ecmd_feature(kty_cal_get, "kty cal get",, Return the calibration difference to 2k2 Resistor.)
    ecmd_feature(kty_calibration, "kty autocalibrate", CHANNEL, Calibrate to 1000 Ohm precision Resistor.)
  ecmd_endif()
*/

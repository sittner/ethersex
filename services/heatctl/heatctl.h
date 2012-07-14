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

#ifndef _HEATCTL_H
#define _HEATCTL_H

#define HEATCTL_MODE_MANU  0
#define HEATCTL_MODE_AUTO  1
#define HEATCTL_MODE_HOTW  2
#define HEATCTL_MODE_RADI  3
#define HEATCTL_MODE_SERV  4

#define HEATCTL_MAX_SLEW_RATE     5000
#define HEATCTL_MAX_CIRCPUMP_TIME 3600

typedef struct
{
  int16_t manu_temp_hotwater;
  int16_t manu_temp_radiator;
  int16_t hyst_boiler;
  int16_t hyst_hotwater;
  int16_t hyst_radiator;
  int16_t min_diff_hotwater;
  int16_t hotwater_offset;
  int16_t radiator_offset;
#ifdef HEATCTL_CIRCPUMP_SUPPORT
  uint16_t circpump_time;
#endif
} heatctl_params_t;

typedef struct
{
  int16_t threshold;
  int16_t slew_rate;
} heatctl_radiator_item_t;

typedef struct
{
  int16_t temp;
} heatctl_hotwater_item_t;

typedef struct
{
  uint8_t mode;
  heatctl_params_t params;
  heatctl_radiator_item_t radiator_items[HEATCTL_RADIATOR_ITEM_COUNT];
  heatctl_hotwater_item_t hotwater_items[HEATCTL_HOTWATER_ITEM_COUNT];
} heatctl_eeprom_t;

extern heatctl_eeprom_t heatctl_eeprom;
extern int8_t heatctl_radiator_index;
extern int8_t heatctl_hotwater_index;
extern int8_t heatctl_hotwater_req;

extern int16_t heatctl_boiler_temp;
extern int16_t heatctl_hotwater_temp;
extern int16_t heatctl_outdoor_temp;

extern int16_t heatctl_radiator_setpoint;
extern int16_t heatctl_hotwater_setpoint;
extern int16_t heatctl_boiler_setpoint;

extern uint8_t heatctl_radiator_on;
extern uint8_t heatctl_hotwater_on;
extern uint8_t heatctl_burner_on;

extern timestamp_t heatctl_radiator_on_time;
extern timestamp_t heatctl_hotwater_on_time;
extern timestamp_t heatctl_burner_on_time;

#ifdef HEATCTL_CIRCPUMP_SUPPORT
extern uint8_t heatctl_circpump_on;
extern uint8_t heatctl_circpump_cmd;
extern timestamp_t heatctl_circpump_on_time;
extern uint16_t heatctl_circpump_timer;
#endif

extern void heatctl_init(void);
extern void heatctl_periodic(void);

extern void heatctl_save_params(void);
extern void heatctl_set_mode(uint8_t mode);
extern void heatctl_save_radiator_items(void);
extern void heatctl_save_hotwater_items(void);

extern int16_t parse_cmd_heatctl_show_params(char *cmd, char *output,
                                             uint16_t len);
extern int16_t parse_cmd_heatctl_param_show(char *cmd, char *output,
                                            uint16_t len);
extern int16_t parse_cmd_heatctl_param_set(char *cmd, char *output,
                                           uint16_t len);
extern int16_t parse_cmd_heatctl_param_save(char *cmd, char *output,
                                            uint16_t len);
extern int16_t parse_cmd_heatctl_mode(char *cmd, char *output, uint16_t len);
extern int16_t parse_cmd_heatctl_radi_show(char *cmd, char *output,
                                           uint16_t len);
extern int16_t parse_cmd_heatctl_radi_set(char *cmd, char *output,
                                          uint16_t len);
extern int16_t parse_cmd_heatctl_radi_save(char *cmd, char *output,
                                           uint16_t len);
extern int16_t parse_cmd_heatctl_radi_sel(char *cmd, char *output,
                                          uint16_t len);
extern int16_t parse_cmd_heatctl_hotw_show(char *cmd, char *output,
                                           uint16_t len);
extern int16_t parse_cmd_heatctl_hotw_set(char *cmd, char *output,
                                          uint16_t len);
extern int16_t parse_cmd_heatctl_hotw_save(char *cmd, char *output,
                                           uint16_t len);
extern int16_t parse_cmd_heatctl_hotw_sel(char *cmd, char *output,
                                          uint16_t len);
extern int16_t parse_cmd_heatctl_hotw_req(char *cmd, char *output,
                                          uint16_t len);
extern int16_t parse_cmd_heatctl_state(char *cmd, char *output, uint16_t len);
#ifdef HEATCTL_CIRCPUMP_SUPPORT
extern int16_t parse_cmd_heatctl_circ(char *cmd, char *output, uint16_t len);
#endif

#endif

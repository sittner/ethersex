/* Copyright(C) 2007 Jochen Roessner <jochen@lugrot.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

// old size: 28084
// new size: 44440


#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "core/eeprom.h"

#include "kty.h"

#ifdef KTY_81_110_120_150_SUPPORT
const int16_t kty81_110_120_150_restab[] PROGMEM = {
  -600, 100, // start temp, step size
  465, 515, 567, 624, 684, 747, 815, 886, 961, 1040,
  1122, 1209, 1299, 1392, 1490, 1591, 1696, 1805, 1915, 2023,
  2124, 2211, 0
};
#endif

#ifdef KTY_81_121_SUPPORT
const int16_t kty81_121_restab[] PROGMEM PROGMEM = {
  -600, 100, // start temp, step size
  460, 510, 562, 617, 677, 740, 807, 877, 951, 1029,
  1111, 1196, 1286, 1378, 1475, 1575, 1679, 1786, 1896, 2003,
  2103, 2189, 0
};
#endif

#ifdef KTY_81_122_SUPPORT
const int16_t kty81_122_restab[] PROGMEM PROGMEM = {
  -600, 100, // start temp, step size
  470, 520, 573, 630, 690, 755, 823, 895, 971, 1050,
  1134, 1221, 1312, 1406, 1505, 1607, 1713, 1823, 1934, 2044,
  2146, 2233, 0
};
#endif

#ifdef KTY_81_210_210_220_250_SUPPORT
const int16_t kty81_210_220_250_restab[] PROGMEM = {
  -600, 100, // start temp, step size
  930, 1030, 1135, 1247, 1367, 1495, 1630, 1772, 1922, 2080,
  2245, 2417, 2597, 2785, 2980, 3182, 3392, 3607, 3817, 4008,
  4166, 4280, 0
};
#endif

#ifdef KTY_81_221_SUPPORT
const int16_t kty81_221_restab[] PROGMEM = {
  -600, 100, // start temp, step size
  921, 1019, 1123, 1235, 1354, 1480, 1613, 1754, 1903, 2059,
  2222, 2393, 2571, 2757, 2950, 3150, 3358, 3571, 3779, 3967,
  4125, 4237, 0
};
#endif

#ifdef KTY_81_222_SUPPORT
const int16_t kty81_222_restab[] PROGMEM = {
  -600, 100, // start temp, step size
  940, 1040, 1146, 1260, 1381, 1510, 1646, 1790, 1941, 2100,
  2267, 2441, 2623, 2812, 3009, 3214, 3426, 3643, 3855, 4048,
  4208, 4323, 0
};
#endif

#ifdef KTY_83_110_110_120_150_SUPPORT
const int16_t kty83_110_120_150_restab[] PROGMEM = {
  -600, 100, // start temp, step
  475, 525, 577, 632, 691, 754, 820, 889, 962, 1039,
  1118, 1202, 1288, 1379, 1472, 1569, 1670, 1774, 1882, 1993,
  2107, 2225, 2346, 2471, 2599, 0
};
#endif

#ifdef KTY_83_121_SUPPORT
const int16_t kty83_121_restab[] PROGMEM = {
  -600, 100, // start temp, step size
  471, 519, 571, 626, 685, 746, 812, 880, 953, 1028,
  1107, 1190, 1276, 1365, 1458, 1554, 1653, 1756, 1863, 1973,
  2086, 2203, 2323, 2446, 2572, 0
};
#endif

#ifdef KTY_83_122_SUPPORT
const int16_t kty83_122_restab[] PROGMEM = {
  -600, 100, // start temp, step size
  480, 530, 583, 639, 698, 762, 828, 898, 972, 1049,
  1130, 1214, 1301, 1392, 1487, 1585, 1687, 1792, 1900, 2012,
  2128, 2247, 2370, 2496, 2624, 0
};
#endif

#ifdef KTY_83_151_SUPPORT
const int16_t kty83_151_restab[] PROGMEM = {
  -600, 100, // start temp, step
  462, 512, 562, 617, 674, 735, 799, 867, 938, 1013,
  1090, 1172, 1256, 1344, 1435, 1530, 1628, 1730, 1835, 1943,
  2054, 2169, 2288, 2409, 2533, 0
};
#endif

#ifdef KTY_84_130_150_SUPPORT
const int16_t kty84_130_150_restab[] PROGMEM = {
  -400, 100, // start temp, step size
  359, 391, 424, 460, 498, 538, 581, 626, 672, 722,
  773, 826, 882, 940, 1000, 1062, 1127, 1194, 1262, 1334,
  1407, 1482, 1560, 1640, 1722, 1807, 1893, 1982, 2073, 2166,
  2261, 2357, 2452, 2542, 2624, 0
};
#endif

#ifdef KTY_84_151_SUPPORT
const int16_t kty84_151_restab[] PROGMEM = {
  -400, 100, // start temp, step size
  350, 381, 414, 449, 486, 525, 566, 610, 656, 704,
  754, 806, 860, 916, 975, 1036, 1099, 1164, 1231, 1300,
  1372, 1445, 1521, 1599, 1679, 1761, 1846, 1932, 2021, 2112,
  2205, 2298, 2391, 2479, 2558, 0
};
#endif

typedef struct
{
  uint8_t adc_channel;
  int16_t vcc_resistor;
  const int16_t *restab;
} kty_channel_t;


const kty_channel_t kty_channels[KTY_CHANNEL_COUNT] PROGMEM = { KTY_CHANNEL_DEFS };

#ifdef KTY_CALIBRATION_SUPPORT
int8_t
kty_calibrate(uint8_t channel)
{
  if (channel >= KTY_CHANNEL_COUNT) {
    return 0;
  }
  const kty_channel_t *ch = &kty_channels[channel];

  uint16_t volt = adc_get((uint8_t)pgm_read_byte(&ch->adc_channel));
#if KTY_REF_SOURCE == KTY_REF_VCC_MEASURE
  uint32_t vcc = adc_get(KTY_VCC_ADC_CHANNEL);
  vcc *= (KTY_VCC_RES_VCC + KTY_VCC_RES_GND);
  vcc /= KTY_VCC_RES_GND;
#elif KTY_REF_SOURCE == KTY_REF_VREF_EQ_VCC
  uint16_t vcc = ADC_RES - 1;
#else
  uint16_t vcc = (ADC_RES << 1) - 1;
#endif

  if (volt == 0) {
    return 0;
  }

  int32_t res = 1000L;
  res *= (vcc - volt);
  res /= volt;

  int16_t cali = (int16_t)pgm_read_word(&ch->vcc_resistor) - res;
  if (cali < -120 || cali > 120) {
    return 0;
  }

  eeprom_save_char (kty_calibration[channel], cali);
  eeprom_update_chksum();
  return 1;
}
#endif

int16_t
kty_get(uint8_t channel)
{
  if (channel >= KTY_CHANNEL_COUNT) {
    return 0;
  }
  const kty_channel_t *ch = &kty_channels[channel];

  uint16_t volt = adc_get((uint8_t)pgm_read_byte(&ch->adc_channel));
#if KTY_REF_SOURCE == KTY_REF_VCC_MEASURE
  uint32_t vcc = adc_get(KTY_VCC_ADC_CHANNEL);
  vcc *= (KTY_VCC_RES_VCC + KTY_VCC_RES_GND);
  vcc /= KTY_VCC_RES_GND;
#elif KTY_REF_SOURCE == KTY_REF_VREF_EQ_VCC
  uint16_t vcc = ADC_RES - 1;
#else
  uint16_t vcc = (ADC_RES << 1) - 1;
#endif

  if (volt >= vcc) {
    return 0;
  }

  int32_t res = (int16_t)pgm_read_word(&ch->vcc_resistor);
#ifdef KTY_CALIBRATION_SUPPORT
  int8_t cali;
  eeprom_restore_char (kty_calibration[channel], &cali);
  res += cali;
#endif
  res *= volt;
  res /= (vcc - volt);

  const int16_t *tab = (int16_t *)pgm_read_word(&ch->restab);

  int16_t start = (int16_t)pgm_read_word(tab++);
  int16_t step = (int16_t)pgm_read_word(tab++);

  tab++;
  uint8_t i = 1;
  while ((int16_t)pgm_read_word(tab) < res && (int16_t)pgm_read_word(tab + 1) != 0) {
    i++;
    tab++;
  }

  int32_t corr = res;
  corr -= (int16_t)pgm_read_word(tab);
  corr *= step;
  corr /= ((int16_t)pgm_read_word(tab) - (int16_t)pgm_read_word(tab - 1));

  return start + (step * i) + corr;
}


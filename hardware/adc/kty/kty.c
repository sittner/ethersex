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

// old size: 44314
// new size: 44440


#include <avr/io.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "core/eeprom.h"
#include "hardware/adc/temp2text.h"

#include "kty.h"

//#define KTY_REF_RES_VCC 2700
#define KTY_REF_RES_VCC 3300
#define KTY_REF_RES_GND 2200
#define KTY_REF_CHAN    5

#define KTY_RES_VCC 2200

#define KTY_81_TEMP_START -600
#define KTY_81_TEMP_STEP  10
#define KTY_81_1X_RES_VCC 2700
#define KTY_81_2X_RES_VCC 4700

#define KTY_83_TEMP_START -600
#define KTY_83_TEMP_STEP  10
#define KTY_83_RES_VCC    3000

#define KTY_84_TEMP_START -400
#define KTY_84_TEMP_STEP  10
#define KTY_84_RES_VCC    3000


const uint16_t kty81_110_120_150_restab[] = {
  465, 515, 567, 624, 684, 747, 815, 886, 961, 1040,
  1122, 1209, 1299, 1392, 1490, 1591, 1696, 1805, 1915, 2023,
  2124, 2211, 0
};

const uint16_t kty81_121_restab[] = {
  460, 510, 562, 617, 677, 740, 807, 877, 951, 1029,
  1111, 1196, 1286, 1378, 1475, 1575, 1679, 1786, 1896, 2003,
  2103, 2189, 0
};

const uint16_t kty81_122_restab[] = {
  470, 520, 573, 630, 690, 755, 823, 895, 971, 1050,
  1134, 1221, 1312, 1406, 1505, 1607, 1713, 1823, 1934, 2044,
  2146, 2233, 0
};

const uint16_t kty81_210_220_250_restab[] = {
  930, 1030, 1135, 1247, 1367, 1495, 1630, 1772, 1922, 2080,
  2245, 2417, 2597, 2785, 2980, 3182, 3392, 3607, 3817, 4008,
  4166, 4280, 0
};

const uint16_t kty81_221_restab[] = {
  921, 1019, 1123, 1235, 1354, 1480, 1613, 1754, 1903, 2059,
  2222, 2393, 2571, 2757, 2950, 3150, 3358, 3571, 3779, 3967,
  4125, 4237, 0
};

const uint16_t kty81_222_restab[] = {
  940, 1040, 1146, 1260, 1381, 1510, 1646, 1790, 1941, 2100,
  2267, 2441, 2623, 2812, 3009, 3214, 3426, 3643, 3855, 4048,
  4208, 4323, 0
};

const uint16_t kty83_110_120_150_restab[] = {
  475, 525, 577, 632, 691, 754, 820, 889, 962, 1039,
  1118, 1202, 1288, 1379, 1472, 1569, 1670, 1774, 1882, 1993,
  2107, 2225, 2346, 2471, 2599, 0
};

const uint16_t kty83_121_restab[] = {
  471, 519, 571, 626, 685, 746, 812, 880, 953, 1028,
  1107, 1190, 1276, 1365, 1458, 1554, 1653, 1756, 1863, 1973,
  2086, 2203, 2323, 2446, 2572, 0
};

const uint16_t kty83_122_restab[] = {
  480, 530, 583, 639, 698, 762, 828, 898, 972, 1049,
  1130, 1214, 1301, 1392, 1487, 1585, 1687, 1792, 1900, 2012,
  2128, 2247, 2370, 2496, 2624, 0
};

const uint16_t kty83_151_restab[] = {
  462, 512, 562, 617, 674, 735, 799, 867, 938, 1013,
  1090, 1172, 1256, 1344, 1435, 1530, 1628, 1730, 1835, 1943,
  2054, 2169, 2288, 2409, 2533, 0
};

const uint16_t kty84_130_150_restab[] = {
  359, 391, 424, 460, 498, 538, 581, 626, 672, 722,
  773, 826, 882, 940, 1000, 1062, 1127, 1194, 1262, 1334,
  1407, 1482, 1560, 1640, 1722, 1807, 1893, 1982, 2073, 2166,
  2261, 2357, 2452, 2542, 2624, 0
};

const uint16_t kty84_151_restab[] = {
  350, 381, 414, 449, 486, 525, 566, 610, 656, 704,
  754, 806, 860, 916, 975, 1036, 1099, 1164, 1231, 1300,
  1372, 1445, 1521, 1599, 1679, 1761, 1846, 1932, 2021, 2112,
  2205, 2298, 2391, 2479, 2558, 0
};

int16_t kty_res_to_centideg(const uint16_t *tab, int16_t start, int16_t step, uint16_t res) {

  uint8_t i = 1;
  while (tab[i] < res && tab[i+1] != 0) {
    i++;
  }

  int32_t corr = res;
  corr -= tab[i];
  corr *= step;
  corr /= (tab[i] - tab[i-1]);

  return start + (step * i) + corr;
}

int8_t
kty_calibrate(uint8_t channel)
{
  int32_t volt = adc_get(channel);
  int8_t calibration;
  volt *= 2500;
  volt /= 1023;
  int32_t R = 1000L;
  R *= 5000L - volt;
  R /= volt;
  if (R < 2320 && R > 2080){
    calibration = 2200L - R;
    eeprom_save_char (kty_calibration, calibration);
    eeprom_update_chksum();
    return 1;
  }
  return 0;
}

/* Berechnet die Temperatur in Zehntelgrad
 * vom adc wert
 */
int16_t
kty_get(uint8_t channel)
{
  uint32_t vref = adc_get_voltage(KTY_REF_CHAN);
  vref *= (KTY_REF_RES_VCC + KTY_REF_RES_GND);
  vref /= KTY_REF_RES_GND;


  uint16_t volt = adc_get_voltage(channel);

  uint32_t res = KTY_RES_VCC;
  res *= volt;
  res /= (vref - volt);

  return kty_res_to_centideg(kty84_130_150_restab, -400, 100, res);
}


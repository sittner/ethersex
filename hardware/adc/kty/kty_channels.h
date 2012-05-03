#ifndef ADC_KTY_CHANNELS_H
#define ADC_KTY_CHANNELS_H

//#define KTY_REF_RES_VCC 2700

#define KTY_81_1X_RES_VCC 2700
#define KTY_81_2X_RES_VCC 4700

#define KTY_83_RES_VCC    3000

//#define KTY_84_RES_VCC    3000
#define KTY_84_RES_VCC    2200

#define KTY_81_110_120_150_SUPPORT 1
//#define KTY_81_121_SUPPORT 1
//#define KTY_81_122_SUPPORT 1
//#define KTY_81_210_210_220_250_SUPPORT 1
//#define KTY_81_221_SUPPORT 1
//#define KTY_81_222_SUPPORT 1
//#define KTY_83_110_110_120_150_SUPPORT 1
//#define KTY_83_121_SUPPORT 1
//#define KTY_83_122_SUPPORT 1
//#define KTY_83_151_SUPPORT 1
#define KTY_84_130_150_SUPPORT 1
//#define KTY_84_151_SUPPORT 1

/*

#define KTY_CHANNEL_COUNT 8

#define KTY_CHANNEL_DEFS \
  { 0, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 1, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 2, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 3, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 4, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 5, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 6, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
  { 7, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \
*/

#define KTY_CHANNEL_COUNT 2

#define KTY_CHANNEL_DEFS \
  { 4, KTY_84_RES_VCC, kty84_130_150_restab }, \
  { 0, KTY_81_1X_RES_VCC, kty81_110_120_150_restab }, \

#endif

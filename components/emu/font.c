#include "font.h"
//DMD font ripped from PF executable
const uint8_t pf_dmd_font[]={
  0x7c, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe,
  0x7c, 0x38, 0x78, 0x78, 0x78, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0xfe,
  0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0x0e, 0x0e, 0x7e, 0xfe, 0xfc, 0xe0, 0xe0,
  0xfe, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0x0e, 0x0e, 0x7e, 0x7c, 0x7e, 0x0e,
  0x0e, 0xfe, 0xfe, 0xfc, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0xfe,
  0x0e, 0x0e, 0x0e, 0x0e, 0x0e, 0xfe, 0xfe, 0xfe, 0xe0, 0xe0, 0xfc, 0xfe,
  0xfe, 0x0e, 0x0e, 0xfe, 0xfe, 0xfc, 0x7e, 0xfe, 0xfe, 0xe0, 0xe0, 0xfc,
  0xfe, 0xfe, 0xee, 0xee, 0xfe, 0xfe, 0x7c, 0xfe, 0xfe, 0xfe, 0x0e, 0x0e,
  0x1c, 0x1c, 0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0x7c, 0xfe, 0xfe, 0xee,
  0xee, 0xfe, 0x7c, 0xfe, 0xee, 0xee, 0xfe, 0xfe, 0x7c, 0x7c, 0xfe, 0xfe,
  0xee, 0xee, 0xfe, 0xfe, 0x7e, 0x0e, 0x0e, 0xfe, 0xfe, 0xfc, 0x7c, 0xfe,
  0xfe, 0xee, 0xee, 0xfe, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfc,
  0xfe, 0xfe, 0xee, 0xee, 0xfe, 0xfc, 0xfe, 0xee, 0xee, 0xfe, 0xfe, 0xfc,
  0x7c, 0xfe, 0xfe, 0xee, 0xee, 0xe0, 0xe0, 0xe0, 0xee, 0xee, 0xfe, 0xfe,
  0x7c, 0xfc, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe,
  0xfe, 0xfc, 0xfe, 0xfe, 0xfe, 0xe0, 0xe0, 0xfc, 0xfc, 0xfc, 0xe0, 0xe0,
  0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xe0, 0xe0, 0xfc, 0xfc, 0xfc, 0xe0,
  0xe0, 0xe0, 0xe0, 0xe0, 0x7c, 0xfe, 0xfe, 0xee, 0xee, 0xe0, 0xee, 0xee,
  0xe6, 0xe6, 0xfe, 0xfe, 0x7e, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe,
  0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0xfe, 0x38, 0x38, 0x38,
  0x38, 0x38, 0x38, 0x38, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x0e, 0x0e,
  0x0e, 0x0e, 0x0e, 0xee, 0xee, 0xfe, 0xfe, 0x7c, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xfc, 0xf8, 0xfc, 0xee, 0xee, 0xee, 0xee, 0xee, 0xe0, 0xe0, 0xe0,
  0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xfe, 0xfe, 0xfe, 0x82, 0xc6,
  0xee, 0xfe, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfc,
  0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
  0x7c, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe,
  0x7c, 0xfc, 0xfe, 0xfe, 0xee, 0xee, 0xfe, 0xfe, 0xfc, 0xe0, 0xe0, 0xe0,
  0xe0, 0xe0, 0x7c, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe,
  0xfe, 0xfe, 0x76, 0xfc, 0xfe, 0xfe, 0xee, 0xee, 0xfe, 0xfc, 0xfe, 0xee,
  0xee, 0xee, 0xee, 0xee, 0x7e, 0xfe, 0xfe, 0xe0, 0xe0, 0xfc, 0xfe, 0x7e,
  0x0e, 0x0e, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0xfe, 0x38, 0x38, 0x38, 0x38,
  0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0x7c, 0xee, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xee, 0xee, 0xfe, 0xfc, 0xf8, 0xf0, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xfe, 0xfe, 0xfe, 0xfe, 0xee, 0xc6, 0x82, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xfe, 0x7c, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xee, 0xfe, 0xfe, 0x7e, 0x0e, 0x0e, 0xfe, 0xfe, 0xfc, 0xfe,
  0xfe, 0xfe, 0x0e, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xe0, 0xfe, 0xfe, 0xfe,
  0x7c, 0xfe, 0xfe, 0xee, 0x0e, 0x1e, 0x3c, 0x38, 0x38, 0x00, 0x38, 0x38,
  0x38, 0x0e, 0x1c, 0x38, 0x70, 0x70, 0xe0, 0xe0, 0xe0, 0x70, 0x70, 0x38,
  0x1c, 0x0e, 0xe0, 0x70, 0x38, 0x1c, 0x1c, 0x0e, 0x0e, 0x0e, 0x1c, 0x1c,
  0x38, 0x70, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xfe, 0xfe, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x7c, 0xfe, 0xfe, 0xce, 0xde, 0xfe, 0xf6, 0xe6, 0xfe, 0xfe, 0x7c, 0x38,
  0x78, 0x78, 0x78, 0x38, 0x38, 0x38, 0x38, 0xfe, 0xfe, 0xfe, 0xfc, 0xfe,
  0xfe, 0x0e, 0x7e, 0xfe, 0xfc, 0xe0, 0xfe, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe,
  0x0e, 0x7e, 0x7c, 0x7e, 0x0e, 0xfe, 0xfe, 0xfc, 0xee, 0xee, 0xee, 0xee,
  0xfe, 0xfe, 0xfe, 0x0e, 0x0e, 0x0e, 0x0e, 0xfe, 0xfe, 0xfe, 0xe0, 0xfc,
  0xfe, 0xfe, 0x0e, 0xfe, 0xfe, 0xfc, 0x7e, 0xfe, 0xfe, 0xe0, 0xfc, 0xfe,
  0xfe, 0xee, 0xfe, 0xfe, 0x7c, 0xfe, 0xfe, 0xfe, 0x0e, 0x0e, 0x1c, 0x38,
  0x38, 0x38, 0x38, 0x38, 0x7c, 0xfe, 0xfe, 0xee, 0xfe, 0x7c, 0xfe, 0xee,
  0xfe, 0xfe, 0x7c, 0x7c, 0xfe, 0xfe, 0xee, 0xfe, 0xfe, 0x7e, 0x0e, 0xfe,
  0xfe, 0xfc, 0x7c, 0xfe, 0xfe, 0xee, 0xfe, 0xfe, 0xfe, 0xee, 0xee, 0xee,
  0xee, 0xfc, 0xfe, 0xfe, 0xee, 0xfe, 0xfc, 0xfe, 0xee, 0xfe, 0xfe, 0xfc,
  0x7c, 0xfe, 0xfe, 0xee, 0xe0, 0xe0, 0xe0, 0xee, 0xfe, 0xfe, 0x7c, 0xfc,
  0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe,
  0xfe, 0xe0, 0xfc, 0xfc, 0xfc, 0xe0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
  0xe0, 0xfc, 0xfc, 0xfc, 0xe0, 0xe0, 0xe0, 0xe0, 0x7c, 0xfe, 0xfe, 0xee,
  0xe0, 0xee, 0xee, 0xe6, 0xfe, 0xfe, 0x7e, 0xee, 0xee, 0xee, 0xee, 0xfe,
  0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0xfe, 0x38, 0x38, 0x38,
  0x38, 0x38, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x0e, 0x0e, 0x0e, 0x0e,
  0xee, 0xfe, 0xfe, 0x7c, 0xee, 0xee, 0xee, 0xee, 0xfc, 0xf8, 0xfc, 0xee,
  0xee, 0xee, 0xee, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xfe,
  0xfe, 0xfe, 0x82, 0xc6, 0xee, 0xfe, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xfc, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
  0x7c, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0x7c, 0xfc,
  0xfe, 0xfe, 0xee, 0xfe, 0xfe, 0xfc, 0xe0, 0xe0, 0xe0, 0xe0, 0x7c, 0xfe,
  0xfe, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0xfc, 0x76, 0xfc, 0xfe, 0xfe,
  0xee, 0xfe, 0xfc, 0xfe, 0xee, 0xee, 0xee, 0xee, 0x7e, 0xfe, 0xfe, 0xe0,
  0xfc, 0xfe, 0x7e, 0x0e, 0xfe, 0xfe, 0xfc, 0xfe, 0xfe, 0xfe, 0x38, 0x38,
  0x38, 0x38, 0x38, 0x38, 0x38, 0x38, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xfe, 0xfe, 0x7c, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
  0xfe, 0xfc, 0xf8, 0xf0, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0xfe,
  0xee, 0xc6, 0x82, 0xee, 0xee, 0xee, 0xee, 0xfe, 0x7c, 0xfe, 0xee, 0xee,
  0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfe, 0x7e, 0x0e, 0xfe, 0xfe,
  0xfc, 0xfe, 0xfe, 0xfe, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0xfe, 0xfe, 0xfe,
  0x7c, 0xfe, 0xfe, 0xce, 0x1e, 0x3c, 0x38, 0x00, 0x38, 0x38, 0x38, 0x0e,
  0x1c, 0x38, 0x70, 0xe0, 0xe0, 0xe0, 0x70, 0x38, 0x1c, 0x0e, 0xe0, 0x70,
  0x38, 0x1c, 0x0e, 0x0e, 0x0e, 0x1c, 0x38, 0x70, 0xe0, 0x00, 0x00, 0x00,
  0x00, 0xfe, 0xfe, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x7c, 0xfe, 0xce, 0xde, 0xf6, 0xe6, 0xfe, 0x7c,
  0x38, 0x78, 0x78, 0x38, 0x38, 0x38, 0x7c, 0x7c, 0xfc, 0xfe, 0x0e, 0x7e,
  0xfc, 0xe0, 0xfe, 0xfe, 0xfc, 0xfe, 0x0e, 0x7c, 0x7e, 0x0e, 0xfe, 0xfc,
  0xee, 0xee, 0xee, 0xfe, 0xfe, 0x0e, 0x0e, 0x0e, 0xfe, 0xfe, 0xe0, 0xfc,
  0xfe, 0x0e, 0xfe, 0xfc, 0x7e, 0xfe, 0xe0, 0xfc, 0xfe, 0xee, 0xfe, 0x7c,
  0xfe, 0xfe, 0x0e, 0x1c, 0x38, 0x38, 0x38, 0x38, 0x7c, 0xfe, 0xee, 0x7c,
  0xfe, 0xee, 0xfe, 0x7c, 0x7c, 0xfe, 0xee, 0xfe, 0x7e, 0x0e, 0xfe, 0xfc,
  0x7c, 0xfe, 0xee, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xfc, 0xfe, 0xee, 0xfc,
  0xfe, 0xee, 0xfe, 0xfc, 0x7c, 0xfe, 0xee, 0xe0, 0xe0, 0xee, 0xfe, 0x7c,
  0xfc, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xfe, 0xfc, 0xfe, 0xfe, 0xe0, 0xfc,
  0xfc, 0xe0, 0xfe, 0xfe, 0xfe, 0xfe, 0xe0, 0xfc, 0xfc, 0xe0, 0xe0, 0xe0,
  0x7e, 0xfe, 0xe0, 0xee, 0xee, 0xe6, 0xfe, 0x7e, 0xee, 0xee, 0xee, 0xfe,
  0xfe, 0xee, 0xee, 0xee, 0x7c, 0x7c, 0x38, 0x38, 0x38, 0x38, 0x7c, 0x7c,
  0xfe, 0xfe, 0x0e, 0x0e, 0x0e, 0xee, 0xfe, 0x7c, 0xee, 0xee, 0xee, 0xfc,
  0xfe, 0xee, 0xee, 0xee, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xfe, 0xfe,
  0x82, 0xc6, 0xee, 0xfe, 0xfe, 0xee, 0xee, 0xee, 0xfc, 0xfe, 0xee, 0xee,
  0xee, 0xee, 0xee, 0xee, 0x7c, 0xfe, 0xee, 0xee, 0xee, 0xee, 0xfe, 0x7c,
  0xfc, 0xfe, 0xee, 0xfe, 0xfc, 0xe0, 0xe0, 0xe0, 0x7c, 0xfe, 0xee, 0xee,
  0xee, 0xee, 0xfc, 0x76, 0xfc, 0xfe, 0xee, 0xfc, 0xfe, 0xee, 0xee, 0xee,
  0x7e, 0xfe, 0xe0, 0xfc, 0x7e, 0x0e, 0xfe, 0xfc, 0xfe, 0xfe, 0x38, 0x38,
  0x38, 0x38, 0x38, 0x38, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0x7c,
  0xee, 0xee, 0xee, 0x7c, 0x7c, 0x38, 0x38, 0x10, 0xee, 0xee, 0xee, 0xfe,
  0xfe, 0xee, 0xc6, 0x82, 0xee, 0xee, 0xee, 0x7c, 0xfe, 0xee, 0xee, 0xee,
  0xee, 0xee, 0xee, 0xfe, 0x7e, 0x0e, 0xfe, 0xfc, 0xfe, 0xfe, 0x1e, 0x3c,
  0x78, 0xf0, 0xfe, 0xfe, 0x7c, 0xfe, 0xce, 0x1c, 0x38, 0x00, 0x38, 0x38,
  0x0e, 0x1c, 0x38, 0x70, 0x70, 0x38, 0x1c, 0x0e, 0x70, 0x38, 0x1c, 0x0e,
  0x0e, 0x1c, 0x38, 0x70, 0x00, 0x00, 0x00, 0xfe, 0xfe, 0x00, 0x00, 0x00,
  0x7c, 0xce, 0xd6, 0xe6, 0x7c, 0x38, 0x78, 0x78, 0x38, 0x38, 0xfc, 0x0e,
  0x7c, 0xe0, 0xfe, 0xfc, 0x0e, 0x7c, 0x0e, 0xfc, 0xee, 0xee, 0xfe, 0x0e,
  0x0e, 0xfe, 0xe0, 0xfc, 0x0e, 0xfc, 0x7e, 0xe0, 0xfc, 0xee, 0x7c, 0xfe,
  0x0e, 0x1c, 0x38, 0x38, 0x7c, 0xee, 0x7c, 0xee, 0x7c, 0x7c, 0xee, 0x7e,
  0x0e, 0xfc, 0x7c, 0xee, 0xfe, 0xee, 0xee, 0xfc, 0xee, 0xfc, 0xee, 0xfc,
  0x7c, 0xee, 0xe0, 0xee, 0x7c, 0xfc, 0xee, 0xee, 0xee, 0xfc, 0xfe, 0xe0,
  0xfc, 0xe0, 0xfe, 0xfe, 0xe0, 0xfc, 0xe0, 0xe0, 0x7e, 0xe0, 0xee, 0xe6,
  0x7e, 0xee, 0xee, 0xfe, 0xee, 0xee, 0x7c, 0x38, 0x38, 0x38, 0x7c, 0x0e,
  0x0e, 0xee, 0xee, 0x7c, 0xee, 0xee, 0xfc, 0xee, 0xee, 0xe0, 0xe0, 0xe0,
  0xe0, 0xfe, 0xc6, 0xee, 0xfe, 0xee, 0xee, 0xce, 0xee, 0xfe, 0xee, 0xe6,
  0x7c, 0xee, 0xee, 0xee, 0x7c, 0xfc, 0xee, 0xfc, 0xe0, 0xe0, 0x7c, 0xee,
  0xee, 0xec, 0x76, 0xfc, 0xee, 0xfc, 0xee, 0xee, 0x7e, 0xe0, 0x7c, 0x0e,
  0xfc, 0xfe, 0x38, 0x38, 0x38, 0x38, 0xee, 0xee, 0xee, 0xee, 0x7c, 0xee,
  0xee, 0x6c, 0x38, 0x10, 0xee, 0xee, 0xfe, 0xee, 0xc6, 0xee, 0xee, 0x7c,
  0xee, 0xee, 0xee, 0xee, 0x7e, 0x0e, 0xfc, 0xfc, 0x3c, 0x78, 0xf0, 0xfc,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x38, 0x70, 0x38, 0x1c, 0x70, 0x38,
  0x1c, 0x38, 0x70, 0x00, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
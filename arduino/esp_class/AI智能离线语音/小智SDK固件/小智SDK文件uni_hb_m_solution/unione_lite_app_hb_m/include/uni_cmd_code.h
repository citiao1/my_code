#ifndef INC_UNI_CMD_CODE_H_
#define INC_UNI_CMD_CODE_H_

typedef struct {
  uni_u8      cmd_code; /* cmd code fro send base on SUCP */
  const char  *cmd_str; /* action string on UDP */;
} cmd_code_map_t;

const cmd_code_map_t g_cmd_code_arry[] = {
  {0x0, "wakeup_uni"},
  {0x1, "exitUni"},
  {0x2, "TurnOn"},
  {0x3, "TurnOff"},
  {0x4, "BrightnessUp"},
  {0x5, "BrightnessOff"},
  {0x6, "dakaifengshan"},
  {0x7, "guanbifengshan"},
  {0x8, "dakaiyinyue"},
  {0x9, "guanbiyinyue"},
  {0xa, "volumeUpUni"},
  {0xb, "volumeDownUni"},
  {0xc, "dakaikaiguan"},
  {0xd, "guanbikaiguan"},
  {0xe, "tiaoliang"},
  {0xf, "tiaoan"},
};

#endif

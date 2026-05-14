#ifndef INC_UNI_NLU_CONTENT_H_
#define INC_UNI_NLU_CONTENT_H_

typedef struct {
  uni_u32 key_word_hash_code; /* 存放识别词汇对应的hashcode */
  uni_u8  nlu_content_str_index; /* 存放nlu映射表中的索引，实现多个识别词汇可对应同一个nlu，暂支持256条，如果不够换u16 */
  char    *hash_collision_orginal_str /* 类似Java String equal，当hash发生碰撞时，赋值为识别词汇，否则设置为NULL */;
} uni_nlu_content_mapping_t;

enum {
  eCMD_wakeup_uni,
  eCMD_exitUni,
  eCMD_TurnOn,
  eCMD_TurnOff,
  eCMD_BrightnessUp,
  eCMD_BrightnessOff,
  eCMD_dakaifengshan,
  eCMD_guanbifengshan,
  eCMD_dakaiyinyue,
  eCMD_guanbiyinyue,
  eCMD_volumeUpUni,
  eCMD_volumeDownUni,
  eCMD_dakaikaiguan,
  eCMD_guanbikaiguan,
  eCMD_tiaoliang,
  eCMD_tiaoan,
};

const char* const g_nlu_content_str[] = {
[eCMD_wakeup_uni] = "{\"asr\":\"小美\",\"cmd\":\"wakeup_uni\",\"pcm\":\"[103]\"}",
[eCMD_exitUni] = "{\"asr\":\"退下\",\"cmd\":\"exitUni\",\"pcm\":\"[104]\"}",
[eCMD_TurnOn] = "{\"asr\":\"打开空调\",\"cmd\":\"TurnOn\",\"pcm\":\"[105]\"}",
[eCMD_TurnOff] = "{\"asr\":\"关闭空调\",\"cmd\":\"TurnOff\",\"pcm\":\"[106]\"}",
[eCMD_BrightnessUp] = "{\"asr\":\"打开模型灯光\",\"cmd\":\"BrightnessUp\",\"pcm\":\"[107]\"}",
[eCMD_BrightnessOff] = "{\"asr\":\"关闭模型灯光\",\"cmd\":\"BrightnessOff\",\"pcm\":\"[108]\"}",
[eCMD_dakaifengshan] = "{\"asr\":\"打开风扇\",\"cmd\":\"dakaifengshan\",\"pcm\":\"[109]\"}",
[eCMD_guanbifengshan] = "{\"asr\":\"关闭风扇\",\"cmd\":\"guanbifengshan\",\"pcm\":\"[110]\"}",
[eCMD_dakaiyinyue] = "{\"asr\":\"打开音乐\",\"cmd\":\"dakaiyinyue\",\"pcm\":\"[111]\"}",
[eCMD_guanbiyinyue] = "{\"asr\":\"关闭音乐\",\"cmd\":\"guanbiyinyue\",\"pcm\":\"[112]\"}",
[eCMD_volumeUpUni] = "{\"asr\":\"音量增大\",\"cmd\":\"volumeUpUni\",\"pcm\":\"[113]\"}",
[eCMD_volumeDownUni] = "{\"asr\":\"音量减小\",\"cmd\":\"volumeDownUni\",\"pcm\":\"[113]\"}",
[eCMD_dakaikaiguan] = "{\"asr\":\"打开开关\",\"cmd\":\"dakaikaiguan\",\"pcm\":\"[114]\"}",
[eCMD_guanbikaiguan] = "{\"asr\":\"关闭开关\",\"cmd\":\"guanbikaiguan\",\"pcm\":\"[113]\"}",
[eCMD_tiaoliang] = "{\"asr\":\"调亮一点\",\"cmd\":\"tiaoliang\",\"pcm\":\"[113]\"}",
[eCMD_tiaoan] = "{\"asr\":\"调暗一点\",\"cmd\":\"tiaoan\",\"pcm\":\"[113]\"}",
};

/*TODO perf sort by hashcode O(logN), now version O(N)*/
const uni_nlu_content_mapping_t g_nlu_content_mapping[] = {
  {2428136115U/*小美*/, eCMD_wakeup_uni, NULL},
  {2497873774U/*退下*/, eCMD_exitUni, NULL},
  {3484489468U/*打开空调*/, eCMD_TurnOn, NULL},
  {2543416495U/*关闭空调*/, eCMD_TurnOff, NULL},
  {420754400U/*打开模型灯光*/, eCMD_BrightnessUp, NULL},
  {4085137235U/*关闭模型灯光*/, eCMD_BrightnessOff, NULL},
  {3534892713U/*打开风扇*/, eCMD_dakaifengshan, NULL},
  {2593819740U/*关闭风扇*/, eCMD_guanbifengshan, NULL},
  {3532300471U/*打开音乐*/, eCMD_dakaiyinyue, NULL},
  {2591227498U/*关闭音乐*/, eCMD_guanbiyinyue, NULL},
  {548623739U/*音量增大*/, eCMD_volumeUpUni, NULL},
  {523242155U/*音量减小*/, eCMD_volumeDownUni, NULL},
  {3443046019U/*打开开关*/, eCMD_dakaikaiguan, NULL},
  {2501973046U/*关闭开关*/, eCMD_guanbikaiguan, NULL},
  {1648810031U/*调亮一点*/, eCMD_tiaoliang, NULL},
  {3455764346U/*调暗一点*/, eCMD_tiaoan, NULL},
};

#endif

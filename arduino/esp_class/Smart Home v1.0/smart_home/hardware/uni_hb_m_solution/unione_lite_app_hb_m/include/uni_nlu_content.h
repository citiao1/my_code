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
  eCMD_LED_ON,
  eCMD_LED_OFF,
  eCMD_FAN_ON,
  eCMD_FAN_OFF,
  eCMD_SERVO_ON,
  eCMD_SERVO_OFF,
  eCMD_CONDITION_ON,
  eCMD_CONDITION_OFF,
  eCMD_ALL_ON,
  eCMD_ALL_OFF,
};

const char* const g_nlu_content_str[] = {
[eCMD_wakeup_uni] = "{\"asr\":\"你好小冯\",\"cmd\":\"wakeup_uni\",\"pcm\":\"[103, 104]\"}",
[eCMD_exitUni] = "{\"asr\":\"退下\",\"cmd\":\"exitUni\",\"pcm\":\"[105]\"}",
[eCMD_LED_ON] = "{\"asr\":\"开灯\",\"cmd\":\"LED_ON\",\"pcm\":\"[106]\"}",
[eCMD_LED_OFF] = "{\"asr\":\"关灯\",\"cmd\":\"LED_OFF\",\"pcm\":\"[107]\"}",
[eCMD_FAN_ON] = "{\"asr\":\"打开风扇\",\"cmd\":\"FAN_ON\",\"pcm\":\"[108]\"}",
[eCMD_FAN_OFF] = "{\"asr\":\"关闭风扇\",\"cmd\":\"FAN_OFF\",\"pcm\":\"[109]\"}",
[eCMD_SERVO_ON] = "{\"asr\":\"打开窗帘\",\"cmd\":\"SERVO_ON\",\"pcm\":\"[110]\"}",
[eCMD_SERVO_OFF] = "{\"asr\":\"关闭窗帘\",\"cmd\":\"SERVO_OFF\",\"pcm\":\"[111]\"}",
[eCMD_CONDITION_ON] = "{\"asr\":\"打开空调\",\"cmd\":\"CONDITION_ON\",\"pcm\":\"[112]\"}",
[eCMD_CONDITION_OFF] = "{\"asr\":\"关闭空调\",\"cmd\":\"CONDITION_OFF\",\"pcm\":\"[113]\"}",
[eCMD_ALL_ON] = "{\"asr\":\"打开所有设备\",\"cmd\":\"ALL_ON\",\"pcm\":\"[114]\"}",
[eCMD_ALL_OFF] = "{\"asr\":\"关闭所有设备\",\"cmd\":\"ALL_OFF\",\"pcm\":\"[115]\"}",
};

/*TODO perf sort by hashcode O(logN), now version O(N)*/
const uni_nlu_content_mapping_t g_nlu_content_mapping[] = {
  {2835564448U/*你好小冯*/, eCMD_wakeup_uni, NULL},
  {2497873774U/*退下*/, eCMD_exitUni, NULL},
  {2438769644U/*开灯*/, eCMD_LED_ON, NULL},
  {2389495330U/*关灯*/, eCMD_LED_OFF, NULL},
  {3534892713U/*打开风扇*/, eCMD_FAN_ON, NULL},
  {2593819740U/*关闭风扇*/, eCMD_FAN_OFF, NULL},
  {3484367690U/*打开窗帘*/, eCMD_SERVO_ON, NULL},
  {2543294717U/*关闭窗帘*/, eCMD_SERVO_OFF, NULL},
  {3484489468U/*打开空调*/, eCMD_CONDITION_ON, NULL},
  {2543416495U/*关闭空调*/, eCMD_CONDITION_OFF, NULL},
  {4074137567U/*打开所有设备*/, eCMD_ALL_ON, NULL},
  {3443553106U/*关闭所有设备*/, eCMD_ALL_OFF, NULL},
};

#endif

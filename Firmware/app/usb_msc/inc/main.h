#ifndef MAIN__H
#define MAIN__H

#include "board_defines.h"

#ifndef MSC_STATUS_LED
  #define MSC_STATUS_LED GPIO_LED_0
#endif //MSC_STATUS_LED

#ifndef MSC_CARD_DETECT_LED
  #define MSC_CARD_DETECT_LED GPIO_LED_1
#endif //MSC_CARD_DETECT_LED

#endif //MAIN__H
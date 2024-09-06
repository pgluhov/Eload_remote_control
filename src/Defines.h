
#ifndef DEFINES_H
#define DEFINES_H
#include <Arduino.h>

// ========== ДЕФАЙНЫ НАСТРОЕК ==========
String DEVICE_NAME = "Eload remote LCD control"; // Имя девайса
String CURRENT_VERSION_SW = "1.00";     // Текущая версиия прошивки 
String VERSION_SW = "Версия ПО 1.00";   // Текст для отображения


#define ENABLE_DEBUG_UART      0  // Если 1 то отладка обмена по uart включена в SerialBT
#define ENABLE_DEBUG           0  // Если 1 то отладка включена в Serial
#define ENABLE_DEBUG_ENC       0  // Если 1 то отладка энкодера в Serial 
#define ENABLE_DEBUG_TASK      0
#define DEBUG_RESIVE_UART1     0
#define DEBUG_CALLIBROVKA      0

#define INIT_KEY      38     // ключ первого запуска. 0-254, на выбор

//-------------объявлять ДО ПОДКЛЮЧЕНИЯ БИБЛИОТЕКИ GyverPortal------------------
#define GP_NO_MDNS          // убрать поддержку mDNS из библиотеки (вход по хосту в браузере)
#define GP_NO_DNS           // убрать поддержку DNS из библиотеки (для режима работы как точка доступа)
//#define GP_NO_OTA         // убрать поддержку OTA обновления прошивки
#define GP_NO_UPLOAD        // убрать поддержку загрузки файлов на сервер
#define GP_NO_DOWNLOAD      // убрать поддержку скачивания файлов с сервера


//--------Настройка ШИМ-------------------
#define frequency     1000 // частота ШИМ (в Гц)
#define resolution    16   // разрешение ШИМа (в битах) 
#define pwmChannel_1  0    // канал, на который назначим в дальнейшем ШИМ 
#define pwmChannel_2  1    // канал, на который назначим в дальнейшем ШИМ 
#define pwmChannel_3  2    // канал, на который назначим в дальнейшем ШИМ 
#define pwmChannel_4  3    // канал, на который назначим в дальнейшем ШИМ 
#define pwmChannel_5  4    // канал, на который назначим в дальнейшем ШИМ
int pwmChannel[] = {pwmChannel_1, pwmChannel_2, pwmChannel_3, pwmChannel_4, pwmChannel_5};
 
//--------номера IO-------------------
#define PWM_Pin_1  25   
#define PWM_Pin_2  27
#define PWM_Pin_3  32
#define PWM_Pin_4  14
#define PWM_Pin_5  16

#define ONE_WIRE_BUS 17

#define BTN_HALL     33 
#define ENCODER_A    39 
#define ENCODER_B    36 

#define DETECT_ELOAD_1  34   
#define DETECT_ELOAD_2  35
#define DETECT_ELOAD_3  26
#define DETECT_ELOAD_4  13
#define DETECT_ELOAD_5  19

//--------номера IO-------------------

#endif //DEFINES_H
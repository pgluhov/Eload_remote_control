#include <Arduino.h>
#include "driver/uart.h"
#include "Defines.h"
#include "Base_table.h"

#define ELNUMBER_MAX 5  // Количество подключенных нагрузок максимально
#define ELNUMBER_MIN 1  // Количество подключенных нагрузок минимально
#define COUNT_PRESET 6  // Количество пресетов тока в каждом канале    
uint8_t ELNUMBER = 3;   // Количество подключенных нагрузок по умолчанию

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3; 
TaskHandle_t Task4; 
TaskHandle_t Task5; // резерв
TaskHandle_t Task6; 
TaskHandle_t Task7; 
TaskHandle_t Task8; 

void Task1code(void* pvParameters);
void Init_Task1();
void Task2code(void* pvParameters);
void Init_Task2();
void Task3code(void* pvParameters);
void Init_Task3();
void Task4code(void* pvParameters);
void Init_Task4();
void Task5code(void* pvParameters);
void Init_Task5();
void Task6code(void* pvParameters);
void Init_Task6();
void Task7code(void* pvParameters);
void Init_Task7();
void Task8code(void* pvParameters);
void Init_Task8();

byte crc8_bytes(byte *buffer, byte size);
void Set_current_chanal(float curr, int number);
void Set_pwm_chanal(uint32_t pwm, int number);
void SelectedPower();
void SelectedPort();
void SaveDotGraf();
void SaveAllConfig();

#pragma pack(push, 1) // используем принудительное выравнивание
struct Rx_buff{       // Структура приемник от клавиатуры   
  int Row;
  int Column;
  int RawBits;
  bool statPress;   
  int enc_step=0;
  int enc_stepH=0;
  int enc_click=0;
  int enc_held=0;
  byte crc;
};
#pragma pack(pop)
Rx_buff RxBuff;

#pragma pack(push, 1) // используем принудительное выравнивание
struct Tx_buff{       // Структура для отправки на клавиатуру
  int x;
  uint8_t y;
  byte crc;
};
#pragma pack(pop)
Tx_buff TxBuff;

QueueHandle_t QueueHandleUartResive; // Определить дескриптор очереди
const int QueueElementSizeUart = 10;
typedef struct{ 
  int activeRow;     // Номер строки
  int activeColumn;  // Номер столбца
  int statusColumn;  // Байт с битами всего столбца
  bool statPress;    // Статус нажата или отпущена кнопка
  int enc_step=0;
  int enc_stepH=0;
  int enc_click=0;
  int enc_held=0;
} message_uart_resive;

//--------------------------------------------------------------------------------------------

#define DWIDTH   320 // ширина экрана
#define DHEIGHT  240 // высота экрана
#include <TFT_eSPI.h>
TFT_eSPI  tft = TFT_eSPI();  // Экземпляр библиотеки
TFT_eSprite spr = TFT_eSprite(&tft);  
uint16_t* sprPtr;            // Указатели на запуск спрайтов в оперативной памяти (тогда это указатели на "изображение")
#include "FreeSerif10.h"
#include "FreeSerif16.h"
#define GFXFF 1
#define FONT &FreeSerif10pt8b
#define FONT2 &FreeSerif16pt8b
#define maxString 100 // ограничиваем строку шириной экрана
char target[maxString + 1] = "";
char *utf8rus(char *source);


//--------------------------------------------------------------------------------------------

#include <EEPROM.h>
struct valueEEprom {  // структура с переменными     
  int initKey; 
  float curr[ELNUMBER_MAX][COUNT_PRESET]; // Значения тока в каждом канале  
  int TermAdress[ELNUMBER_MAX][8];        // Адреса пяти датчиков подключенных к этому блоку
  char Text_This_Mac_Hex[40];             // Текст mac адреса этого устройства в HEX формате
  char Text_This_Mac_Dec[40];             // Текст mac адреса этого устройства в DEC формате 
  int elnumber;  
  char ssid[20];
  char pass[20];
  char locate[20];
  
  int EL_max_current[5];
  int EL_max_voltage[5];
  int EL_max_power[5];
  int EL_dot_count[5];
  uint16_t EL_table_1[50][2];
  uint16_t EL_table_2[50][2];
  uint16_t EL_table_3[50][2];
  uint16_t EL_table_4[50][2];
  uint16_t EL_table_5[50][2];
}; 
valueEEprom EE_VALUE;

//--------------------------------------------------------------------------------------------

struct eload_t {  // Структура для хранения параметров нагрузок
  uint8_t                device_id[8];    // имя нагрузки { 28 0C E6 25 0E 00 00 20 }   
  uint8_t         device_conn_counter;    // счетчик ошибок чтения ID нагрузки 
  uint16_t        device_table[50][2];    // таблица калибровочная [ток mA] [ PWM ]
  uint8_t           device_table_size;    // размер массива
  uint8_t          device_max_current;    // максимальный ток
  uint8_t          device_max_voltage;    // максимальное напряжение
  uint8_t            device_max_power;    // максимальная мощность на канал   

  uint16_t   curr_pos_x[COUNT_PRESET];    // x координата центра конкретного пресета тока
  uint16_t   curr_pos_y[COUNT_PRESET];    // y координата центра конкретного пресета тока  
  float curr_val_preset[COUNT_PRESET];    // значение персетов тока (в адресе [0]=0.00A)  
  uint8_t  curr_decimal[COUNT_PRESET];    // количаство знаков после запятой для каждого значения пресета 
  uint16_t     curr_col[COUNT_PRESET];    // цвет конкретного пресета тока  
  
  uint16_t                tempC_pos_x;    // x координата центра температуры 
  uint16_t                tempC_pos_y;    // y координата центра температуры 
  float                     tempC_val;    // значение температуры 
  uint8_t               tempC_decimal;    // количаство знаков после запятой 
  uint16_t                  tempC_col;    // цвет температуры   
  }; 
eload_t eload[ELNUMBER_MAX];  // Создайте структуру и получите указатель на нее

int active_preset = 0;   // Активная позиция пресета
int active_eload = 0;    // Активная нагрузка
int setting_eload = 0;   // уровень входа в режим изменения тока (подменю)
const char* text_change_step = "change step";
const char* text_step_1000 = "1000 mA";
const char* text_step_100 = "100 mA";
const char* text_step_10 = "10 mA";
const char* text_step_1 = "1 mA";
int decimal_set_eload = 0; // шаг изменения тока (0)1,0А - (1)0,1А - (2)0,01А - (3)0,001А
float decimal_val_eload[] = {1.0, 0.1, 0.01, 0.001}; // массив значений для изменения тока
int decimal_arr_size = sizeof(decimal_val_eload) / sizeof (decimal_val_eload[0]);
bool F_show_lcd_change_step = 0;
bool F_first_show = 1;
int counter_show_off = 1;

struct call_eload_t {  // Структура для хранения параметров нагрузи при калибровке  
  int       arr_call_position = 0;  // номер строки в массиве [ток mA] [ PWM ]
  int      port_call_position = 0;  // номер порта нагрузки
  int          active_pos_enc = 0;  // значение энкодера 
  int          pwm_val_enc[4] = {1, 10, 100, 1000}; //значение шим на шаг энкодера
  int            pwm_arr_size = sizeof(pwm_val_enc) / sizeof (pwm_val_enc[0]); // размер массива pwm_val_enc[]
  int       step_callibration = 0;  // шаг этапа калибровки 0-выбор порта, 1-выбор мощности, 2-настройка тока, 3-сохранение
  uint16_t   pwm_chanal_value = 0;  // значение шим для отправки нагрузке
}; 
call_eload_t call_eload;  // Создайте структуру и получите указатель на нее

int Enc_step  = 0;
int Enc_stepH = 0;
int Enc_click = 0;
int Enc_held  = 0;

//--------------------------------------------------------------------------------------------

#include <OneWire.h>
#include <DallasTemperature.h>
#define TEMPERATURE_PRECISION 9 // Разрешение 9-12 бит
#define TEMPERATURE_MAX 60      // Максимальная температура
#define CONN_COUNT_MAX_ERRRR 5  // Максимальное количество ошибок чтения температуры
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int numberberOfDevices; // Количество найденных температурных устройств

uint8_t Thermometer_0[8]  = { 40,237,19,38,14,0,0,194}; 
uint8_t Thermometer_1[8]  = { 40,174,0,38,14,0,0,220}; 
uint8_t Thermometer_2[8]  = { 40,215,83,38,14,0,0,9}; 
uint8_t Thermometer_3[8]  = { 40,59,161,38,14,0,0,120}; 
uint8_t Thermometer_4[8]  = { 40,61,186,39,14,0,0,73}; 
uint8_t ThermometerArr[ELNUMBER_MAX][8];

//--------------------------------------------------------------------------------------------

#define EB_FAST 60     // таймаут быстрого поворота, мс
#define EB_DEB 40      // дебаунс кнопки, мс
#define EB_CLICK 200   // таймаут накликивания, мс
#include <EncButton2.h>
EncButton2<EB_ENCBTN> enc(INPUT, ENCODER_A, ENCODER_B, BTN_HALL);  // энкодер с кнопкой
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

//--------------------------------------------------------------------------------------------

void IRAM_ATTR onTimer(){ // Опрос энкодера
  portENTER_CRITICAL_ISR(&timerMux);
  enc.tick();    // опрос энкодера  
  portEXIT_CRITICAL_ISR(&timerMux);
}

void Task1code(void* pvParameters) {  // Обработка принятых данных от клавиатуры
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task1code running on core ");
  Serial.println(xPortGetCoreID()); 
  #endif  

  message_uart_resive message;

  for (;;) {     
    
    if(QueueHandleUartResive != NULL){ // Проверка работоспособности просто для того, чтобы убедиться, что очередь действительно существует
      int ret = xQueueReceive(QueueHandleUartResive, &message, portMAX_DELAY);
      if(ret == pdPASS){ 
        #if (ENABLE_DEBUG_UART == 1)
        SerialBT.println("Task1 получены данные от  serialEvent" );         
        SerialBT.print("Task1 активная строка: " );
        SerialBT.println(message.activeRow); 
        SerialBT.print("Task1 активный столбец: " );
        SerialBT.println(message.activeColumn);  
        SerialBT.print("Task1 статус нажатия: " );
        SerialBT.println(message.statPress);
         
        SerialBT.print("enc_step: " );
        SerialBT.println(message.enc_step);
        SerialBT.print("enc_click: " );
        SerialBT.println(message.enc_click);
        SerialBT.print("enc_held: " );
        SerialBT.println(message.enc_held);         
        SerialBT.println(); 
        #endif 

        Enc_step  = message.enc_step;
        Enc_stepH = message.enc_stepH;
        Enc_click = message.enc_click;
        Enc_held  = message.enc_held; 
        //active_eload = message.activeColumn; 
        //active_preset = message.activeRow; 
        
        if(message.activeRow != -1){
          for(int col=0; col<COUNT_PRESET; col++){eload[message.activeColumn].curr_col[col] = TFT_PURPLE;}  // цвет тока всех изменить на TFT_LIGHTGREY        
          eload[message.activeColumn].curr_col[message.activeRow] = TFT_GREEN;
          active_eload = message.activeColumn; 
          active_preset = message.activeRow;
          Set_current_chanal(eload[active_eload].curr_val_preset[active_preset], active_eload);
          F_show_lcd_change_step = 0;
          }
        }
      }  
   }
}

void Init_Task1() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task1code, /* Функция задачи. */
    "Task1",   /* Ее имя. */
    4096,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task1,    /* Дескриптор задачи для отслеживания */
    1);        /* Указываем пин для данного ядра */
  delay(500);
}

void Task2code(void* pvParameters) {  // Функции энкодера
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task2code running on core ");
  Serial.println(xPortGetCoreID());
  #endif  

  for (;;) {
    
  // =============== ЭНКОДЕР ===============

   if (enc.left() || Enc_step<0)  { // поворот налево
    Enc_step=0;    
    eload[active_eload].curr_val_preset[active_preset] = eload[active_eload].curr_val_preset[active_preset] - decimal_val_eload[decimal_set_eload];
      if(eload[active_eload].curr_val_preset[active_preset] < 0){eload[active_eload].curr_val_preset[active_preset] = 0;}
      if(eload[active_eload].curr_val_preset[active_preset] >= 10){eload[active_eload].curr_decimal[active_preset] = 2;}
      if(eload[active_eload].curr_val_preset[active_preset] < 10){eload[active_eload].curr_decimal[active_preset] = 3;}
      Set_current_chanal(eload[active_eload].curr_val_preset[active_preset], active_eload);
      F_show_lcd_change_step = 0;
      F_first_show = 1;
      }
   if (enc.right()|| Enc_step>0) { // поворот направо 
    Enc_step=0;     
    eload[active_eload].curr_val_preset[active_preset] = eload[active_eload].curr_val_preset[active_preset] + decimal_val_eload[decimal_set_eload];     
      if(eload[active_eload].curr_val_preset[active_preset] > eload[active_eload].device_max_current){eload[active_eload].curr_val_preset[active_preset] = eload[active_eload].device_max_current;}      
      if(eload[active_eload].curr_val_preset[active_preset] >= 10){eload[active_eload].curr_decimal[active_preset] = 2;}      
      if(eload[active_eload].curr_val_preset[active_preset] < 10){eload[active_eload].curr_decimal[active_preset] = 3;} 
      Set_current_chanal(eload[active_eload].curr_val_preset[active_preset], active_eload);
      F_show_lcd_change_step = 0;
      F_first_show = 1;
      }
   if (enc.click()|| Enc_click==1){
    Enc_click=0;
    if (F_first_show == 0){ // Если первое нажатие на энкодер - просто показать текущий шаг настройки
      decimal_set_eload++;
      if(decimal_set_eload == decimal_arr_size){decimal_set_eload = 0;}
      F_show_lcd_change_step = 1;
      counter_show_off = 2000;
      } 
    if (F_first_show == 1){ // Если второе нажатие на энкодер - изменить шаг настройки
      F_first_show = 0; 
      F_show_lcd_change_step = 1;
      counter_show_off = 2000;      
      } 
    }  
   if (enc.held() || Enc_held==1){
    Enc_held=0;   
    if (active_preset == 3){ // Если выбран максимальный ток модуля, то расчить все значения пресетов  
      float current_100 = eload[active_eload].curr_val_preset[active_preset];
      eload[active_eload].curr_val_preset[0] = 0;   
      eload[active_eload].curr_decimal[0] = 3;   

      eload[active_eload].curr_val_preset[1] = current_100*0.10; 
      if (eload[active_eload].curr_val_preset[1] <  10 ){eload[active_eload].curr_decimal[1] = 3;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[1] >= 10 ){eload[active_eload].curr_decimal[1] = 2;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[1] >= 100){eload[active_eload].curr_decimal[1] = 1;} // количаство знаков после запятой 

      eload[active_eload].curr_val_preset[2] = current_100*0.55;
      if (eload[active_eload].curr_val_preset[2] <  10 ){eload[active_eload].curr_decimal[2] = 3;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[2] >= 10 ){eload[active_eload].curr_decimal[2] = 2;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[2] >= 100){eload[active_eload].curr_decimal[2] = 1;} // количаство знаков после запятой 

      eload[active_eload].curr_val_preset[4] = current_100*1.10;
      if (eload[active_eload].curr_val_preset[4] > eload[active_eload].device_max_current){eload[active_eload].curr_val_preset[4] = eload[active_eload].device_max_current;}
      if (eload[active_eload].curr_val_preset[4] <  10 ){eload[active_eload].curr_decimal[4] = 3;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[4] >= 10 ){eload[active_eload].curr_decimal[4] = 2;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[4] >= 100){eload[active_eload].curr_decimal[4] = 1;} // количаство знаков после запятой 

      eload[active_eload].curr_val_preset[5] = current_100*1.40;
      if (eload[active_eload].curr_val_preset[5] > eload[active_eload].device_max_current){eload[active_eload].curr_val_preset[5] = eload[active_eload].device_max_current;}      
      if (eload[active_eload].curr_val_preset[5] <  10 ){eload[active_eload].curr_decimal[5] = 3;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[5] >= 10 ){eload[active_eload].curr_decimal[5] = 2;} // количаство знаков после запятой 
      if (eload[active_eload].curr_val_preset[5] >= 100){eload[active_eload].curr_decimal[5] = 1;} // количаство знаков после запятой 

      }                                                                                                                                                                        
    } 
    
          
  #if (ENABLE_DEBUG_ENC == 1)  
  if (enc.left()) Serial.println("left");     // поворот налево
  if (enc.right()) Serial.println("right");   // поворот направо
  if (enc.leftH()) Serial.println("leftH");   // нажатый поворот налево
  if (enc.rightH()) Serial.println("rightH"); // нажатый поворот направо
  #endif

  // =============== КНОПКА ===============
  
  #if (ENABLE_DEBUG_ENC == 1)
  if (enc.press()) Serial.println("press");
  if (enc.click()) Serial.println("click");
  if (enc.release()) Serial.println("release"); 
  if (enc.held()) Serial.println("held");      // однократно вернёт true при удержании
  #endif
 
   enc.resetState();     
   vTaskDelay(30);    
  }
}

void Init_Task2() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task2code, /* Функция задачи. */
    "Task2",   /* Ее имя. */
    4096,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task2,    /* Дескриптор задачи для отслеживания */
    0);        /* Указываем пин для данного ядра */
  delay(500);
}

void Task3code(void* pvParameters) {  // Работа LCD
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task3code running on core ");
  Serial.println(xPortGetCoreID()); 
  #endif 
  int sprite_select = 0;
  int sprite_offset = 0;
  int sprite_draw = 0;

  for (;;) {    
  
  switch (sprite_select){  
    case 0: sprite_offset =  0;   sprite_draw =  0;  break;  
    case 1: sprite_offset = -60;  sprite_draw = 60;  break;
    case 2: sprite_offset = -120; sprite_draw = 120; break;
    case 3: sprite_offset = -180; sprite_draw = 180; break;
    case 4: sprite_offset = 0; sprite_draw = 0; sprite_select = 0; break;
  }  
  
  sprPtr = (uint16_t*)spr.createSprite(DWIDTH, 60);
  if(sprite_select != 0){spr.setViewport(0, sprite_offset, DWIDTH, DHEIGHT);}
  spr.setTextDatum(MC_DATUM);  
  
  /*------------Сетка-----------------------------*/
  spr.fillSprite(TFT_BLACK); // Очистка экрана   
  spr.drawFastHLine(0, 0,   DWIDTH, TFT_SILVER);    
  spr.drawFastHLine(0, 40,  DWIDTH, TFT_SILVER);  
  spr.drawFastHLine(0, 79,  DWIDTH, TFT_SILVER);  
  spr.drawFastHLine(0, 118, DWIDTH, TFT_SILVER);   
  spr.drawFastHLine(0, 157, DWIDTH, TFT_SILVER);   
  spr.drawFastHLine(0, 196, DWIDTH, TFT_SILVER);
  spr.drawFastHLine(0, 235, DWIDTH, TFT_SILVER);   
  spr.drawFastVLine(0, 0, 235, TFT_SILVER);  
  spr.drawFastVLine(106, 0, 235, TFT_SILVER);  
  spr.drawFastVLine(212, 0, 235, TFT_SILVER);
  spr.drawFastVLine(319, 0, 235, TFT_SILVER);

/*--------------Отображение тока--------------------------*/
  for (int i = 0; i < COUNT_PRESET; i++) {   
    spr.setTextColor(eload[0].curr_col[i], TFT_BLACK);
    spr.drawFloat(eload[0].curr_val_preset[i], eload[0].curr_decimal[i], eload[0].curr_pos_x[i], eload[0].curr_pos_y[i], 4);
    }
  for (int i = 0; i < COUNT_PRESET; i++) {   
    spr.setTextColor(eload[1].curr_col[i], TFT_BLACK);
    spr.drawFloat(eload[1].curr_val_preset[i], eload[1].curr_decimal[i], eload[1].curr_pos_x[i], eload[1].curr_pos_y[i], 4);
    }
  for (int i = 0; i < COUNT_PRESET; i++) {   
    spr.setTextColor(eload[2].curr_col[i], TFT_BLACK);
    spr.drawFloat(eload[2].curr_val_preset[i], eload[2].curr_decimal[i], eload[2].curr_pos_x[i], eload[2].curr_pos_y[i], 4);
    }     

/*--------------Шаг регулировки тока-----------------------*/
  if ((active_preset==0 || active_preset==1 || active_preset==4 || active_preset==5) && F_show_lcd_change_step==1){
    spr.fillRect(90, 91, 140, 59, TFT_BLACK);
    spr.fillRect(93, 93, 134, 54, TFT_WHITE);
    spr.fillRect(95, 95, 130, 50, TFT_BLACK);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(text_change_step, 160, 110, 2);    
    if(decimal_set_eload==0){spr.drawString(text_step_1000, 160, 130, 2);} 
    if(decimal_set_eload==1){spr.drawString(text_step_100, 160, 130, 2);}
    if(decimal_set_eload==2){spr.drawString(text_step_10, 160, 130, 2);} 
    if(decimal_set_eload==3){spr.drawString(text_step_1, 160, 130, 2);}
  }

  if ((active_preset==2 || active_preset==3) && F_show_lcd_change_step==1){
    spr.fillRect(90, 11, 140, 59, TFT_BLACK);
    spr.fillRect(93, 13, 134, 54, TFT_WHITE);
    spr.fillRect(95, 15, 130, 50, TFT_BLACK);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.drawString(text_change_step, 160, 30, 2);    
    if(decimal_set_eload==0){spr.drawString(text_step_1000, 160, 50, 2);} 
    if(decimal_set_eload==1){spr.drawString(text_step_100, 160, 50, 2);}
    if(decimal_set_eload==2){spr.drawString(text_step_10, 160, 50, 2);}
    if(decimal_set_eload==3){spr.drawString(text_step_1, 160, 50, 2);}  
  }

  
  tft.pushImageDMA(0, sprite_draw, 320, 60, sprPtr);
  spr.deleteSprite();  // Delete the sprite to free up the RAM
  sprite_select ++;
  vTaskDelay(20);
  }
}

void Init_Task3() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task3code, /* Функция задачи. */
    "Task3",   /* Ее имя. */
    4096,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task3,    /* Дескриптор задачи для отслеживания */
    1);        /* Указываем пин для данного ядра */
  delay(500);
}

void Task4code(void* pvParameters) {  // Выключение отображения окна с шагом настройки
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task4code running on core ");
  Serial.println(xPortGetCoreID()); 
  #endif

  for (;;) {
    if(F_show_lcd_change_step == 1){
      counter_show_off -= 100;
      if(counter_show_off == 0){F_show_lcd_change_step = 0; F_first_show = 1;}
      }
   vTaskDelay(100);   
  }
}

void Init_Task4() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task4code, /* Функция задачи. */
    "Task4",   /* Ее имя. */
    1024,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task4,    /* Дескриптор задачи для отслеживания */
    0);        /* Указываем пин для данного ядра */
  //delay(500);
}

void Task6code(void* pvParameters) {  // Работа LCD (терминал)
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task6code running on core ");
  Serial.println(xPortGetCoreID()); 
  #endif 
  int sprite_select = 0;
  int sprite_offset = 0;
  int sprite_draw = 0;

  char text_1[] = "Установите ток";   
  char text_2[10]; 
  char text_3[40]; 
  char text_4[] = "Выберите мощность нагрузки"; 
  char text_5[] = "Выберите порт подключения"; 
  char text_6[40]; 
  char text_7[] = "Сохранение параметров"; 
  char text_8[] = "Ожидайте перезагрузки"; 
  char text_9[] = "#####################"; 

  for (;;) {    
  
  switch (sprite_select){  
    case 0: sprite_offset =  0;   sprite_draw =  0;  break;  
    case 1: sprite_offset = -60;  sprite_draw = 60;  break;
    case 2: sprite_offset = -120; sprite_draw = 120; break;
    case 3: sprite_offset = -180; sprite_draw = 180; break;
    case 4: sprite_offset = 0; sprite_draw = 0; sprite_select = 0; break;
  }  
  
  sprPtr = (uint16_t*)spr.createSprite(DWIDTH, 60);
  if(sprite_select != 0){spr.setViewport(0, sprite_offset, DWIDTH, DHEIGHT);}
  spr.setTextDatum(MC_DATUM);  
  
  /*------------Сетка-----------------------------*/
  spr.fillSprite(TFT_BLACK); // Очистка экрана   
  spr.drawFastHLine(0, 0,   DWIDTH, TFT_SILVER); 
  spr.drawFastHLine(0, 235, DWIDTH, TFT_SILVER);   
  spr.drawFastVLine(0, 0, 235, TFT_SILVER);
  spr.drawFastVLine(319, 0, 235, TFT_SILVER);  

 if(call_eload.step_callibration==0){ 
    spr.setFreeFont(FONT);    
    spr.drawString(utf8rus(text_5), 159, 30);    
    String strPort = "Порт номер ";
    call_eload.port_call_position = call_eload.active_pos_enc;    
    strPort += String(call_eload.port_call_position + 1);
    strPort.toCharArray(text_6, 40);  
    spr.setFreeFont(FONT2);    
    spr.drawString(utf8rus(text_6), 159, 105); 
  }

  if(call_eload.step_callibration==1){ 
    spr.setFreeFont(FONT);    
    spr.drawString(utf8rus(text_4), 159, 30); 
    String strCur = String(EL_max_power[call_eload.active_pos_enc]);
    strCur += " W";
    strCur.toCharArray(text_2, 10);  
    spr.setFreeFont(FONT2);    
    spr.drawString(utf8rus(text_2), 159, 105);   
  }
  
  if(call_eload.step_callibration==2){ 
    spr.setFreeFont(FONT);    
    spr.drawString(utf8rus(text_1), 159, 30); 
    String strCur = String(EL_table[call_eload.arr_call_position][0]);
    strCur += " mA";
    strCur.toCharArray(text_2, 10);  
    spr.setFreeFont(FONT2);    
    spr.drawString(utf8rus(text_2), 159, 105);    
    String strEnc = "шаг энкодера ";
    strEnc += String(call_eload.pwm_val_enc[call_eload.active_pos_enc]);  
    strEnc += " pwm";
    strEnc.toCharArray(text_3, 40);
    spr.setFreeFont(FONT);    
    spr.drawString(utf8rus(text_3), 159, 200);  
  }

  if(call_eload.step_callibration==3){ 
    spr.setTextColor(TFT_DARKGREEN);
    spr.setFreeFont(FONT);    
    spr.drawString(utf8rus(text_7), 159, 30);     
    spr.setFreeFont(FONT);    
    spr.drawString(utf8rus(text_9), 159, 200);
    spr.setFreeFont(FONT2);     
    spr.drawString(utf8rus(text_8), 159, 105); 
  }    
  
  
  tft.pushImageDMA(0, sprite_draw, 320, 60, sprPtr);
  spr.deleteSprite();  // Delete the sprite to free up the RAM
  sprite_select ++;
  vTaskDelay(20);
  }
}

void Init_Task6() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task6code, /* Функция задачи. */
    "Task6",   /* Ее имя. */
    4096,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task6,    /* Дескриптор задачи для отслеживания */
    1);        /* Указываем пин для данного ядра */
  delay(500);
}

void Task7code(void* pvParameters) {  // Функции энкодера (терминал)
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task7code running on core ");
  Serial.println(xPortGetCoreID());
  #endif  

  for (;;) {
    
  // =============== ЭНКОДЕР ===============

   if (enc.left() || Enc_step<0)  { // поворот налево
    Enc_step=0;    
    
      if(call_eload.step_callibration==0){
        call_eload.active_pos_enc --;
        if(call_eload.active_pos_enc < 0){call_eload.active_pos_enc = 0;}       
      }

      if(call_eload.step_callibration==1){
        call_eload.active_pos_enc --;
        if(call_eload.active_pos_enc < 0){call_eload.active_pos_enc = 0;}
        }

      if(call_eload.step_callibration==2){
        call_eload.pwm_chanal_value = call_eload.pwm_chanal_value - call_eload.pwm_val_enc[call_eload.active_pos_enc];
        if(call_eload.pwm_chanal_value > 64000){call_eload.pwm_chanal_value = 0;}
        Set_pwm_chanal(call_eload.pwm_chanal_value, call_eload.port_call_position);       
       }     
      }
   if (enc.right()|| Enc_step>0) {  // поворот направо 
    Enc_step=0;   

      if(call_eload.step_callibration==0){
        call_eload.active_pos_enc ++;
        if(call_eload.active_pos_enc > 4){call_eload.active_pos_enc = 4;}        
      }

      if(call_eload.step_callibration==1){
        call_eload.active_pos_enc ++;
        if(call_eload.active_pos_enc > 3){call_eload.active_pos_enc = 3;}
        }

      if(call_eload.step_callibration==2){  
        call_eload.pwm_chanal_value = call_eload.pwm_chanal_value + call_eload.pwm_val_enc[call_eload.active_pos_enc];
        if(call_eload.pwm_chanal_value > 64000){call_eload.pwm_chanal_value = 64000;}
        Set_pwm_chanal(call_eload.pwm_chanal_value, call_eload.port_call_position);  
        }
      }
   if (enc.click()|| Enc_click==1){ // click по энкодеру
    Enc_click=0;
    if (call_eload.step_callibration == 2){ 
      call_eload.active_pos_enc++;
      if(call_eload.active_pos_enc == call_eload.pwm_arr_size){call_eload.active_pos_enc = 0;}      
      }    
    }  
   if (enc.held() || Enc_held==1){  // долгое нажатие
    Enc_held=0;     
    switch (call_eload.step_callibration){
      case 0: SelectedPort();  call_eload.step_callibration++; call_eload.active_pos_enc=0; break;    
      case 1: SelectedPower(); call_eload.step_callibration++; call_eload.active_pos_enc=0; break;       
      case 2: call_eload.step_callibration++; SaveAllConfig(); break; 
      }                                                                                                                                                                          
    } 
   if (enc.leftH()|| Enc_stepH<0) { // нажатый поворот налево    
   Enc_stepH=0;
    if(call_eload.port_call_position == 0){EE_VALUE.EL_table_1[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 1){EE_VALUE.EL_table_2[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 2){EE_VALUE.EL_table_3[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 3){EE_VALUE.EL_table_4[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 4){EE_VALUE.EL_table_5[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    call_eload.arr_call_position --;    
    if(call_eload.arr_call_position < 0){call_eload.arr_call_position=0;}
    if(call_eload.port_call_position == 0){call_eload.pwm_chanal_value = EE_VALUE.EL_table_1[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 1){call_eload.pwm_chanal_value = EE_VALUE.EL_table_2[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 2){call_eload.pwm_chanal_value = EE_VALUE.EL_table_3[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 3){call_eload.pwm_chanal_value = EE_VALUE.EL_table_4[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 4){call_eload.pwm_chanal_value = EE_VALUE.EL_table_5[call_eload.arr_call_position][1];}
    Set_pwm_chanal(call_eload.pwm_chanal_value, call_eload.port_call_position);
   }  
   if (enc.rightH()|| Enc_stepH>0){ // нажатый поворот направо  
   Enc_stepH=0;
    if(call_eload.port_call_position == 0){EE_VALUE.EL_table_1[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 1){EE_VALUE.EL_table_2[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 2){EE_VALUE.EL_table_3[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 3){EE_VALUE.EL_table_4[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    if(call_eload.port_call_position == 4){EE_VALUE.EL_table_5[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
    call_eload.arr_call_position ++;
    if(call_eload.arr_call_position > EE_VALUE.EL_dot_count[call_eload.port_call_position]){call_eload.arr_call_position = EE_VALUE.EL_dot_count[call_eload.port_call_position];}    
    if(call_eload.port_call_position == 0){call_eload.pwm_chanal_value = EE_VALUE.EL_table_1[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 1){call_eload.pwm_chanal_value = EE_VALUE.EL_table_2[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 2){call_eload.pwm_chanal_value = EE_VALUE.EL_table_3[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 3){call_eload.pwm_chanal_value = EE_VALUE.EL_table_4[call_eload.arr_call_position][1];}
    if(call_eload.port_call_position == 4){call_eload.pwm_chanal_value = EE_VALUE.EL_table_5[call_eload.arr_call_position][1];}
    Set_pwm_chanal(call_eload.pwm_chanal_value, call_eload.port_call_position);    
    } 
   enc.resetState();     
   vTaskDelay(30);    
  }
}

void Init_Task7() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task7code, /* Функция задачи. */
    "Task7",   /* Ее имя. */
    4096,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task7,    /* Дескриптор задачи для отслеживания */
    0);        /* Указываем пин для данного ядра */
  delay(500);
}

void Task8code(void* pvParameters) {  // Сохранение в EEPROM 
  #if (ENABLE_DEBUG_TASK == 1)
  Serial.print("Task8code running on core ");
  Serial.println(xPortGetCoreID()); 
  #endif 

  int update_eeprom = 0; 

  for (;;) {   


    //for (int i = 0; i < PSNUMBER; i++) { 
    //  for(int j=0; j<COUNT_PRESET; j++){ 
    //    if (EE_VALUE.volt_preset[i][j]  != power_supply[i].volt_preset[j])  {EE_VALUE.volt_preset[i][j]  = power_supply[i].volt_preset[j];  update_eeprom = 1;}     
    //    if (EE_VALUE.curr_protect[i][j] != power_supply[i].curr_protect[j]) {EE_VALUE.curr_protect[i][j] = power_supply[i].curr_protect[j]; update_eeprom = 1;} 
    //   }
    // }

    for (int i = 0; i < ELNUMBER_MAX; i++) {  // Инициализация структур параметров нагрузок    
        
    for(int val=0; val<COUNT_PRESET; val++){
      if (EE_VALUE.curr[i][val] != eload[i].curr_val_preset[val]){ // значения токов
        EE_VALUE.curr[i][val] = eload[i].curr_val_preset[val]; 
        update_eeprom = 1;} 
      }
    }

   if (update_eeprom == 1){    
    update_eeprom = 0; 
    EEPROM.put(0, EE_VALUE); // сохраняем
    EEPROM.commit();         // записываем
   }
   vTaskDelay(300000/portTICK_PERIOD_MS); // 5 минут
  }
}

void Init_Task8() {  //создаем задачу
  xTaskCreatePinnedToCore(
    Task8code, /* Функция задачи. */
    "Task8",   /* Ее имя. */
    4096,      /* Размер стека функции */
    NULL,      /* Параметры */
    2,         /* Приоритет */
    &Task8,    /* Дескриптор задачи для отслеживания */
    0);        /* Указываем пин для данного ядра */
  delay(50);
}

void Set_pwm_chanal(uint32_t pwm, int number){   
  switch (number){  
    case 0 : number = 2; break;
    case 2 : number = 0; break;
  } 
  if(number > 4){return;} 
  if(pwm == 0){ledcWrite(pwmChannel[number], 0); return;}
  ledcWrite(pwmChannel[number], pwm);  
} 

void SelectedPort(){
  #if (DEBUG_CALLIBROVKA == 1) 
  Serial.println();
  Serial.print("Выбран порт ");
  Serial.println(call_eload.port_call_position + 1);
  Serial.println();
  #endif
}

void SelectedPower(){  
  EE_VALUE.EL_max_current[call_eload.active_pos_enc] = EL_max_current[call_eload.active_pos_enc];
  EE_VALUE.EL_max_voltage[call_eload.active_pos_enc] = EL_max_voltage[call_eload.active_pos_enc];
  EE_VALUE.EL_max_power[call_eload.active_pos_enc] = EL_max_power[call_eload.active_pos_enc];
  EE_VALUE.EL_dot_count[call_eload.active_pos_enc] = EL_dot_count[call_eload.active_pos_enc];   
}

void SaveAllConfig(){  
  Set_pwm_chanal(0, call_eload.port_call_position); 
  if(call_eload.port_call_position == 0){EE_VALUE.EL_table_1[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
  if(call_eload.port_call_position == 1){EE_VALUE.EL_table_2[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
  if(call_eload.port_call_position == 2){EE_VALUE.EL_table_3[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
  if(call_eload.port_call_position == 3){EE_VALUE.EL_table_4[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
  if(call_eload.port_call_position == 4){EE_VALUE.EL_table_5[call_eload.arr_call_position][1] = call_eload.pwm_chanal_value;}
  #if (DEBUG_CALLIBROVKA == 1) 
  Serial.print("Сохраняем ВСЁ");  
  #endif
  EEPROM.put(0, EE_VALUE);      // сохраняем
  EEPROM.commit();              // записываем
  delay(5000);
  ESP.restart();
}

void IRAM_ATTR serialEvent(){   

  if (Serial.readBytes((byte*)&RxBuff, sizeof(RxBuff))) {
  byte crc = crc8_bytes((byte*)&RxBuff, sizeof(RxBuff));
  if (crc == 0) {
  
      message_uart_resive message;          
      message.activeRow = RxBuff.Row;        // Номер строки
      message.activeColumn = RxBuff.Column;  // Номер столбца
      message.statusColumn = RxBuff.RawBits; // Байт с битами всего столбца
      message.statPress = RxBuff.statPress;  // Статус нажата или отпущена кнопка
      message.enc_step = RxBuff.enc_step; 
      message.enc_stepH = RxBuff.enc_stepH;  
      message.enc_click = RxBuff.enc_click; 
      message.enc_held = RxBuff.enc_held; 
    
      if(QueueHandleUartResive != NULL && uxQueueSpacesAvailable(QueueHandleUartResive) > 0){ // проверьте, существует ли очередь И есть ли в ней свободное место
        int ret = xQueueSend(QueueHandleUartResive, (void*) &message, 0);
        if(ret == pdTRUE){      
          }        
        }     
      } 
   else {}
  }  
}

byte crc8_bytes(byte *buffer, byte size) {
  byte crc = 0;
  for (byte i = 0; i < size; i++) {
    byte data = buffer[i];
    for (int j = 8; j > 0; j--) {
      crc = ((crc ^ data) & 1) ? (crc >> 1) ^ 0x8C : (crc >> 1);
      data >>= 1;
    }
  }
  return crc;
}

void Set_current_chanal(float curr, int number){ 
  
  if(number > 4){return;} 
  if(curr == 0){ledcWrite(pwmChannel[number], 0); return;}
  uint16_t search;
  uint16_t min;
  uint16_t max; 
  uint16_t val_max;
  uint16_t val_min;
  float curr_f = round(curr*1000.00); 
  uint16_t current = (unsigned int)curr_f;
  #if (ENABLE_DEBUG_CONV == 1)
    Serial.println();
    Serial.print("curr_f: ");
    Serial.println(curr_f);
    Serial.print("word(curr_f: ");
    Serial.println(current);
    Serial.print("eload[number].device_table_size: ");
    Serial.println(eload[number].device_table_size);
  #endif

  for (uint8_t i = 0; i < eload[number].device_table_size; i++){ 
    search = eload[number].device_table[i][0]; 
  if(current <= search){
    max = search;
    val_max = eload[number].device_table[i][1];
    i=i-1; 
    min = eload[number].device_table[i][0];
    val_min = eload[number].device_table[i][1];
    float t = float(max-min)/float(current-min);  
    float PWM = ((val_max-val_min)/t)+val_min;
    word pwm_load = word(PWM);
  #if (ENABLE_DEBUG_CONV == 1)
    Serial.print("search ");
    Serial.println(search);
    Serial.print("t " );
    Serial.println(t);
    Serial.print("float PWM ");
    Serial.println(PWM);   
    Serial.print("pwm_load "); 
    Serial.println(pwm_load);
  #endif
  ledcWrite(pwmChannel[number], pwm_load);
  return; 
  }
 }
} 

void INIT_PWM_IO(){
  ledcSetup(pwmChannel_1, frequency, resolution); // задаём настройки ШИМ-канала:                                            
  ledcAttachPin(PWM_Pin_1, pwmChannel_1);         // подключаем ШИМ-канал к пину  
  ledcSetup(pwmChannel_2, frequency, resolution); // задаём настройки ШИМ-канала:                                            
  ledcAttachPin(PWM_Pin_2, pwmChannel_2);         // подключаем ШИМ-канал к пину  
  ledcSetup(pwmChannel_3, frequency, resolution); // задаём настройки ШИМ-канала:                                            
  ledcAttachPin(PWM_Pin_3, pwmChannel_3);         // подключаем ШИМ-канал к пину  
  ledcSetup(pwmChannel_4, frequency, resolution); // задаём настройки ШИМ-канала:                                            
  ledcAttachPin(PWM_Pin_4, pwmChannel_4);         // подключаем ШИМ-канал к пину  
  ledcSetup(pwmChannel_5, frequency, resolution); // задаём настройки ШИМ-канала:                                            
  ledcAttachPin(PWM_Pin_5, pwmChannel_5);         // подключаем ШИМ-канал к пину
  
  ledcWrite(pwmChannel_1, 0);      // Начальное значение скважности ШИМ-канала
  ledcWrite(pwmChannel_2, 0);      // Начальное значение скважности ШИМ-канала
  ledcWrite(pwmChannel_3, 0);      // Начальное значение скважности ШИМ-канала
  ledcWrite(pwmChannel_4, 0);      // Начальное значение скважности ШИМ-канала
  ledcWrite(pwmChannel_5, 0);      // Начальное значение скважности ШИМ-канала

  pinMode(DETECT_ELOAD_1, INPUT);
  pinMode(DETECT_ELOAD_2, INPUT);
  pinMode(DETECT_ELOAD_3, INPUT);
  pinMode(DETECT_ELOAD_4, INPUT);
  pinMode(DETECT_ELOAD_5, INPUT);
  
  pinMode(BTN_HALL, INPUT);
  pinMode(ENCODER_A, INPUT);
  pinMode(ENCODER_B, INPUT);
}

void INIT_DALLAS(){
  
  sensors.begin();                               // Запустите библиотеку
  numberberOfDevices = sensors.getDeviceCount(); // Подсчитайте количество устройств на проводе    
  for (int i=0; i<8; i++){ThermometerArr[0][i] = (byte)EE_VALUE.TermAdress[0][i];}
  for (int i=0; i<8; i++){ThermometerArr[1][i] = (byte)EE_VALUE.TermAdress[1][i];}
  for (int i=0; i<8; i++){ThermometerArr[2][i] = (byte)EE_VALUE.TermAdress[2][i];}
  for (int i=0; i<8; i++){ThermometerArr[3][i] = (byte)EE_VALUE.TermAdress[3][i];}
  for (int i=0; i<8; i++){ThermometerArr[4][i] = (byte)EE_VALUE.TermAdress[4][i];}   
  sensors.setResolution(ThermometerArr[0], TEMPERATURE_PRECISION); // разрешение датчика в битах
  sensors.setResolution(ThermometerArr[1], TEMPERATURE_PRECISION); // разрешение датчика в битах
  sensors.setResolution(ThermometerArr[2], TEMPERATURE_PRECISION); // разрешение датчика в битах
  sensors.setResolution(ThermometerArr[3], TEMPERATURE_PRECISION); // разрешение датчика в битах
  sensors.setResolution(ThermometerArr[4], TEMPERATURE_PRECISION); // разрешение датчика в битах  

}

void INIT_LCD(){
  tft.init();
  tft.setRotation(1);
  tft.initDMA();
  tft.fillScreen(TFT_BLACK);  
  tft.startWrite(); // TFT chip select held low permanently
}

void INIT_ELOAD(){

    memmove (&eload[0].device_id, &ThermometerArr[0], 8);
    memcpy(*eload[0].device_table, *EE_VALUE.EL_table_3, sizeof(EE_VALUE.EL_table_3));
    eload[0].device_table_size = EE_VALUE.EL_dot_count[0];
    eload[0].device_max_current = EE_VALUE.EL_max_current[0];
    eload[0].device_max_voltage = EE_VALUE.EL_max_voltage[0];
    eload[0].device_max_power = EE_VALUE.EL_max_power[0];

    memmove (&eload[1].device_id, &ThermometerArr[1], 8);
    memcpy(*eload[1].device_table, *EE_VALUE.EL_table_2, sizeof(EE_VALUE.EL_table_2));
    eload[1].device_table_size = EE_VALUE.EL_dot_count[1];
    eload[1].device_max_current = EE_VALUE.EL_max_current[1];
    eload[1].device_max_voltage = EE_VALUE.EL_max_voltage[1];
    eload[1].device_max_power = EE_VALUE.EL_max_power[1];

    memmove (&eload[2].device_id, &ThermometerArr[2], 8);
    memcpy(*eload[2].device_table, *EE_VALUE.EL_table_1, sizeof(EE_VALUE.EL_table_1));
    eload[2].device_table_size = EE_VALUE.EL_dot_count[2];
    eload[2].device_max_current = EE_VALUE.EL_max_current[2];
    eload[2].device_max_voltage = EE_VALUE.EL_max_voltage[2];
    eload[2].device_max_power = EE_VALUE.EL_max_power[2];

    memmove (&eload[3].device_id, &ThermometerArr[3], 8);
    memcpy(*eload[3].device_table, *EE_VALUE.EL_table_4, sizeof(EE_VALUE.EL_table_4));
    eload[3].device_table_size = EE_VALUE.EL_dot_count[3];
    eload[3].device_max_current = EE_VALUE.EL_max_current[3];
    eload[3].device_max_voltage = EE_VALUE.EL_max_voltage[3];
    eload[3].device_max_power = EE_VALUE.EL_max_power[3];

    memmove (&eload[4].device_id, &ThermometerArr[4], 8);
    memcpy(*eload[4].device_table, *EE_VALUE.EL_table_5, sizeof(EE_VALUE.EL_table_5));
    eload[4].device_table_size = EE_VALUE.EL_dot_count[4];
    eload[4].device_max_current = EE_VALUE.EL_max_current[4];
    eload[4].device_max_voltage = EE_VALUE.EL_max_voltage[4];
    eload[4].device_max_power = EE_VALUE.EL_max_power[4];
   
  for (int i = 0; i < ELNUMBER_MAX; i++) {  // Инициализация структур параметров нагрузок 
    eload[i].tempC_val = 0.00;          // значение температуры
    eload[i].tempC_decimal = 1;         // количаство знаков после запятой 
    eload[i].tempC_col = TFT_LIGHTGREY; // цвет температуры  
    eload[i].device_conn_counter = 0;   // счетчик ошибок соединения с нагрузкой 
    for(int col=0; col<COUNT_PRESET; col++){eload[i].curr_col[col] = TFT_PURPLE;}                   // цвет тока не активного
    eload[i].curr_col[0] = TFT_GREEN;                                                               // цвет тока активного       
    for(int val=0; val<COUNT_PRESET; val++){
      eload[i].curr_val_preset[val] = EE_VALUE.curr[i][val]; // значение тока 
      if (eload[i].curr_val_preset[val] <  10 ){eload[i].curr_decimal[val] = 3;} // количаство знаков после запятой 
      if (eload[i].curr_val_preset[val] >= 10 ){eload[i].curr_decimal[val] = 2;} // количаство знаков после запятой 
      if (eload[i].curr_val_preset[val] >= 100){eload[i].curr_decimal[val] = 1;} // количаство знаков после запятой 
    }         
  }

    
  int pos_x    = 50;  // Координата х значения тока в верхнем левом углу
  int pos_y    = 217; // Координата y значения тока в верхнем левом углу
  int offset_x = 107; // Смещение вправо для следующего значения тока 
  int offset_y = 39;  // Смещение вниз для следующего значения тока
  int pos_x_t = pos_x;
  int pos_y_t = pos_y; 
     
  for(int pos=0; pos<COUNT_PRESET; pos++){ // Назначение eload[2] левого столбца для отображения
    eload[2].curr_pos_x[pos] = pos_x_t;
    eload[2].curr_pos_y[pos] = pos_y_t;
    pos_y_t -= offset_y;
    }      
  pos_x_t = pos_x + offset_x;
  pos_y_t = pos_y;  
  for(int pos=0; pos<COUNT_PRESET; pos++){ // Назначение eload[1] центрального столбца для отображения
    eload[1].curr_pos_x[pos] = pos_x_t;
    eload[1].curr_pos_y[pos] = pos_y_t;
    pos_y_t -= offset_y;
    }      
  pos_x_t = pos_x_t + offset_x;
  pos_y_t = pos_y;
  for(int pos=0; pos<COUNT_PRESET; pos++){ // Назначение eload[0] правого столбца для отображения
    eload[0].curr_pos_x[pos] = pos_x_t;
    eload[0].curr_pos_y[pos] = pos_y_t;
    pos_y_t -= offset_y;
    }
}

void INIT_TIM_ENC(){
  timer = timerBegin(0, 240, true);
  timerAttachInterrupt(timer, &onTimer, false);
  timerAlarmWrite(timer, 500, true);
  timerAlarmEnable(timer); //Just Enable
}

void INIT_DEFAULT_VALUE(){ // Заполняем переменные в EEPROM базовыми значениями 
    
    EE_VALUE.initKey = INIT_KEY;
    String ssid_default = "AEDON_DEV";
    String pass_default = "NL3eW6kWH"; 
    ssid_default.toCharArray(EE_VALUE.ssid, 20);    
    pass_default.toCharArray(EE_VALUE.pass, 20);

    float default_curr[] = {0, 0.1, 0.55, 1.00, 1.10, 1.4}; // значение тока 
    for(int i=0; i<ELNUMBER_MAX; i++){
      for(int val=0; val<COUNT_PRESET; val++){
        EE_VALUE.curr[i][val] = default_curr[val];
        } 
      } 

    memcpy(*EE_VALUE.EL_table_1, *EL_table, sizeof(EL_table));  
    memcpy(*EE_VALUE.EL_table_2, *EL_table, sizeof(EL_table));
    memcpy(*EE_VALUE.EL_table_3, *EL_table, sizeof(EL_table));
    memcpy(*EE_VALUE.EL_table_4, *EL_table, sizeof(EL_table));
    memcpy(*EE_VALUE.EL_table_5, *EL_table, sizeof(EL_table));

    for(int i=0; i<ELNUMBER_MAX; i++){       
        EE_VALUE.EL_max_current[i] = EL_max_current[0];
        EE_VALUE.EL_max_voltage[i] = EL_max_voltage[0];
        EE_VALUE.EL_max_power[i]   = EL_max_power[0];
        EE_VALUE.EL_dot_count[i]   = EL_dot_count[0];        
      }

    EEPROM.put(0, EE_VALUE);      // сохраняем
    EEPROM.commit();              // записываем
    delay(5000);
    
}

char *utf8rus(char *source) // функция для поддержки русского шрифта
{
int i,j,k;
unsigned char n;
char m[2] = { '0', '\0' };
strcpy(target, ""); k = strlen(source); i = j = 0;
while (i < k) {
    n = source[i]; i++;

    if (n >= 127) {
     switch (n) {
        case 208: {
         n = source[i]; i++;
         if (n == 129) { n = 192; break; } // перекодируем букву Ё
         break;
        }
        case 209: {
         n = source[i]; i++;
         if (n == 145) { n = 193; break; } // перекодируем букву ё
         break;
        }
     }
    }

    m[0] = n; strcat(target, m);
    j++; if (j >= maxString) break;
}
return target;
}

void setup() {
  INIT_PWM_IO();
  Serial.setTimeout(5);
  Serial.begin(115200);
  
  EEPROM.begin(4096);
  EEPROM.get(0, EE_VALUE); //читаем всё из памяти   

  if (EE_VALUE.initKey != INIT_KEY){INIT_DEFAULT_VALUE();} // первый запуск устройства
  ELNUMBER = EE_VALUE.elnumber;
  
  QueueHandleUartResive = xQueueCreate(QueueElementSizeUart, sizeof(message_uart_resive));
  if(QueueHandleUartResive == NULL){  // Проверьте, была ли успешно создана очередь
    Serial.println("QueueHandleUartResive could not be created. Halt.");
    while(1) delay(1000);   // Halt at this point as is not possible to continue
  }

  INIT_DALLAS();
  INIT_ELOAD();
  INIT_LCD();
  INIT_TIM_ENC();  

  if(digitalRead(BTN_HALL) == 1){ // Нормальный запуск
    Init_Task3();    // Работа LCD
    Init_Task2();    // Обработка энкодера  
    Init_Task1();    // Обработка принятых данных от клавиатуры
    Init_Task4();    // Выключение отображения окна с шагом настройки
    Init_Task8();    // Сохранение в EEPROM
  }

  if(digitalRead(BTN_HALL) == 0){ // Если нажат энкодер при старте запускаем калибровку
    Init_Task6();    // Работа LCD (терминал)
    Init_Task7();    // Функции энкодера (терминал)     
    Init_Task1();    // Обработка принятых данных от клавиатуры
  }
}

void loop() {
   
}
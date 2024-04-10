// #define button_h
#include "items.h"
#include <Arduino.h>
#include "U8g2lib.h"
#include "bitmaps.h"
#include "button.h"
#include "task_manager.h"
#include "si4432.h"
// https://github.com/nopnop2002/Arduino-SI4432/tree/main

U8G2_SSD1306_128X64_NONAME_F_HW_I2C U8G2(U8G2_R0); // [full framebuffer, size = 1024 bytes]

#define BUTTON_UP_PIN PC13	  // pin for UP button
#define BUTTON_OK_PIN PB10	  // pin for SELECT button
#define BUTTON_DOWN_PIN PA2	  // pin for DOWN button
#define BUTTON_CANCEL_PIN PA7 // pin for demo mode, use switch or wire to enable or disable demo mode, see more details below

#define MOSI2 PB15
#define MISO2 PB14
#define SCLK2 PB13

item Cube("3D Cube", bitmap_icon_3dcube);
// item Battery("Battery", bitmap_icon_battery, 1);
item Sys_info("Sys. info", bitmap_icon_dashboard);
// item Fireworks("Fireworks", bitmap_icon_fireworks, 3);
sniffer SI4432_Test("SI4432 Test", bitmap_icon_gps_speed);

item *items_array[] = {&Cube, &Sys_info, &SI4432_Test};
menu main_menu(3, items_array);
// item Big_Knob("Big_Knob", bitmap_icon_knob_over_oled, 5);
// item Park_Sensor("Park Sensor", bitmap_icon_parksensor, 6);
// item Turbo_Gauge("Turbo Gauge", bitmap_icon_turbo, 7);
GUI MY_GUI(&U8G2, &main_menu);

HardwareTimer My_timer(TIM9);
task_manager My_tasks(&My_timer, 500);

static Button button_ok(BUTTON_OK_PIN, &My_tasks, ok);
static Button button_down(BUTTON_DOWN_PIN, &My_tasks, down);
static Button button_up(BUTTON_UP_PIN, &My_tasks, up);
static Button button_cancel(BUTTON_CANCEL_PIN, &My_tasks, cancel);
Si4432 radio(PB12, PA9, PA8); // CS, SDN, IRQ

// lambda a = [](){on_receive();}

void setup()
{
	SPI.setMOSI(MOSI2);
	SPI.setMISO(MISO2);
	SPI.setSCLK(SCLK2);
	SPI.begin();

	radio.init(&SPI);

	if (radio.init() == false)
	{
		Serial.println("SI4432 not installed");

		// while(1);
		NVIC_SystemReset();
	}
	radio.setBaudRate(70);
	radio.setFrequency(433);
	radio.readAll();

	radio.startListening();

	U8G2.setColorIndex(1); // set the color to white
	U8G2.begin();
	U8G2.setBitmapMode(1);

	MY_GUI.draw();
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);

	// https://stackoverflow.com/questions/7582546/using-generic-stdfunction-objects-with-member-functions-in-one-class
	//	std::function<void()> f = std::bind(&MY_GUI.interrupt);

	// костыль, т.к. иначе бы пришлось иметь дело с статическими членами, едиными для нескольких экземпляров класса.
	SI4432_Test.init([]()
					 { SI4432_Test.on_receive(); },
					 &My_tasks, &radio);
	button_ok.init(
		[]()
		{ button_ok.Clarify_Status_Single(); }, // обработка одинарного нажатия
		[]()
		{ button_ok.Clarify_Status_Double(); }, // обработка двойного   нажатия
		[]()
		{ button_ok.Clarify_Status_Long(); }, // обработка повторного нажатия
		[]()
		{ button_ok.Detect_Press(); }, // обработка изменения статуса кнопки (отпустили/нажали)
		[]()
		{ button_ok.Contact_Bounce_Checker(); }, // обработка дребезга контакта
		[]()
		{ MY_GUI.interrupt(ok); }, // одиночное нажатие
		[]()
		{ MY_GUI.interrupt(ok); }, // двойное нажатие
		[]()
		{ MY_GUI.interrupt(ok); }); // долгое нажатие // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

	button_cancel.init(
		[]()
		{ button_cancel.Clarify_Status_Single(); }, // обработка одинарного нажатия
		[]()
		{ button_cancel.Clarify_Status_Double(); }, // обработка двойного   нажатия
		[]()
		{ button_cancel.Clarify_Status_Long(); }, // обработка повторного нажатия
		[]()
		{ button_cancel.Detect_Press(); }, // обработка изменения статуса кнопки (отпустили/нажали)
		[]()
		{ button_cancel.Contact_Bounce_Checker(); }, // обработка дребезга контакта
		[]()
		{ MY_GUI.interrupt(cancel); }, // одиночное нажатие
		[]()
		{ MY_GUI.interrupt(cancel); }, // двойное нажатие
		[]()
		{ MY_GUI.interrupt(cancel); }); // долгое нажатие // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

	button_down.init(
		[]()
		{ button_down.Clarify_Status_Single(); }, // обработка одинарного нажатия
		[]()
		{ button_down.Clarify_Status_Double(); }, // обработка двойного   нажатия
		[]()
		{ button_down.Clarify_Status_Long(); }, // обработка повторного нажатия
		[]()
		{ button_down.Detect_Press(); }, // обработка изменения статуса кнопки (отпустили/нажали)
		[]()
		{ button_down.Contact_Bounce_Checker(); }, // обработка дребезга контакта
		[]()
		{ MY_GUI.interrupt(down); }, // одиночное нажатие
		[]()
		{ MY_GUI.interrupt(down); }, // двойное нажатие
		[]() {						 /*MY_GUI.interrupt(down);*/
			   button_down.Repeat([]()
								  { MY_GUI.interrupt(down); });
		}); // долгое нажатие // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

	button_up.init(
		[]()
		{ button_up.Clarify_Status_Single(); }, // обработка одинарного нажатия
		[]()
		{ button_up.Clarify_Status_Double(); }, // обработка двойного   нажатия
		[]()
		{ button_up.Clarify_Status_Long(); }, // обработка повторного нажатия
		[]()
		{ button_up.Detect_Press(); }, // обработка изменения статуса кнопки (отпустили/нажали)
		[]()
		{ button_up.Contact_Bounce_Checker(); }, // обработка дребезга контакта
		[]()
		{ MY_GUI.interrupt(up); }, // одиночное нажатие
		[]()
		{ MY_GUI.interrupt(up); }, // двойное нажатие
		[]() {					   /*MY_GUI.interrupt(down);*/
			   button_up.Repeat([]()
								{ MY_GUI.interrupt(up); });
		}); // долгое нажатие // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void loop()
{
	MY_GUI.draw();
	U8G2.sendBuffer();
}
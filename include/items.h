#ifndef items_h
#define items_h

#include <Arduino.h>
#include "U8g2lib.h"
#include "bitmaps.h"
#include "button.h"
#include "si4432.h"

class item
{
public:
	bool actual    = false;
	bool to_update = false;

	const char *name = nullptr;
	const uint8_t *icon = nullptr;
	item *prev_item = nullptr;
	// bool actual = false;

	item() {}

	item(const char *_name, const uint8_t *_icon) : name(_name),
													icon(_icon) {}
	//																			 item_selected(_item_number) {}

	virtual void draw(U8G2_SSD1306_128X64_NONAME_F_HW_I2C *_u8g2)
	{
		_u8g2->drawXBMP(0, 0, 128, 64, bitmap_screenshots[0]); // draw screenshot
	}

	// нажатие кнопки например
	virtual item *event(const buttons_name name)
	{
		if (name == cancel)
		{
			//	actual = false;
			return prev_item;
		}
		else
		{
			return this;
		}
	}
};

class menu : public item
{
	uint8_t item_selected = 0;	// номер текущей вкладки
	const uint8_t item_num = 0; // количество вкладок
	item **items_set;			// массив указателей на вкладки

public:
	// template<typename... Args>
	// void

	menu(const int N, item **items_list) : item_num(N), items_set(items_list)
	{
		for (int i = 0; i < N; ++i)
		{
			items_set[i]->prev_item = this;
		}
	}

	void draw(U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2) override
	{
		uint8_t item_selected_prev = (item_num + item_selected - 1) % item_num;
		uint8_t item_selected_next = (item_selected + 1) % item_num;

		// selected item background
		u8g2->drawXBMP(0, 22, 128, 21, bitmap_item_sel_outline);

		// draw previous item as icon + label
		u8g2->setFont(u8g_font_7x14);
		u8g2->drawStr(25, 15, items_set[item_selected_prev]->name);
		u8g2->drawXBMP(4, 2, 16, 16, items_set[item_selected_prev]->icon);

		// draw selected item as icon + label in bold font
		u8g2->setFont(u8g_font_7x14B);
		u8g2->drawStr(25, 15 + 20 + 2, items_set[item_selected]->name);
		u8g2->drawXBMP(4, 24, 16, 16, items_set[item_selected]->icon);

		// draw next item as icon + label
		u8g2->setFont(u8g_font_7x14);
		u8g2->drawStr(25, 15 + 20 + 20 + 2 + 2, items_set[item_selected_next]->name);
		u8g2->drawXBMP(4, 46, 16, 16, items_set[item_selected_next]->icon);

		// draw scrollbar background
		u8g2->drawXBMP(128 - 8, 0, 8, 64, bitmap_scrollbar_background);

		// draw scrollbar handle
		u8g2->drawBox(125, 64 / item_num * item_selected, 3, 64 / item_num);

		// draw upir logo
		u8g2->drawXBMP(128 - 16 - 4, 64 - 4, 16, 4, upir_logo); // draw screenshot
	}

	item *event(const buttons_name name) override
	{ // нажатие кнопки например

		if (name == ok)
		{
			return items_set[item_selected];
		}
		else if (name == down)
		{
			item_selected = (item_selected + 1) % item_num;
		}
		else if (name == up)
		{
			item_selected = (item_num + item_selected - 1) % item_num;
		}
		return this;
	}
};

class sniffer : public item
{
public:
	char Buf[64] = {'\0'};
	Si4432 *radio = nullptr;
	task_manager *tasks = nullptr;

	std::function<void()> on_receive_f;

	sniffer(const char *_name, const uint8_t *_icon) : item(_name, _icon) {}

	void init(std::function<void()> __on_receive, task_manager *_tasks, Si4432 *_radio)
	{
		on_receive_f = __on_receive;
		tasks = _tasks;
		radio = _radio;
	}

	virtual void draw(U8G2_SSD1306_128X64_NONAME_F_HW_I2C *_u8g2)
	{
		if (actual == false){
			actual = true;
			on_receive();
		}
		//_u8g2->drawXBMP(0, 0, 128, 64, bitmap_screenshots[0]); // draw screenshot
		//Serial.println("WRITE!!!");
		_u8g2->setFont(u8g_font_4x6);

		_u8g2->drawStr(5, 8, Buf);
	//	to_update = false;
	}

	void on_receive()
	{
		//Serial.println("on_receive");
		if (!actual)
			return;

		byte rxBuf[64] = {0};
		byte rxLen = 0;

		bool recv = radio->isPacketReceived();
		if (recv)
		{
			digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
			radio->getPacketReceived(&rxLen, rxBuf);
			Serial.print(millis());
			Serial.print(" rxLen:");
			Serial.println(rxLen, DEC);

			for (int i = 0; i < rxLen; ++i)
			{
				Serial.print(rxBuf[i], HEX);
				Serial.print(" ");
			}
			Serial.println();

			for (int i = 0; i < rxLen; ++i)
			{
				char c = rxBuf[i];
				if (c > 0x20 && c < 0xFF)
				{
					Buf[i] = c;
					Serial.print(c);
				}
				else
				{
					
					Serial.print(" ");


					Buf[i] = ' '; 	// Space


				}
			}
			Serial.println();

			radio->startListening(); // restart the listening.

			//std::copy(std::begin(Buf),  Buf+20,  std::begin(rxBuf));
			to_update = true;
		}

		tasks->add_task(&on_receive_f, millis() + 50);
		//Serial.println("end_receive");
	}

	item *event(const buttons_name name) override
	{ // нажатие кнопки например

		if (name == cancel)
		{
			actual = false;
			return prev_item; // return //item_selected = (item_num + item_selected - 1) % item_num;
		}
		else
		{
			return this;
		}
	}
};

// Класс-меню. От него наследуются все вкладки
class GUI
{
	item *current_item = nullptr;
	bool overwrite = true;								 // нужно ли обновить (заново отрисовать) дисплей
	U8G2_SSD1306_128X64_NONAME_F_HW_I2C *u8g2 = nullptr; // экземпляр класса, с помощью которого управляется дисплей

public:
	GUI(U8G2_SSD1306_128X64_NONAME_F_HW_I2C *_u8g2, menu *_current_item) : u8g2(_u8g2), current_item(_current_item) {}
	//
	void draw()
	{
		if (overwrite == false and current_item->to_update == false)
			return; // не нужно обновлять дисплей
		u8g2->clear();
		overwrite = false;

		current_item->draw(u8g2);
		current_item->to_update = false;
	}

	void interrupt(const buttons_name name)
	{
		digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
		// Serial.println("interrupt");
		// u8g2->clear();
		current_item = current_item->event(name);

		overwrite = true;
	}
};


#endif

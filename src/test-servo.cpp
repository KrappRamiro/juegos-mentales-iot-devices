#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include <Arduino.h>
#include <Servo.h>

Servo servo;
int angle = 0;
bool backwards = false;
void setup()
{
	Serial.begin(115200);
	while (!Serial) {
		;
	}
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
	servo.attach(13);
}

void loop()
{
	/*
	servo.write(angle);
	if (!backwards)
		angle += 5;
	else
		angle -= 5;
	if (angle == 180) {
		backwards = true;
	} else if (angle == 0) {
		backwards = false;
	}
	Serial.println(angle);
	delay(1000);
	*/
	servo.write(180);
	delay(1000);
	servo.write(130);
	delay(1000);
	servo.write(80);
	delay(1000);
	servo.write(30);
	delay(1000);
	servo.write(0);
	delay(1000);
}

/*
Final: 0
0 180
1 130
2 80
3 30
explode: 0
*/
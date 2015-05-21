/**
 * DIY-HOTT-GPS is a standalone Arduino based Application that acts 
 * as a HoTTv4 capable device to transmit GPS/Vario information. 
 * Code contains parts by Oliver Bayer, Carsten Giesen, Jochen Boenig, Mikal Hart, ZiegeOne and Stefan Muerzl 03/2015
 * tested by Robert aka Skyfighter THANKS!
 * 
 * Before compiling check you Baudrate in void setup()!
 * Vor dem Compilieren bitte in der void setup() die Baudrate an euer GPS anpassen!
 */

#include "SoftwareSerial.h"
#include <avr/io.h> 
#include "TinyGPS.h"

//#define Vario
//#define EAM

TinyGPS gps; 

float f_HOME_LAT = 0, f_HOME_LON = 0;
float start_height = 0;

bool is_set_home = 0;
uint32_t last = 0;
int p_alt[4]={0,0,0,0};

//Variables for GPS-Functions
unsigned long speed_k=0; // Knots * 100
//long lat=0, lon=0;
long lati=0, loni=0;
float flat=0, flon=0;
unsigned long age=0, dat=0, tim=0, ui_course=0;
int alt=0;
unsigned int numsat=0; 
float alt_offset = 500;
bool bGpsUpdate = false;
uint32_t now = millis(); 

// status LED
#define	POWER_ON		0
#define	DATA_FROM_GPS	1
#define	FIX_FROM_GPS	2
#define	HOTT_GOT_DATA	3
uint8_t DiyHottStatus = POWER_ON;


uint16_t GPS_distanceToHome;

struct {
  
  //uint8_t  GPS_fix;
  //uint8_t  GPS_numSat;
  float GPS_latitude;
  float GPS_longitude;
  //uint16_t GPS_altitude;
  //uint16_t GPS_speed;
  //uint16_t GPS_Vario1;
  //uint8_t  GPS_Vario3;
  //uint8_t  GPS_flightDirection;
  
  //uint8_t  GPS_directionToHome;
  //uint8_t  GPS_alarmTone;
  int32_t altitude;
  
} MultiHoTTModule;

#define LED 13 

void CheckGpsData( void );

void setup( void ) 
{
  
  pinMode(LED, OUTPUT);
  Serial.begin(57600);

  #ifdef Vario
	setupAltitude();
  #endif
  
  #ifdef EAM
    setupEAM();
  #endif
  
  hottV4Setup();
  is_set_home = 0;
  p_alt[0]=0;
  p_alt[1]=0;
  p_alt[2]=0;
  p_alt[3]=0;  
}

void loop( void ) 
{

	CheckGpsData();
   if( bGpsUpdate )
   {
	//gps.get_position(&lat, &lon, &age);
	gps.get_position2(&lati, &loni, &age);
	gps.f_get_position(&flat, &flon);
	bGpsUpdate = false;
   }
   
   #ifdef Vario
	  alt = readAltitude();
   #else
	  alt =  gps.altitude()/100;
   #endif
   
   now = millis();
   if ((now - last) > 1000) //measure every second for Vario function
   {  
     last = now;
     p_alt[3] = p_alt[2];
     p_alt[2] = p_alt[1];
     p_alt[1] = p_alt[0];
     p_alt[0] = alt;     
   }
   
   sethome();
   hottV4SendTelemetry();
   setStatusLED();
}

void CheckGpsData( void )
{
	while ( Serial.available()>0 )
   {
	   gps.encode( Serial.read() );
	   if( true )
	   {
			bGpsUpdate = true;
			if( DiyHottStatus < DATA_FROM_GPS )
			{
				DiyHottStatus = DATA_FROM_GPS;
			}
	   }
	}

	// debug:
   static unsigned long lastchars=0;
   unsigned long chars;
   unsigned short good_sentences, failed_cs;
   gps.stats( &chars, &good_sentences, &failed_cs);
   if( (chars - lastchars) > 300 )
   {
	Serial.print( "stats: " ); 
	Serial.print( chars );
	Serial.print( " / " ); 
	Serial.print( good_sentences ); 
	Serial.print( " / " ); 
	Serial.println( failed_cs ); 
	lastchars = chars;
	}
}

void setStatusLED( void )
{
	static uint32_t ui32ToggleTime = 0;
	if( millis() > ui32ToggleTime )	// ignore overrun
	{
		digitalWrite(LED, !digitalRead(LED)); 
		switch( DiyHottStatus )
		{
			default:
			case POWER_ON:
				ui32ToggleTime = millis() + 3000;
				break;
			case DATA_FROM_GPS:
				ui32ToggleTime = millis() + 2000;
				break;
			case FIX_FROM_GPS:
				ui32ToggleTime = millis() + 1000;
				break;
			case HOTT_GOT_DATA:
				ui32ToggleTime = millis() + 200;
				break;
		};
	}
}

void sethome()
{
   if (is_set_home == 0 && gps.satellites() >= 6)  // we need more than 5 sats
   {
       
       f_HOME_LAT = flat;
       f_HOME_LON = flon;
       start_height = alt;	   //in future height will be set with BMP085
       p_alt[0]=alt;
       p_alt[1]=alt;
       p_alt[2]=alt;
       p_alt[3]=alt; 

	   if ((gps.altitude()/100) != 9999999)	
	   {
		is_set_home = 1;
		if( DiyHottStatus < FIX_FROM_GPS )
		{
			DiyHottStatus = FIX_FROM_GPS;
		}
	   }	
   }  
}

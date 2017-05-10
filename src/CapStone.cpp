// Capstone project
// SEIS744 IoT
// Charles Christianson
// Air quality sensor/indicator
// v.91

LEDStatus showStatusGreen(RGB_COLOR_GREEN, LED_PATTERN_FADE);
LEDStatus showStatusYellow(RGB_COLOR_YELLOW, LED_PATTERN_SOLID);
LEDStatus showStatusOrange(RGB_COLOR_ORANGE, LED_PATTERN_SOLID);
LEDStatus showStatusRed(RGB_COLOR_RED, LED_PATTERN_SOLID);
LEDStatus showStatusMagenta(RGB_COLOR_MAGENTA, LED_PATTERN_BLINK);
LEDStatus showStatusWhite(RGB_COLOR_WHITE, LED_PATTERN_BLINK);

#include <google-maps-device-locator.h>

GoogleMapsDeviceLocator locator;
const int MAX_DATA_LENGTH = 255;
//Default to New York City
String latitude = "40.7141667";
String longitude = "-74.0063889";

//String latitude = "39.9075000";
//String longitude = "116.3972300";

bool toggleInternet = true;
bool toggleSensor = false;
int button1 = D1;
int button2 = D2;

int led = D7;

int scanInterval = 30000;
int intervalStart = 0;
bool inInterval = false;

int personalAirQualityGrade = 0;
String myID = System.deviceID();

void setup() {
    pinMode(button1, INPUT_PULLDOWN);
    pinMode(button2, INPUT_PULLDOWN);
    pinMode(led, OUTPUT);
    // Open connection to host serial for local monitoring (optional)
    Serial.begin(9600);
    // Open connection to Digital Air-sensor board via UART (Serial1)
    Serial1.begin(9600, SERIAL_8N1);
    // Scan for visible networks and publish to the cloud every 30 seconds
    // Pass the returned location to be handled by the locationCallback() method
    locator.withSubscribe(locationCallback).withLocatePeriodic(60);
    Particle.subscribe(myID+"_AQI_EVENT_BREEZOMETER",myHandler, MY_DEVICES);
}

void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  // - Latitude
  // - Longitude
  // - Accuracy of estimated location (in meters)
  Serial.println("Entering locationCallback");
  latitude = String(lat);
  longitude = String(lon);
  Serial.println("\tlocation updated: " + latitude + ", " + longitude);
  //we are ignoring accuracy at the moment!
  Serial.println("Exiting locationCallback");
}

void loop() {
    Serial.print("Entering main loop @ ");
     Serial.println(Time.timeStr());
    Serial.println("Check location");
    // Send Control to the location Sensor code
    locator.loop();  //update the location
    
    //Initialize data string buffer
    char sensor[MAX_DATA_LENGTH] = {""};
    String sensorDataJson = "";
    
    Serial.println("Begin sensor data");
    if (Serial1.available() && toggleSensor)    // Make sure the UART is active
    {
        Serial.println("Sensor UART Available");   // Just a debug heads-up
        Serial1.write('c');     // This is an actuator signal - basically any character sent. (Sending a c within 4 seconds would result in continuous data send)
        // The sensor will send data bytes that end with /r
        Serial1.readBytesUntil('\r',sensor,MAX_DATA_LENGTH);
        // convert raw chars to json with field names (also append any other data needed)
        sensorDataJson = createJson(String(sensor) + "," + latitude + "," + longitude);
    }
    else
    {
        Serial.println("Sensor is Disabled!");
        //just in standby mode
    }
    //Serial.print("Toggle State 1 = ");
    //Serial.println(toggleState1);
    
    if (WiFi.ready() && toggleInternet) //this simulates network down
    {
        //digitalWrite(led, LOW); // indicate internet data
        //Upload the local data for analysis
        if(toggleSensor)
        {
            Particle.publish("SensorData", sensorDataJson , PRIVATE);
            setAirQualityIndicator(personalAirQualityGrade);
        }
        else //sensor is off, so get our data from the internet
        {
            // Check to update the current local air quality status
            Particle.publish("GET_AQI_BREEZOMETER", "{\"coords\":{\"lat\":" + latitude + ",\"lon\":" + longitude + "}}"); // request the air quality value for this location
        }
    }
    else
    {
        Serial.println("\tNot connected to Internet! Cannot publish data!");
        //digitalWrite(led, HIGH); // indicate personal data
        // insert local storage here.
        //also use local indicator
        if(toggleSensor)
        {
            showStatusWhite.setActive(false);
            setAirQualityIndicator(personalAirQualityGrade);
        }
        else
        {
            showStatusWhite.setActive(true);
            Serial.println("No Internet and no Sensor data! Can't display AQ Grade!");
        }
    }
    // Waiting - this is a crude way to manage frequency of data reads - but good enough for now
    
    
    
    
    //delay(30000);
    Serial.println("Begin delay loop...");
    
    int lastStatus1 = LOW;
    int lastStatus2 = LOW;
    for(int l = 0; l < 300; l++)
    {
        // check buttons
        if (digitalRead(button1) == HIGH )
        {
            if(lastStatus1 == HIGH)
            {
            //Button pressed;
                toggleInternet = !toggleInternet;
                Serial.print("\tButton 1 pressed: toggle is now: ");
                Serial.println(toggleInternet);
                lastStatus1 = LOW;
                //delay(100);
                //break;
            }
            else
            {
                //not yet....
                lastStatus1 = HIGH;
            }
        }
        
        if (digitalRead(button2) == HIGH )
        {
            if(lastStatus2 == HIGH)
            {
                //Button pressed;
                toggleSensor = !toggleSensor;
                Serial.print("\tButton 2 pressed: toggle is now: ");
                Serial.println(toggleSensor);
                lastStatus2 = LOW;
                if(toggleSensor)
                {
                    digitalWrite(led, HIGH);
                }
                else
                {
                    digitalWrite(led, LOW);
                }
                //delay(100);
                //break;
            }
            else
            {
                // not yet...next time yes!
                lastStatus2 = HIGH;
            }
        }        
        
        delay(100);
    }
    Serial.println("End delay loop...");

    Serial.println("Exiting main loop.");
}

void myHandler(const char *eventName, const char *data)
{
    Serial.println("Entered myHandler");
    const int _BREEZEOMETER_AQI_EXCELLENT = 80;
    const int _BREEZEOMETER_AQI_FAIR = 60;
    const int _BREEZEOMETER_AQI_MODERATE = 40;
    const int _BREEZEOMETER_AQI_LOW = 20;
    const int _BREEZEOMETER_AQI_POOR = 20;
    int _genericAirQualityGrade = 0;
    int _breezometer_AQI = 0;
    
    Serial.print("\tData received: ");
    Serial.println(data);
    
    _breezometer_AQI = String(data).trim().toInt();
    Serial.print("\tConverted Integer Value: ");
    Serial.println(_breezometer_AQI);
    Serial.print("\t");
   if (_breezometer_AQI >= _BREEZEOMETER_AQI_EXCELLENT)
   {
       //set quality to 5
       _genericAirQualityGrade = 5;
       Serial.println("Air Quality Excellent");
   }
   else if (_breezometer_AQI >= _BREEZEOMETER_AQI_FAIR)
   {
       //set quality to 4
       _genericAirQualityGrade = 4;
       Serial.println("Air Quality Fair");
   }
   else if (_breezometer_AQI >= _BREEZEOMETER_AQI_MODERATE)
   {
        //set quality to 3
        _genericAirQualityGrade = 3;
        Serial.println("Air Quality Moderate");
   }
   else if (_breezometer_AQI >= _BREEZEOMETER_AQI_LOW)
   {
        //set quality to 2
        _genericAirQualityGrade = 2;
        Serial.println("Air Quality Low");
   }
    else if (_breezometer_AQI >= _BREEZEOMETER_AQI_POOR)
   {
       //set quality to 1
       _genericAirQualityGrade = 1;
       Serial.println("Air Quality Poor");
   }
   
   setAirQualityIndicator(_genericAirQualityGrade);
   Serial.println("Exiting myHandler");
}

void setAirQualityIndicator(int _airQualityGrade)
{
    
    switch (_airQualityGrade)
    {
        case 5:
            showStatusGreen.setActive(true);
            showStatusYellow.setActive(false);
            showStatusOrange.setActive(false);
            showStatusRed.setActive(false);
            showStatusMagenta.setActive(false);
            showStatusWhite.setActive(false);
            break;    
        case 4:
            showStatusYellow.setActive(true);
            showStatusGreen.setActive(false);              
            showStatusOrange.setActive(false);
            showStatusRed.setActive(false);
            showStatusMagenta.setActive(false);
            showStatusWhite.setActive(false);
            break;
        case 3:    
            showStatusOrange.setActive(true);
            showStatusGreen.setActive(false);   
            showStatusYellow.setActive(false);
            showStatusRed.setActive(false);
            showStatusMagenta.setActive(false);
            showStatusWhite.setActive(false);
            break;
        case 2:    
            showStatusRed.setActive(true);
            showStatusGreen.setActive(false);   
            showStatusYellow.setActive(false);
            showStatusOrange.setActive(false);
            showStatusMagenta.setActive(false);
            showStatusWhite.setActive(false);
            break;
        case 1:    
            showStatusMagenta.setActive(true);
            showStatusGreen.setActive(false);   
            showStatusYellow.setActive(false);
            showStatusOrange.setActive(false);
            showStatusRed.setActive(false);
            showStatusWhite.setActive(false);
            break;
        default:
            showStatusGreen.setActive(false);   
            showStatusYellow.setActive(false);
            showStatusOrange.setActive(false);
            showStatusRed.setActive(false);
            showStatusMagenta.setActive(false);
            showStatusWhite.setActive(false);
    }
    
}

void setLocalAirQualityGrade(int _ppb)
{
    Serial.println("Setting local air quality");
    if (_ppb > 150) { personalAirQualityGrade = 1; Serial.println("Sensor Air Quality Poor"); }
    else if (_ppb > 100) { personalAirQualityGrade = 2; Serial.println("Sensor Air Quality Low"); }
    else if (_ppb > 50) { personalAirQualityGrade = 3; Serial.println("Sensor Air Quality Moderate"); }
    else if (_ppb > 10) { personalAirQualityGrade = 4; Serial.println("Sensor Air Quality Fair"); }
    else if (_ppb <= 10) { personalAirQualityGrade = 5; Serial.println("Sensor Air Quality Excellent"); }
}

String createJson(String _string)
{
    Serial.println("Entering createJson");
    Serial.println("Raw data:");
    Serial.println(_string);
    
    //Trim leading "\n"
    if(_string.charAt(0) == '\n')
    {
        _string.remove(0,1);
    }
    
    // Full data list
    const int _NAME_COUNT = 13;
    
    //String _fieldNames[_nameCount] = {"SN","PPB","TEMP","RH","RawSensor","TempDigital","RHDigital","Day","Hour","Minute","Second"};
    
    String _fieldNames[] = {"SN","PPB","TEMP","RH","RawSensor","TempDigital","RHDigital","Day","Hour","Minute","Second","Latitude","Longitude"};
    
    int _startIndex = 0;
    int _tempIndex = 0;
    float _upTime = 0;
    
    String _tempString = "{\"sensor\":{";
    
    for(int i = 0; i < _NAME_COUNT; i++)
    {
        _tempIndex = _string.indexOf(',', _startIndex);
        if(_tempIndex != -1)
        {
            _tempString.concat(String("\""));
            _tempString.concat(_fieldNames[i]);
            _tempString.concat(String("\":\""));
            _tempString.concat(_string.substring(_startIndex, _tempIndex).trim());
            _tempString.concat(String("\","));
            if (_fieldNames[i] == "Day")
            {
                _upTime += _string.substring(_startIndex, _tempIndex).trim().toFloat() * 24;
            }
            else if (_fieldNames[i] == "Hour")
            {
                _upTime += _string.substring(_startIndex, _tempIndex).trim().toFloat();
            }
            else if (_fieldNames[i] == "Minute")
            {
                _upTime += _string.substring(_startIndex, _tempIndex).trim().toFloat() / 60;
            }
            else if (_fieldNames[i] == "Second")
            {
                _upTime += _string.substring(_startIndex, _tempIndex).trim().toFloat() / 3600;
                _tempString.concat(String("\""));
                _tempString.concat("UpTime");
                _tempString.concat(String("\":\""));
                _tempString.concat(String(_upTime));
                _tempString.concat(String("\","));
            }
            else if (_fieldNames[i] == "PPB")
            {
               setLocalAirQualityGrade(_string.substring(_startIndex, _tempIndex).trim().toInt());
            }
            
            _startIndex = _tempIndex + 1;
        }
        else
        {
            // we're done
            if(_startIndex < _string.length())
            {
                _tempIndex = _string.length();
                _tempString.concat(String("\""));
                _tempString.concat(_fieldNames[i]);
                _tempString.concat(String("\":\""));
                _tempString.concat(_string.substring(_startIndex, _tempIndex).trim());
                _tempString.concat(String("\""));
            }
        }
        //follwing lines useed for debugging.
        //Serial.println(i);
        //Serial.println(_tempIndex);
    }
    _tempString.concat(String("}}"));
    Serial.println(_tempString);
    
    // SN [XXXXXXXXXXXX]
    // PPB [0 : 999999]
    // TEMP [-99:99]
    // RH [0:99]
    // RawSensor [ADCCount]
    // TempDigital
    // RHDigital
    // Day [0:99]
    // Hour [0:23]
    // Minute [0:59]
    // Second [0:59]
    Serial.println("Exiting createJson");
    return _tempString;
}

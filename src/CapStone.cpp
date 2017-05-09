// Capstone project
// SEIS744 IoT
// Charles Christianson
// AIr quality sensor/indicator
// v.5

LEDStatus showStatusGreen(RGB_COLOR_GREEN, LED_PATTERN_FADE);
LEDStatus showStatusYellow(RGB_COLOR_YELLOW, LED_PATTERN_SOLID);
LEDStatus showStatusOrange(RGB_COLOR_ORANGE, LED_PATTERN_SOLID);
LEDStatus showStatusRed(RGB_COLOR_RED, LED_PATTERN_SOLID);
LEDStatus showStatusMagenta(RGB_COLOR_MAGENTA, LED_PATTERN_BLINK);

#include <google-maps-device-locator.h>

GoogleMapsDeviceLocator locator;
const int MAX_DATA_LENGTH = 255;
String latitude = "40.7141667";
String longitude = "-74.0063889";

void setup() {
    // Open connection to host serial for local monitoring (optional)
    Serial.begin(9600);
    // Open connection to Digital Air-sensor board via UART (Serial1)
    Serial1.begin(9600, SERIAL_8N1);
    // Scan for visible networks and publish to the cloud every 30 seconds
    // Pass the returned location to be handled by the locationCallback() method
    locator.withSubscribe(locationCallback).withLocatePeriodic(60);
    Particle.subscribe("AQI_EVENT_BREEZOMETER",myHandler, MY_DEVICES);
}

void locationCallback(float lat, float lon, float accuracy) {
  // Handle the returned location data for the device. This method is passed three arguments:
  // - Latitude
  // - Longitude
  // - Accuracy of estimated location (in meters)
  latitude = String(lat);
  longitude = String(lon);
  Particle.publish("Mylocation updated: " + latitude + ", " + longitude);
}

void loop() {
    // Send Control to the location Sensor code
    Serial.println(Time.timeStr());
    locator.loop();  //update the location
    
    //Initialize data string buffer
    char sensor[MAX_DATA_LENGTH] = {""};
    
    if (Serial1.available())    // Make sure the UART is active
    {
        Particle.publish("Serial Available");   // Just a debug heads-up
        Serial1.write('c');     // This is an actuator signal - basically any character sent. (Sending a c within 4 seconds would result in continuous data send)
        // The sensor will send data bytes that end with /r
        Serial1.readBytesUntil('\r',sensor,MAX_DATA_LENGTH);
        //Serial1.read();
        //convert the string to Json
        // Particle.publish("UART",String(sensor),60,PRIVATE); //This is the RAW data string from the UART. mainly for troublshooting.
        // Particle.publish("UART_JSON",convert2JSON(String(sensor)),60,PRIVATE);  // This is the data parsed into JSON.
        // Trigger the integration
        Particle.publish("SensorData", convert2JSON(String(sensor) + "," + latitude + "," + longitude), PRIVATE);
        
    }
    // Check to update the current local air quality status
    Particle.publish("GET_AQI_BREEZOMETER", "{\"coords\":{\"lat\":" + latitude + ",\"lon\":" + longitude + "}}"); // request the air quality value for this location
    // Waiting - this is a crude way to manage frequency of data reads - but good enough for now
    delay(30000);
}

void myHandler(const char *eventName, const char *data)
{
    const int _BREEZEOMETER_AQI_EXCELLENT = 80;
    const int _BREEZEOMETER_AQI_FAIR = 60;
    const int _BREEZEOMETER_AQI_MODERATE = 40;
    const int _BREEZEOMETER_AQI_LOW = 20;
    const int _BREEZEOMETER_AQI_POOR = 20;
    int _genericAirQualityGrade = 0;
    int _breezometer_AQI = 0;
    
    Serial.println("Entered myHandler");
    Serial.print("\tData receveived: ");
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
            break;    
        case 4:
            showStatusYellow.setActive(true);
            break;
        case 3:    
            showStatusOrange.setActive(true);
            break;
        case 2:    
            showStatusRed.setActive(true);
            break;
        case 1:    
            showStatusMagenta.setActive(true);
            break;
        default:
            showStatusGreen.setActive(false);   
            showStatusYellow.setActive(false);
            showStatusOrange.setActive(false);
            showStatusRed.setActive(false);
            showStatusMagenta.setActive(false);
    }
    
}

String convert2JSON(String _string)
{
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
    
    return _tempString;
}

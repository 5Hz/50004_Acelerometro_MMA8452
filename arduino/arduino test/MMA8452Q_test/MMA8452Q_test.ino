#include <Wire.h>

/*
codigo test para el acelerometro mma8452q
generado en electronica 5Hz por Baruc
*/

//-----------------------------------------------------------------------------------------definiciones
//direccion del sensor
//se elige 1 si el jumper SA0 no esta soldado, y 0 si no lo esta
#define SA0 1
#if SA0
#define MMA8452_ADDRESS 0x1D  // SA0 is high, 0x1C if low
#else
#define MMA8452_ADDRESS 0x1C
#endif

#define SCALE 2 //define el rango a +/- 2, 4 u 8g
#define dataRate 0 //define la velocidad de refresco de la informacion, debe estar entre 0 y 7
// 0=800Hz, 1=400Hz, 2=200Hz, 3=100Hz, 4=50Hz, 5=12.5Hz, 6=6.25Hz, 7=1.56Hz

//--------------------------------------------------------------------------------definiciones de pines
int pinInt1 = 2;
int pinInt2 = 3;

//-------------------------------------------------------------------definiciones de variables globales

int accelCount[3]; //almacena el valor de 12 bits
float accelG[3]; //almacena  el valor real de aceleracion en g's

//-----------------------------------------------------------------------------prototipos de funciones

void readAccelData(int * _dest_);
void tapHandler();
void portraitLandscapeHandler();
void MMA8452Setup();
void MMA8452inactivo();
void MMA8452activo();
void leeRegistros(byte _address_, int _i_, byte * _dest_);
byte leeRegistro(byte _address_);
void escribeRegistro(byte _address_, byte _data_);

//--------------------------------------------------------------------------------------configuracion
void setup()
{
  byte c;
  
  Serial.begin(9600);
  Wire.begin();
  // Set up the interrupt pins, they're set as active high, push-pull
  pinMode(pinInt1, INPUT);
  digitalWrite(pinInt1, LOW);
  pinMode(pinInt2, INPUT);
  digitalWrite(pinInt2, LOW);
  c = leeRegistro(0x0D);
  // lee el registro WHO_I_AM, esta es una buena comprobacion para la comunicacion
  if (c == 0x2A) // lee el registro WHO_AM_I, siempre debe ser 0x2A
  {  
    MMA8452Setup();  // inicia el acelerometro si la comunicacion se establecio correctamente
    Serial.println("MMA8452Q is online...");
  }
  else
  {
    Serial.print("Could not connect to MMA8452Q: 0x");
    Serial.println(c, HEX);
    while(1) ; // no llega al ciclo principal si la comunicacion no se estableció
  }
}

//######################################################################################ciclo principal
void loop()
{
  static byte source;

  // If int1 goes high, all data registers have new data
  if (digitalRead(pinInt1)==1)  // Interrupt pin, should probably attach to interrupt function
  {
    readAccelData(accelCount);  // Read the x/y/z adc values

    /* 
     //Below we'll print out the ADC values for acceleration
     for (int i=0; i<3; i++)
     {
     Serial.print(accelCount[i]);
     Serial.print("\t\t");
     }
     Serial.println();
     */

    // Now we'll calculate the accleration value into actual g's
    for (int i=0; i<3; i++)
      accelG[i] = (float) accelCount[i]/((1<<12)/(2*SCALE));  // get actual g value, this depends on scale being set
    // Print out values
    for (int i=0; i<3; i++)
    {
      Serial.print(accelG[i], 4);  // Print g values
      Serial.print("\t\t");  // tabs in between axes
    }
    Serial.println();
  }

  // If int2 goes high, either p/l has changed or there's been a single/double tap
  if (digitalRead(pinInt2)==1)
  {
    source = leeRegistro(0x0C);  // Read the interrupt source reg.
    if ((source & 0x10)==0x10)  // If the p/l bit is set, go check those registers
      portraitLandscapeHandler();
    else if ((source & 0x08)==0x08)  // Otherwise, if tap register is set go check that
      tapHandler();
  }
  delay(100);  // Delay here for visibility
}
//##############################################################################fin del ciclo principal

//-------------------------------------------------------------------------------------------funciones
void readAccelData(int * _dest_)
{
  byte _rawData_[6];  // x/y/z accel register data stored here

  leeRegistros(0x01, 6, &_rawData_[0]);  // Read the six raw data registers into data array

  // Loop to calculate 12-bit ADC and g value for each axis
  for (int i=0; i<6; i+=2)
  {
    _dest_[i/2] = ((_rawData_[i] << 8) | _rawData_[i+1]) >> 4;  // Turn the MSB and LSB into a 12-bit value
    if (_rawData_[i] > 0x7F)
    {  
      // If the number is negative, we have to make it so manually (no 12-bit data type)
      _dest_[i/2] = ~_dest_[i/2] + 1;
      _dest_[i/2] *= -1;  // Transform into negative 2's complement #
    }
  }
}

void tapHandler()
{
    byte source = leeRegistro(0x22);  // lee el registro PULSE_SRC

  if ((source & 0x10)==0x10)  // Si el bit AxX esta en alto
  {
    if ((source & 0x08)==0x08)  // Si el bit DPE (doble tap) esta en alto
      Serial.print("    Double Tap (2) on X");
    else
      Serial.print("Single (1) tap on X");

    if ((source & 0x01)==0x01)  // Si el bit PoIX esta en alto
      Serial.println(" +");
    else
      Serial.println(" -");
  }
  if ((source & 0x20)==0x20)  // Si el bit AxY esta en alto
  {
    if ((source & 0x08)==0x08)  // Si el bit DPE (doble tap) esta en alto
      Serial.print("    Double Tap (2) on Y");
    else
      Serial.print("Single (1) tap on Y");

    if ((source & 0x02)==0x02)  // If PoIY is set
      Serial.println(" +");
    else
      Serial.println(" -");
  }
  if ((source & 0x40)==0x40)  // Si el bit AxZ esta en alto
  {
    if ((source & 0x08)==0x08)  // Si el bit DPE (doble tap) esta en alto
      Serial.print("    Double Tap (2) on Z");
    else
      Serial.print("Single (1) tap on Z");
    if ((source & 0x04)==0x04)  // Si el bit PoIZ esta en alto
      Serial.println(" +");
    else
      Serial.println(" -");
  }
}

void portraitLandscapeHandler()
{
  byte _PL_ = leeRegistro(0x10);  // Reads the PL_STATUS register
  switch((_PL_&0x06)>>1)  // Check on the LAPO[1:0] bits
  {
  case 0:
    Serial.print("Portrait up, ");
    break;
  case 1:
    Serial.print("Portrait Down, ");
    break;
  case 2:
    Serial.print("Landscape Right, ");
    break;
  case 3:
    Serial.print("Landscape Left, ");
    break;
  }
  if (_PL_&0x01)  // Check the BAFRO bit
    Serial.print("Back");
  else
    Serial.print("Front");
  if (_PL_&0x40)  // Check the LO bit
    Serial.print(", Z-tilt!");
  Serial.println();
}

void MMA8452Setup()
{
    MMA8452inactivo();  // Must be in standby to change registers

  // selecciona el rango a una escala de +/- 2, 4 u 8g
  if ((SCALE==2)||(SCALE==4)||(SCALE==8))
    escribeRegistro(0x0E, SCALE >> 2);  
  else
    escribeRegistro(0x0E, 0);

  // Setup the 3 data rate bits, from 0 to 7
  escribeRegistro(0x2A, leeRegistro(0x2A) & ~(0x38));
  if (dataRate <= 7)
    escribeRegistro(0x2A, leeRegistro(0x2A) | (dataRate << 3));  

  // Set up portrait/landscap registers - 4 steps:
  // 1. Enable P/L
  // 2. Set the back/front angle trigger points (z-lock)
  // 3. Set the threshold/hysteresis angle
  // 4. Set the debouce rate
  // For more info check out this app note: http://cache.freescale.com/files/sensors/doc/app_note/AN4068.pdf
  escribeRegistro(0x11, 0x40);  // 1. Enable P/L
  escribeRegistro(0x13, 0x44);  // 2. 29deg z-lock (don't think this register is actually writable)
  escribeRegistro(0x14, 0x84);  // 3. 45deg thresh, 14deg hyst (don't think this register is writable either)
  escribeRegistro(0x12, 0x50);  // 4. debounce counter at 100ms (at 800 hz)

  /* Set up single and double tap - 5 steps:
   1. Set up single and/or double tap detection on each axis individually.
   2. Set the threshold - minimum required acceleration to cause a tap.
   3. Set the time limit - the maximum time that a tap can be above the threshold
   4. Set the pulse latency - the minimum required time between one pulse and the next
   5. Set the second pulse window - maximum allowed time between end of latency and start of second pulse
   for more info check out this app note: http://cache.freescale.com/files/sensors/doc/app_note/AN4072.pdf */
  escribeRegistro(0x21, 0x7F);  // 1. enable single/double taps on all axes
  // writeRegister(0x21, 0x55);  // 1. single taps only on all axes
  // writeRegister(0x21, 0x6A);  // 1. double taps only on all axes
  escribeRegistro(0x23, 0x20);  // 2. x thresh at 2g, multiply the value by 0.0625g/LSB to get the threshold
  escribeRegistro(0x24, 0x20);  // 2. y thresh at 2g, multiply the value by 0.0625g/LSB to get the threshold
  escribeRegistro(0x25, 0x08);  // 2. z thresh at .5g, multiply the value by 0.0625g/LSB to get the threshold
  escribeRegistro(0x26, 0x30);  // 3. 30ms time limit at 800Hz odr, this is very dependent on data rate, see the app note
  escribeRegistro(0x27, 0xA0);  // 4. 200ms (at 800Hz odr) between taps min, this also depends on the data rate
  escribeRegistro(0x28, 0xFF);  // 5. 318ms (max value) between taps max

  // Set up interrupt 1 and 2
  escribeRegistro(0x2C, 0x02);  // Active high, push-pull interrupts
  escribeRegistro(0x2D, 0x19);  // DRDY, P/L and tap ints enabled
  escribeRegistro(0x2E, 0x01);  // DRDY on INT1, P/L and taps on INT2

  MMA8452activo();  // Set to active to start reading
}

void MMA8452inactivo()
{
  escribeRegistro(0x2A, leeRegistro(0x2A) &  ~(0x01));
}

void MMA8452activo()
{
  escribeRegistro(0x2A, leeRegistro(0x2A) | 0x01);
}

void leeRegistros(byte _address_, int _i_, byte * _dest_)
{
  Wire.beginTransmission(MMA8452_ADDRESS);  //inicia comunicacion con sensor elegido
  Wire.write(_address_);                    //escribe el registro del cual solicitas la informacion
  Wire.endTransmission();                   //termina la transmision
  
  Wire.beginTransmission(MMA8452_ADDRESS);  //inicia comunicacion con sensor elegido
  Wire.requestFrom(MMA8452_ADDRESS, _i_);   //solicita 1 byte del sensor elegido (previamente elegido)
  for(int _j_ = 0; _j_ < _i_; _j_++)
  {
    do{}while(Wire.available() < 1);          //espera a recibir la informacion del registro
  _dest_[_j_] = Wire.read();                //regresa el valor leido del registro al programa principal
  }
  Wire.endTransmission();                   //termina la transmision
}

byte leeRegistro(byte _address_)
{
  Wire.beginTransmission(MMA8452_ADDRESS);  //inicia comunicacion con sensor elegido
  Wire.write(_address_);                    //escribe el registro del cual solicitas la informacion
  Wire.endTransmission();                   //termina la transmision
  
  Wire.beginTransmission(MMA8452_ADDRESS);  //inicia comunicacion con sensor elegido
  Wire.requestFrom(MMA8452_ADDRESS, 1);     //solicita 1 byte del sensor elegido (previamente elegido)
  do{}while(Wire.available() < 1);          //espera a recibir la informacion del registro
  byte _value_ = Wire.read();
  Wire.endTransmission();                   //termina la transmision
  
  return _value_;                       //regresa el valor leido del registro al programa principal
}

void escribeRegistro(byte _address_, byte _data_)
{
  Wire.beginTransmission(MMA8452_ADDRESS);  //inicia comunicacion con el sensor seleccionado
  Wire.write(_address_);                    //selecciona el registro al cual escribir
  Wire.write(_data_);                       //escribe el valor al registro seleccionado
  Wire.endTransmission();                   //termina la transmision
}

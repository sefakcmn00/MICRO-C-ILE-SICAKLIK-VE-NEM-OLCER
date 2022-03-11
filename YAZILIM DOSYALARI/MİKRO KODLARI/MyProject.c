


// SD kart �ipi pin ba�lant�s�n� se�
sbit Mmc_Chip_Select           at RC2_bit;
sbit Mmc_Chip_Select_Direction at TRISC2_bit;

// DHT22 sens�r ba�lant�s� (burada data pini RB2 pinine ba�l�d�r)
#define DHT22_PIN         RB2_bit
#define DHT22_PIN_DIR     TRISB2_bit

// LCD module ba�lant�s�
sbit LCD_RS at RD2_bit;
sbit LCD_EN at RD3_bit;
sbit LCD_D4 at RD4_bit;
sbit LCD_D5 at RD5_bit;
sbit LCD_D6 at RD6_bit;
sbit LCD_D7 at RD7_bit;
sbit LCD_RS_Direction at TRISD2_bit;
sbit LCD_EN_Direction at TRISD3_bit;
sbit LCD_D4_Direction at TRISD4_bit;
sbit LCD_D5_Direction at TRISD5_bit;
sbit LCD_D6_Direction at TRISD6_bit;
sbit LCD_D7_Direction at TRISD7_bit;


// button tan�mlar�
#define button1      RB0_bit   // B1 butonu RB0 pinine ba�land�
#define button2      RB1_bit   // B2 butonu RB1 pinine ba�land�

#define DS3231_I2C2    // use hardware I2C2 moodule (MSSP2) for DS3231 RTC
#include <DS3231.c>    //  DS3231 RTC k�t�phanesi dahil ediyoruz.
RTC_Time *mytime;      // DS3231 K�t�phanesinin ba�lant�s�

//  __Lib_FAT32.h  k�t�phanesi dahil ediyoruz
#include "__Lib_FAT32.h"
__HANDLE fileHandle;     // FAT32 k�t�phanesinden fileHandle aktif ediyoruz

// Di�er ana de�i�kenler
short fat_err;
short i, p_second = 1;

// B1  butonun lcd ilerlemesi i�in yaz�lm�� kod blogu
char debounce()
{
  char m, count = 0;
  for(m = 0; m < 5; m++)
  {
    if ( !button1 )
      count++;
    delay_ms(10);
  }

  if(count > 2)  return 1;
  else           return 0;
}

//  Geciklemeli ba�lant� sa�l�yoruz
void wait()
{
  unsigned int TMR0_value = 0;
  TMR0L = TMR0H = 0;
  while( (TMR0_value < 62500L) && button1 && button2 )
    TMR0_value = (TMR0H << 8) | TMR0L;
}

char edit(char x_pos, char y_pos, char parameter)
{
  char buffer[3];
  while(debounce());  // B1 butonuna bas�ld���nda de�i�mesi i�in de�i�ken
  sprinti(buffer, "%02u", (int)parameter);

  while(1)
  {
    while( !button2 )
    {
      parameter++;
      if(i == 0 && parameter > 23)
        parameter = 0;
      if(i == 1 && parameter > 59)
        parameter = 0;
      if(i == 2 && parameter > 31)
        parameter = 1;
      if(i == 3 && parameter > 12)
        parameter = 1;
      if(i == 4 && parameter > 99)
        parameter = 0;

      sprinti(buffer, "%02u", (int)parameter);
      LCD_Out(y_pos, x_pos, buffer);
      delay_ms(200);
    }

    LCD_Out(y_pos, x_pos, "  ");
    wait();
    LCD_Out(y_pos, x_pos, buffer);
    wait();

    if(!button1) // Buton 1 bas�ld�g�nda d�ng�ye giriyor
    {
      i++;                // Di�er parametreye ge�iyor
      return parameter;   // Parametreye d�n�yor ve ��k�yor
    }
  }
}

void dow_print()
{
  switch(mytime->dow)
  {
    case SUNDAY   :  LCD_Out(2, 7, "PZR");  break;
    case MONDAY   :  LCD_Out(2, 7, "PZT");  break;
    case TUESDAY  :  LCD_Out(2, 7, "SAL");  break;
    case WEDNESDAY:  LCD_Out(2, 7, "CAR");  break;
    case THURSDAY :  LCD_Out(2, 7, "PER");  break;
    case FRIDAY   :  LCD_Out(2, 7, "CUM");  break;
    default       :  LCD_Out(2, 7, "CMR");
  }
}

void rtc_print()
{
  char buffer[17];
  // hafta ve g�n�n yazd�r�lmas�
  dow_print();
  // zaman�n yazd�r�lmas�
  sprinti(buffer, "%02u:%02u:%02u", (int)mytime->hours, (int)mytime->minutes, (int)mytime->seconds);
  LCD_Out(1, 11, buffer);
  // tarihin yazd�r�lmas�
  sprinti(buffer, "-%02u-%02u-20%02u", (int)mytime->day, (int)mytime->month, (int)mytime->year);
  LCD_Out(2, 10, buffer);
}

//////////////////////////////////////// DHT22 Fonksiyonu ////////////////////////////////////////

// send start signal to the sensor
void Start_Signal(void)
{
  DHT22_PIN     = 0;   // DHT22 pinine 0 volt g�nderilmesi
  DHT22_PIN_DIR = 0;   // ��k�s�n�n ayarlanmas�
  delay_ms(25);        // 25 mikrosaniye bekliyor
  DHT22_PIN     = 1;   // pine 5 volt g�nderilmesi
  delay_us(30);        // 30 mikrosaniye bekliyor
  DHT22_PIN_DIR = 1;
}

// Sens�rlerin Kontrol edilmesi
char Check_Response()
{
  TMR0L = TMR0H = 0;  // Reseet tu�una 0 volt g�nderilmesi

  // DHT22_PIN y�kselene kadar bekleyin (80�s d���k zaman yan�t�n�n kontrol�)
  while(!DHT22_PIN && TMR0L < 100) ;

  if(TMR0L >= 100)  // yan�t s�resi > 80�S ==> yan�t hatas� ise
    return 0;       // 0 d�nd�r (cihaz�n yan�t vermede bir sorunu var)

  TMR0L = TMR0H = 0;  // dinlenme Timer0 d���k ve y�ksek kay�tlar

  // DHT22_PIN d���k olana kadar bekleyin (80�s y�ksek zaman yan�t�n�n kontrol edilmesi
  while(DHT22_PIN && TMR0L < 100) ;

  if(TMR0L >= 100)  // yan�t s�resi > 80�S ==> yan�t hatas� ise
    return 0;       // 0 d�nd�r (cihaz�n yan�t vermede bir sorunu var)

  return 1;   // d�ng�ye tekrar sokuluyor 1 (yan�t tamam)
}

// Veri fonskiyonun okunmas�
void Read_Data(unsigned short* dht_data)
{
  *dht_data = 0;
  for(i = 0; i < 8; i++)
  {
    TMR0L = TMR0H = 0;

    while(!DHT22_PIN)
    {
      if(TMR0L > 80);
    }

    TMR0L = TMR0H = 0;

    while(DHT22_PIN)
    {
      if(TMR0L > 80)
        return;
    }
    if(TMR0L > 40)
    {
      *dht_data |= (1 << (7 - i));
    }
  }


  return;
}



// Ana fonskiyon
void main()
{
  OSCCON = 0x70;      // dahili osilat�r� 16MHz'e ayarl�yoruz
  ANSELB = 0;         // t�m PORTB pinlerini dijital olarak yap�land�r�yoruz
  ANSELC = 0;         // t�m PORTC pinlerini dijital olarak yap�land�r�yoruz
  ANSELD = 0;         // t�m PORTD pinlerini dijital olarak yap�land�r�yoruz
  // RB0 ve RB1 dahili pull-up'lar�n� etkinle�tirin
  RBPU_bit  = 0;      // RBPU bitini temizle (INTCON2.7)
  WPUB      = 0x03;   // WPUB kayd� = 0b00000011

  delay_ms(1000);           // 1 saniye bekliyoruz
  Lcd_Init();               // LCD mod�l�n� ba�latyoruz
  Lcd_Cmd(_LCD_CURSOR_OFF); // �mleci kapat�yoruz
  Lcd_Cmd(_LCD_CLEAR);      // LCD temizleme Komutu
  LCD_Out(1, 1, "ZAMAN:");
  LCD_Out(2, 1, "TARIH:");
  LCD_Out(3, 1, "SICAK:");
  LCD_Out(4, 1, "NEM:");

  // 100KHz saat frekans� ile I2C2 mod�l�n� ba�lat
  I2C2_Init(100000);
  // SPI1 mod�l�n� en d���k h�zda ba�lat
  SPI1_Init_Advanced(_SPI_MASTER_OSC_DIV64, _SPI_DATA_SAMPLE_MIDDLE, _SPI_CLK_IDLE_LOW, _SPI_LOW_2_HIGH);

  UART1_Init(9600);   // UART1 mod�l�n� 9600 baud'da ba�lat
  UART1_Write_Text("\r\n\nFat Kutuphanesi Baslatildi ... ");

  fat_err = FAT32_Init();
  if(fat_err != 0)
  {  // FAT32 kitapl��� ba�lat�l�rken bir sorun olu�tuysa
    UART1_Write_Text("Fat Kutuphanesi Baslatilamadi!");
  }

  else
  {  // FAT32 kitapl��� (& SD kart) ba�lat�ld� (ba�lat�ld�)
    // SPI1 mod�l�n� en y�ksek h�zda yeniden ba�lat
    SPI1_Init_Advanced(_SPI_MASTER_OSC_DIV4, _SPI_DATA_SAMPLE_MIDDLE, _SPI_CLK_IDLE_LOW, _SPI_LOW_2_HIGH);
    UART1_Write_Text("FAT Kutuphanesi Bulundu");
    if(FAT32_Exists("DHT22Log.txt") == 0)   // 'DHT22.Log.txt' adl� dosyan�n mevcut olup olmad���n� test edin
    {
      // 'DHT22Log.txt' adl� bir metin dosyas� olu�turun
      UART1_Write_Text("\r\n\r\nCreate 'DHT22Log.txt' file ... ");
      fileHandle = FAT32_Open("DHT22Log.txt", FILE_WRITE);
      if(fileHandle == 0){
        UART1_Write_Text("OK");
        // 'DHT22Log.txt' dosyas�na baz� metinler yaz�n
        FAT32_Write(fileHandle, "    TARIH    |    ZAMAN  | SICAKLIK | NEM\r\n", 49);
        FAT32_Write(fileHandle, "(dd-mm-yyyy)|(hh:mm:ss)|             |\r\n", 40);
        FAT32_Write(fileHandle, "            |          |             |\r\n", 40);
        // �imdi 'DHT22Log.txt' dosyas�n� kapat�n
        FAT32_Close(fileHandle);
      }
      else
        UART1_Write_Text("DOSYA HATASI.");
    }
  }

  while(1)
  {
    // RTC �ipinden ge�erli saati ve tarihi oku
    mytime = RTC_Get();
    // t�m RTC verilerini yazd�r
    rtc_print();

    if( !button1 )    // B1 d��mesine bas�l�rsa
    if( debounce() )  // geri d�nme i�levini �a��r (B1'e bas�ld���ndan emin olun) Geri d�nme i�levini �a��r (B1'e bas�ld���ndan emin olun)
    {
      i = 0;
      while( debounce() );  // geri d�nme i�levini �a��r�n (B1 d��mesinin serbest b�rak�lmas�n� bekleyin)
      T0CON  = 0x84;        // Timer0 mod�l�n� etkinle�tir (16-bit zamanlay�c� ve �n �l�ekleyici = 32)

      mytime->hours   = edit(11, 1, mytime->hours);    // Saati Ayarla
      mytime->minutes = edit(14, 1, mytime->minutes);  // Dakikay� Ayarla
      mytime->seconds = 0;                             // Saniyeyi resetle

      while(debounce());  // geri d�nme i�levini �a��r�n (B1'in serbest b�rak�lmas�n� bekleyin)
      while(1)
      {
        while( !button2 )  //  B2 d��mesine bas�l�rsa
        {
          mytime->dow++;
          if(mytime->dow > SATURDAY)
            mytime->dow = SUNDAY;
          dow_print();     // haftan�n g�n�n� yazd�r
          delay_ms(500);   // yar�m saniye bekle
        }
        LCD_Out(2, 7, "   ");
        wait();         //  wait()    fonsiyonun �a��r
        dow_print();    // haftan�n g�n�n� yazd�r
        wait();
        if( !button1 )
          break;
      }

      mytime->day     = edit(11, 2, mytime->day);      // g�n� d�zenle (ay�n g�n�)
      mytime->month   = edit(14, 2, mytime->month);    // ay� d�zenle
      mytime->year    = edit(19, 2, mytime->year);     // y�l� d�zenle

      TMR0ON_bit = 0;     // Timer0 mod�l�n� devre d��� b�rak
      while(debounce());  //geri d�nme i�levini �a��r�n (B1 d��mesinin serbest b�rak�lmas�n� bekleyin)
      // RTC �ipine veri yaz
      RTC_Set(mytime);
    }

    if( mytime->seconds != p_second )
    {   // her 1 saniyede bir sens�rden s�cakl�k de�erini oku ve yazd�r
      char T_Byte1, T_Byte2, RH_Byte1, RH_Byte2, CheckSum;
      unsigned int Temp, Humi;
      char temp_a[9], humi_a[8];
      bit read_ok;
      p_second = mytime->seconds;  // ge�erli saniyeyi kaydet
      T0CON  = 0x81;   // Timer0 mod�l�n� etkinle�tir (16-bit zamanlay�c� ve �n �l�ekleyici = 4)

      Start_Signal();  // sens�re bir ba�lang�� ??sinyali g�nder
      if(Check_Response())  // sens�rden yan�t gelip gelmedi�ini kontrol edin (Tamam ise nem ve s�cakl�k verilerini okumaya ba�lay�n)
      {
        Read_Data(&RH_Byte1);  //  humidity 1st byte okumas� ve kaydedilmesi RH_Byte1
        Read_Data(&RH_Byte2);  //  humidity 2nd byte okunmas� ve kaydedilmesi H_Byte2
        Read_Data(&T_Byte1);   //  s�cakl�k 1st byte okunmas� ve yaz�lmas�  T_Byte1
        Read_Data(&T_Byte2);   //  s�cakl�k  2nd byte okunmas�  ve  yaz�lmas� T_Byte2
        Read_Data(&CheckSum);  // sa�lama toplam�n� okuyun ve de�erini CheckSum'a kaydedin

        // t�m verilerin do�ru g�nderilip g�nderilmedi�ini test edin
        if(CheckSum == ((RH_Byte1 + RH_Byte2 + T_Byte1 + T_Byte2) & 0xFF))
        {
          read_ok = 1;
          Humi = (RH_Byte1 << 8) | RH_Byte2;
          Temp = (T_Byte1  << 8) |  T_Byte2;

          if(Temp & 0x8000) {      //
            Temp = Temp & 0x7FFF;
            sprinti(temp_a, "-%02u.%01u%cC ", (Temp/10)%100, Temp % 10, (int)223);
          }
          else
            sprinti(temp_a, " %02u.%01u%cC ", (Temp/10)%100, Temp % 10, (int)223);

          if(humi >= 1000)
            sprinti(humi_a, "1%02u.%01u %%", (humi/10)%100, humi % 10);
          else
            sprinti(humi_a, " %02u.%01u %%", (humi/10)%100, humi % 10);

        }
        else
        {
          read_ok = 0;
          sprinti(temp_a, "Checksum");
          sprinti(humi_a, " Error ");
        }
      }

      else
      {
        read_ok = 0;
        sprinti(temp_a, " Sensor ");
        sprinti(humi_a, " Error ");
      }

      TMR0ON_bit = 0;     // Timer0 mod�l�n� devre d��� b�rak
      // LCD'de sens�r verilerini yazd�r
      LCD_Out(3, 7, temp_a);
      LCD_Out(4, 7, humi_a);

      // sens�r verilerini SD karta kaydedin (DHT22Log.txt dosyas�)
      if(fat_err == 0)
      { // FAT32 kitapl��� ba�ar�yla ba�lat�ld�ysa
        char buffer[28];
        sprinti(buffer, " %02u-%02u-20%02u | %02u:%02u:%02u |   ", (int)mytime->day, (int)mytime->month, (int)mytime->year,
                       (int)mytime->hours, (int)mytime->minutes, (int)mytime->seconds);
        // 'DHT22Log.txt' dosyas�n� ekleme izniyle a��n
        fileHandle = FAT32_Open("DHT22Log.txt", FILE_APPEND);
        if(fileHandle == 0)
        {
        FAT32_Write(fileHandle, buffer, 27);
        if(read_ok == 1)      // sa�lama toplam� veya sens�r hatas� yoksa
          temp_a[5] = '�';    // put degree symbol
          FAT32_Write(fileHandle, temp_a, 8);
          FAT32_Write(fileHandle, "  | ", 5);
          FAT32_Write(fileHandle, humi_a, 7);
          FAT32_Write(fileHandle, "\r\n", 2);     // start a new line
          // �imdi dosyay� kapat�n (DHT22Log.txt)
          FAT32_Close(fileHandle);
        }
      }

    }

    delay_ms(100);

  }

}
#include "N64Controller.h"

N64Controller player1 (2);

void readRamFromMBC5(unsigned char bank = 0x00);

unsigned short n64_addr_encode(unsigned short addr){
  unsigned char table[11]={0x15, 0x1F, 0x0B, 0x16, 0x19, 0x07, 0x0E,
  0x1C, 0x0D, 0x1A, 0x01};
  unsigned char i=0;
  //Mask off the lower 5 bits. These should be zero.
  addr&=0xFFE0;
  for(i=0;i<11;i++){
    if (addr & (1<<(i+5))){
      addr ^= table[i];
    }
  }
  return addr; //Returns the original address with the crc in the lower 5 bits
}

void setup() {
   
    Serial.begin(9600);
    player1.begin(); // Initialisation
    
}

unsigned char* printHex() {
  unsigned char* data = new unsigned char[32];
  Serial.print("Hex: ");
  for (int byteIndex = 0; byteIndex < 32; byteIndex++) {
    unsigned char byte = 0;
    for (int bit = 0; bit < 8; bit++) {
      if (player1.interface->raw_dump[byteIndex * 8 + bit]) {
        byte |= 1 << (7 - bit);
      }
    }
    // If value is less than 16, add a 0 to keep the format XX on Hex prints
    if (byte < 16) {
      Serial.print("0");
    }
    Serial.print(byte, HEX);
    Serial.print(" "); // Space between bytes
    data[byteIndex] = byte;
  }
  Serial.println();
  return data;
}

void sendCommands ( unsigned char command[], int len) {
    noInterrupts();
    player1.interface->send(command, len);
    player1.interface->get();
    interrupts();
}

void end() {
  while(1) {

  }
}

void loop() {
    delay(30);

    // Enable power
    unsigned char command[] = {0x03, 0x80, n64_addr_encode(0x8000), 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, // 8bytes
    };
    sendCommands( command , 35);
    delay(40);

    // Enable cartridge access mode
    unsigned char command2[] = {0x03, 0xB0, n64_addr_encode(0xB000), 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // 8bytes
    };
    sendCommands( command2 , 35);
    delay(40);

    // Switch to bank 0
    unsigned char command3[] = {0x03, 0xA0, n64_addr_encode(0xA000), 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    };
    sendCommands( command3 , 35);
    delay(40);
    // Read 
    
    Serial.println("Step 1 Detect cartridge type on byte 0148");
    unsigned char command4[] = {0x02, 0xC1, n64_addr_encode(0xC140)};
    sendCommands( command4 , 3);
    delay(40);
    printHex();


    unsigned char* data = printHex();
    Serial.println(*data, HEX);
    unsigned char cartType = data[8];
    int cartSize = 0;
    unsigned char MBC = 5; // Manually set MBC type for debugging purposes
                           // This should be deleted on the future to use cartTypes instead
    switch(cartType) {
      case 0x05: // This is currently the only mode implemented to dump Pokemon Yellow & other MBC similar games
        cartSize = 1000; 
        Serial.println("Detected 1MB");
        if(MBC == 5) {
          readRamFromMBC5(0x00);
          readRamFromMBC5(0x01);
          readRamFromMBC5(0x02);
          readRamFromMBC5(0x03);
        }
        end();
        break;
      case 0x06: // TODO other cases should be implemented
        cartSize = 2000; 
        Serial.println("Detected 2MB");
        
        break;
      default:
        Serial.print("Not configured for cart type ");
        Serial.print(cartType, HEX);
        end();
    }
    int iterations = cartSize /16;
    Serial.println("Iterations ");
    Serial.print(iterations);
    Serial.println();
    delay(1000);
    
    // Default behaviour 
    if(MBC == 5) {
      Serial.println("Default mode MBC5");
      readRamFromMBC5(0x00);
      readRamFromMBC5(0x01);
      readRamFromMBC5(0x02);
      readRamFromMBC5(0x03);
    }
    

    end();

    // This code is never executed, is here as an example to debug cart data reqading in raw mode
    Serial.println("Read first 16kb [ 32 bytes x 512 Iterations]");
    String myString = "";
    for(int i = 0; i < 512; i++) {
      int pos = 0xC000 + i*32;
      unsigned char highByte = (pos >> 8) & 0xFF; // Move 8 bits and get the bigger one
      unsigned char command4[] = {0x02, highByte, n64_addr_encode(pos)};
      sendCommands( command4 , 3);
      //delay(1);
      if(i%10==0) {
        Serial.println(i);     
      }
      unsigned char* myChars = printHex();
      delete(myChars); // Important to avoid memory leaks (2kb of ram ONLY on arduino)

    }
    Serial.println("Done");

    end();
}

void readRamFromMBC5(unsigned char bank = 0x00) {

    // Enable ram wirtting
    // Switch to bank 1 to map address 4000 on 0xC000 | 0xA000 -> 1
    // Set 0xC000 to ram bank number 0 | 0xC000 -> 0

   //Enable ram by writing to 0x0000
    unsigned char enableRamCommand[] = {0x03, 0xC0, n64_addr_encode(0xC000), 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, // 8bytes
    };
    sendCommands( enableRamCommand , 35);
    delay(40);

    // Switch to position 0x4000 to 0xC000
    unsigned char switchPositionCommand[] = {0x03, 0xA0, n64_addr_encode(0xA000), 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // 8bytes
    };
    sendCommands( switchPositionCommand , 35);
    delay(40);

  // Set ram bank 0-3
  unsigned char ramBankCommand[] = {0x03, 0xC0, n64_addr_encode(0xC000), 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, bank, // 8bytes
    };
     sendCommands( ramBankCommand , 35);
    delay(40);
    // switchPositionCommand to 0x8000 - 0xBFFF -> -xA000 = 0x02
    unsigned char switchPositionCommand2[] = {0x03, 0xA0, n64_addr_encode(0xA000), 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 8bytes
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, // 8bytes
    };
     sendCommands( switchPositionCommand2 , 35);
    delay(40);
    // Now if we read 0xE000 we read RAM
    // Read first 8 KB
    for(int i = 0; i < 256; i++) {
      int pos = 0xE000 + i*32;
      unsigned char highByte = (pos >> 8) & 0xFF; //Move 8 bits and get the higher one
      unsigned char command4[] = {0x02, highByte, n64_addr_encode(pos)};
      sendCommands( command4 , 3);
      //delay(1);
      if(i%10==0) {
        Serial.println(i);     
      }
      unsigned char* myChars = printHex();
      delete(myChars); // Important to avoid memory leaks (2kb of ram ONLY on arduino)
      

    }
    Serial.println("Done");

   
    
    delay(1);
}


/* Code used for initial signal testing [Incorrect values returned]
const int pin = 2;
bool data[32];
void one() {
  digitalWrite(pin, LOW);
  delayMicroseconds(1);
  digitalWrite(pin, HIGH);
  delayMicroseconds(3);
}
void zero() {
  digitalWrite(pin, LOW);
  delayMicroseconds(3);
  digitalWrite(pin, HIGH);
  delayMicroseconds(1);
}

void sendReadButtonRequest() {//0x01
  pinMode(pin, OUTPUT);
  zero(); zero(); zero(); zero();
  zero(); zero(); zero(); one();
  // Stop bit
  one();
}

bool readKey() {
  int response = 0;
  for(int i =0; i < 4; i ++){
    response += digitalRead(pin);
    delayMicroseconds(1);
  }
  if( response >= 2 ) return true;
  return false;
}



void readResponse() {
  pinMode(pin, INPUT);
  delayMicroseconds(2); //Wait 2us after falling edge
  
  for(int i = 0; i <32; i++){
    
    data[i] = digitalRead(pin);
    delayMicroseconds(1);
  }
  for(int i = 0; i < 32; i++) {
    Serial.print(data[i]);
  }
}

void setup(){
  Serial.begin(9600);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH); // We start in high mode
  Serial.println("Starting");
  sendReadButtonRequest();
  readResponse();

}

void loop() {

  sendReadButtonRequest();
  readResponse();
  Serial.println();
  delay(100);
}

*/

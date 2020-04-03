#define PDURESET      A0 //output PC0 
#define RX_STS        A1 //GPIO IO3 PC1 
#define COMNM_VREF    A2 //COMM_voltage status input PC2
#define BDtemp        A3 //LM61 Analog temperature sensor(Option) PC3
#define RX_EN         2 //STRX_data_input flag  PD2
#define RXCLKO        3 //STRX_data clock input  PD3
#define RX_NRZL       4 //STRX_data bitstream    PD4
#define CMD_DATA_MAX 128 + 2

#define START_SEQ_1 0xEB
#define START_SEQ_2 0x90
#define END_SEQ_1 0xC5
#define END_SEQ_2 0x79

#define REC_STATUS_0 0x00
#define REC_STATUS_1 0x01
#define REC_STATUS_2 0x02
#define REC_STATUS_3 0x03
#define REC_STATUS_4 0x04


void setup() {
  Serial.begin(57600);
  Serial.print("CMD_RCV Clock: ");
  Serial.print(F_CPU);
  Serial.println("Hz");
  
  pinMode(COMNM_VREF ,INPUT);
  pinMode(RX_EN ,INPUT);
  pinMode(RXCLKO ,INPUT);
  pinMode(RX_NRZL ,INPUT);
  pinMode(PDURESET  ,OUTPUT);
  pinMode(RX_STS  ,OUTPUT);

  digitalWrite(PDURESET,LOW);
  digitalWrite(RX_STS,LOW);
  
}
void loop() {
  RxDataRead();
}

void RxDataRead(void)
{
  int i;
  int level;
  int dn_flg = 0;
  int sync_sts = 0;
  int data_pos = 0;
  unsigned char tmp_bit=0;
  int bit_pos = 0;
  unsigned char rec_data[CMD_DATA_MAX];
  unsigned char pn_code[28] =
  {0xFF,0x48,0x0E,0xC0,0x9A,0x0D,0x70,0xBC,0x8E,0x2C,0x93,0xAD,0xA7,0xB7,  //1
   0x46,0xCE,0x5A,0x97,0x7D,0xCC,0x32,0xA2,0xBF,0x3E,0x0A,0x10,0xF1,0x88,  //2
  };
  int pn_count = 0;
  int rx_en;
  for(i = 0; i < CMD_DATA_MAX; i++)
  {
      rec_data[i] = 0;
  }

  unsigned int sync_marker = 0;
  unsigned char bit_inverse = 0;
  #define START_SEQ 0xEB90
  
  while(1)
  {
    if(bit_is_set(PIND,PD3) == 0)
        level = LOW;
    else
        level = HIGH;
    
    if(level == HIGH && dn_flg == 0)
    { 
      delayMicroseconds(10);
      if(bit_is_set(PIND,PD4))
        tmp_bit = 1;
      else
        tmp_bit = 0;
      
      if(sync_sts == REC_STATUS_0)
      {
        sync_marker = (sync_marker << 1)&0xFFFFFFFE;
        sync_marker += (tmp_bit&0x01);
        if(sync_marker == START_SEQ || sync_marker == ~START_SEQ)
        {
          sync_sts = REC_STATUS_1;
          rec_data[data_pos] = START_SEQ_1;data_pos++;
          rec_data[data_pos] = START_SEQ_2;data_pos++;
          if(sync_marker == ~START_SEQ)
          {
            bit_inverse = 1;
          }
        }
      }
      else if(sync_sts == REC_STATUS_1 || sync_sts == REC_STATUS_2)
      {
        rec_data[data_pos] = (rec_data[data_pos] << 1)&0xFE;
        rec_data[data_pos] += (tmp_bit&0x01);
        bit_pos++;
        if(bit_pos == 8)
        {
          if(bit_inverse)
          {
            rec_data[data_pos] = ~rec_data[data_pos];
          }
          
          if(rec_data[data_pos] == END_SEQ_1)
          {
            sync_sts = REC_STATUS_2;
          }
          else if(rec_data[data_pos] == END_SEQ_2 && sync_sts == REC_STATUS_2)
          {
            sync_sts = REC_STATUS_3;
            data_pos++;
            break;
          }
          data_pos++;
          bit_pos = 0;
        }
      }
      dn_flg = 1;
    }
    else if(level == LOW && dn_flg == 1)
    {
      dn_flg = 0;
    }

    if(data_pos == CMD_DATA_MAX)
    {
      break;
    }
  }
  
  if(data_pos > 0 && rec_data[0] == START_SEQ_1 && rec_data[1] == START_SEQ_2)
  {
    //derandomize
    for(i = 2; i < (data_pos - 8); i++)
    {
      if(pn_count > 27)
        pn_count = 0;
        
      rec_data[i] = rec_data[i]^pn_code[pn_count];
      pn_count++;
    }

    if(data_pos > 128)
      data_pos = 128;

    for(i = 2; i < (data_pos - 8); i++)
    {
      Serial.write(rec_data[i]);
    }
    Serial.write(END_SEQ_1);
    Serial.write(END_SEQ_2);
    
    data_pos = 0;
    pn_count = 0;
  }
}

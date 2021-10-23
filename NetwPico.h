#ifndef NETWPICO_H
#define NETWPICO_H

#define P2C_BUFFER_LENGTH   	20
#define P2C_RESET_ON_ERR_COUNT  2
#define P2C_NEXT_TX_DELAY_MS	2
#define P2C_RX_DELAY_MS			2

#define P2C_STATE_READING			1
#define P2C_STATE_SWITCH_2_WRITE	2
#define P2C_STATE_SEND				3
#define P2C_STATE_SWITCH_2_READ		4



#define TX_RETRY_COUNT			3


#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include <NetwBase.h>


// #define DEBUG

/*

2021-09-05 1.00.00 refactored form NetwTWI
         - default in slave mode
		 - before sending switch to master mode

	TOTRY:
	&	pull up
	&	re-add NW_TIMEOUT_RX
	-   400.000	 

	i2c	sda/scl
	port 0: gpio's 0/1, 4/5, 8/9, 12/13, 16/17, 20/21  
	port 1: gpio's 2/3, 6/7, 10/11, 14/.., 18/19, 22/.., 26/27

*/


class NetwPico: public NetwBase
{

public:

	NetwPico(uint sda){
		if( sda==0 || sda==4 || sda==8 || sda==12 || sda==16 || sda==20 ){
			i2cPort = i2c0;

		} else if( sda==2 || sda==6 || sda==10 || sda==18 || sda==26 ){
			i2cPort = i2c1;

		} else {
			Serial.println("Error i2c SDA pin!!");
		}

		// masterSlave = false;
		sda_pin = sda;
		scl_pin = sda+1;
		txRetryCount = TX_RETRY_COUNT; 
	}
	NetwPico(int port, uint sda, uint scl){
		if(port==0) i2cPort = i2c0;
		if(port==1) i2cPort = i2c1; 
		// masterSlave = false;
		sda_pin = sda;
		scl_pin = scl;
		txRetryCount = TX_RETRY_COUNT; 
	}

	NetwPico(uint sda, uint scl, uint sda2, uint scl2){
		masterSlave = true;
		// if(port==0){
			i2cPort = i2c0;	
			i2cPort2 = i2c1;
		// } 
		// if(port==1){

		// 	i2cPort = i2c1;
		// 	i2cPort2 = i2c0;
		// }  
		sda_pin = sda;
		scl_pin = scl;		
		sda2_pin = sda2;
		scl2_pin = scl2;
		txRetryCount = TX_RETRY_COUNT; 
	}






	virtual ~NetwPico(){}  // suppress warning

	i2c_inst_t *i2cPort;
	i2c_inst_t *i2cPort2;

	uint	sda_pin;
	uint	scl_pin;
	uint	sda2_pin;
	uint	scl2_pin;
	uint	slave_address=9;    // default to 9 for parentNode
	volatile uint	readAvailable=0;
	int 	health=P2C_RESET_ON_ERR_COUNT;
	bool	pullUp=true;
	bool  	masterSlave=false;

	volatile uint 	available;
	volatile uint 	state = 0;
	
	volatile int   	restarts=0;

    byte twUploadNode = 0x09;  // default for upload


	uint8_t tw_rxBuffer[P2C_BUFFER_LENGTH];
	volatile uint8_t tw_rxBufferIndex=0;
 

	volatile unsigned int err40Count=0;
	volatile unsigned int err41Count=0;
	volatile unsigned int err42Count=0;
	volatile unsigned int err43Count=0;
	volatile unsigned int err44Count=0;
	volatile unsigned int err45Count=0;
	volatile unsigned int err46Count=0;
	volatile unsigned int err47Count=0;
	volatile unsigned int err48Count=0;
	volatile unsigned int err49Count=0;
	volatile unsigned int err50Count=0;
	volatile unsigned int err51Count=0;
	volatile unsigned int err52Count=0;
	volatile unsigned int err53Count=0;
	volatile unsigned int err54Count=0;
	volatile unsigned int err55Count=0;

    // void onRequest( void (*)(void) );
	void begin(int slaveAddress);

    uint32_t status(){ 
		return i2cPort->hw->status;
	}

    int upload(int id);
	void localCmd(int cmd, long val);	
	void loop(void);

    void trace(char* id);

    bool isReady(void);
	bool isBusy(void);
	bool isIdle(void);
	int  getWriteAvailable(void);
	int  getReadAvailable(void);

	int write(RxData *rxData);

	void pullUpsOff()
	{
	//   digitalWrite(SDA, 0);
	//   digitalWrite(SCL, 0);
	}
	void pullUpsOn()
	{
	//   digitalWrite(SDA, 1);
	//   digitalWrite(SCL, 1);
	}

private:

	// #ifdef SUPPORT_ON_REQUEST
	// 	void (*user_onRequest)(void);
	// 	void onRequestService(void);
	// #endif


};

#endif


/* status values 
https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf

pagina 510   IC_STATUS

bit 0 i2c idle
bit 5 master idle
bit 6 slave idle

 6 			0110
 71		111 0001
 14     001 0100
 79     111 1001
 30     011 0000
 95    1001 0101

*/

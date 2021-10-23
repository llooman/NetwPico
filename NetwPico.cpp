#include "NetwPico.h"
// #include <compat/twi.h>

// #ifndef cbi
// #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
// #endif
// #ifndef sbi
// #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
// #endif


/* switch to state to prevent 1ms waiting in the write  loop*/

	// if( isReady(NW_TIMEOUT_RX)
	// ){
	// 	timerOff(NW_TIMEOUT_RX);

	// 	switch(state){

	// 	case 0:
	// 		available = i2c_get_read_available( i2cPort  );	
	// 		if(available >0){
	// 			state = P2C_STATE_READING;
	// 			nextTimerMillis(NW_TIMER_BUSY, P2C_RX_DELAY_MS);
	// 		} else {

	// 			NetwBase::loop();

	// 		}
	// 		break;


	// 	case P2C_STATE_READING:  // 1
	// 		available = i2c_get_read_available( i2cPort  );	
	// 		if(available >0){

	// 			i2c_read_raw_blocking (i2cPort, tw_rxBuffer, available);

	// 			if( isRxFull() )
	// 			{
	// 				// #ifdef DEBUG
	// 					{  Serial.println(F("rxBuf overflow!")); Serial.flush();}
	// 				// #endif
	// 				lastError =	ERR_RX_FULL;
	// 				readOverflowCount++;

	// 			} else {

	// 				for(int i=0; i<sizeof(RxMsg); i++)
	// 				{
	// 					rxFiFo[rxBufIn].data.raw[i] = i< available ? tw_rxBuffer[i] : 0x00;
	// 				}
	
	// 			}
	// 		}

	// 		NetwBase::loop();

	// 		state = 0;
	// 		nextTimerMillis(NW_TIMEOUT_TX, P2C_NEXT_TX_DELAY_MS );  			
	// 		nextTimerMillis(NW_TIMER_BUSY, P2C_NEXT_TX_DELAY_MS);
		
	// 		break;
	// 	case 2: 
		
	// 		nextTimerMillis(NW_TIMER_BUSY, 1);	
	// 		break;
	// 	case 3: 
		
		
	// 		break;
	// 	case 4: 
		
		
	// 		break;

	// 	}

	// 	return;
	// }	




void NetwPico::loop()  // TODO  parentFlag or client ????
{
	if(health<=0){

		Serial.print(F("@"));
		Serial.print(millis());
		Serial.print(F(" ")); 
		Serial.print(F("p2c restart "));
		Serial.println( nodeId);
		
		i2c_deinit(i2cPort);

		sleep_ms(250);

		begin(slave_address);

		health = P2C_RESET_ON_ERR_COUNT;

		nextTimerMillis(NW_TIMEOUT_TX, P2C_NEXT_TX_DELAY_MS ); //P2C_NEXT_TX_DELAY_MS

		restarts++;
		return;
	}

	available = i2c_get_read_available( i2cPort  );
 
	if( readAvailable < 1 
	 && available > 0)
	{
		readAvailable = available;
		nextTimerMillis(NW_TIMEOUT_RX, P2C_RX_DELAY_MS);

		state = P2C_STATE_READING;
		nextTimerMillis(NW_TIMEOUT_RX, P2C_RX_DELAY_MS);
	}
		
	if( isTime(NW_TIMEOUT_RX)){

		if(available >0){

			i2c_read_raw_blocking (i2cPort, tw_rxBuffer, available);
 
			if( isRxFull() )
			{
				#ifdef DEBUG
					{  Serial.println(F("rxBuf overflow!")); Serial.flush();}
				#endif
				lastError =	ERR_RX_FULL;
				readOverflowCount++;

			} else {

				for(int i=0; i<sizeof(RxMsg); i++)
				{
					rxFiFo[rxBufIn].data.raw[i] = i< available ? tw_rxBuffer[i] : 0x00;
				}

				// #ifdef DEBUG
					// {  Serial.print(F("rx["));Serial.print(rxBufIn);Serial.println(F("<")); Serial.flush();}
				// #endif

				rxAddCommit( ); // point to the next
				
			}
		}

		readAvailable = 0;
		timerOff(NW_TIMEOUT_RX);
		nextTimerMillis(NW_TIMEOUT_TX, 3 ); // P2C_NEXT_TX_DELAY_MS
		// NetwBase::loop();
		loopRX(); 
		return;
	}
 
	loopTxReset();
  
	loopTX();  // send when idle
 
	loopRX();  // process received data

	uploadNewErrors();

	if(autoPing > 0) loopPing();

	// NetwBase::loop(); // 2018

	if( isTxEmpty()
	 && isTime(NW_TIMER2_UPLOAD_DEBUG)  
	){
		nextTimerMillis(NW_TIMER2_UPLOAD_DEBUG, 25);

		switch(nextDebugId2ForUpload){
			case 0:upload(140); break;
			case 1:upload(142); break;
			case 2:upload(144); break;
			case 3:upload(148); break;
			case 4:upload(149); break;
			case 5:upload(150); break;
			case 6:upload(151); break;
			case 7:upload(152); break;
			case 8:upload(153); break;
		}

		nextDebugId2ForUpload++;

		if(nextDebugId2ForUpload>8)
		{
			timerOff(NW_TIMER2_UPLOAD_DEBUG);
			nextDebugId2ForUpload = 0; 
		}
	}
}

bool  NetwPico::isBusy(void){ return (  NetwBase::isBusy(NW_TIMEOUT_RX)  || ! i2cPort->hw->enable    ); }  

bool  NetwPico::isReady(void){ return ( NetwBase::isTimerInactive(NW_TIMEOUT_RX)
									 && (i2cPort->hw->status & 0x01)  == 0 
                                     && i2cPort->hw->enable 
									 && NetwBase::isReady(NW_TIMEOUT_TX)

						     ); }

bool  NetwPico::isIdle(void){ return ( (i2cPort->hw->status & 0x01) == 0  ); }
							 

int NetwPico::write(RxData *rxData)  // TODO
{
	if( false ) //! my_i2c.enable
	{
		#ifdef DEBUG
			Serial.println("write: !enable");
		#endif
 
		err48Count++;
		return ERR_TWI_NOT_READY;
	}

	 

	if(i2c_get_read_available( i2cPort  )>0){
		Serial.println("write: i2c_get_read_available>0 !!! ");
		return -60;
	}

	int ret =0;
	uint8_t toAddress = rxData->msg.node;

	if( isParent 
	 || (char)rxData->msg.cmd == 'U'
	 || (char)rxData->msg.cmd == 'u'
	 || (char)rxData->msg.cmd == 'E'
	 || (char)rxData->msg.cmd == 'e'
	   
	){
		toAddress = twUploadNode; // override address where the nodeId is from instead of to
	}
	else if (isMeshEnabled)
	{
		int connId = getMeshConn(toAddress);
		if(connId>0)
		{
			toAddress = connId;
		}
	}

	// handle Little Endian
	rxData->raw[4]=lowByte(nodeId);
	rxData->raw[5]=highByte(nodeId);

	int twSendSize=6;
	if( rxData->msg.val!=0) twSendSize=10;
	if( rxData->msg.deltaMillis >0)twSendSize=12;

	if(nodeId == rxData->msg.node ) nextTimer(NW_TIMER_PING, autoPing);	

	int tries = txFiFo[txBufOut].tries;

	if(masterSlave){
		ret = i2c_write_timeout_us (i2cPort2, toAddress, rxData->raw, twSendSize,  false, 21000 ); 
	} else {

		i2c_deinit(i2cPort);
		i2c_init( i2cPort, 100 * 1000);
		sleep_ms(1);
	
		ret = i2c_write_timeout_us (i2cPort, toAddress, rxData->raw, twSendSize,  false, 21000 ); 

		sleep_ms(1);

		i2c_deinit(i2cPort);
		i2c_init( i2cPort, 100 * 1000);
		i2c_set_slave_mode(i2cPort, true, slave_address);

	}

 	nextTimerMillis(NW_TIMEOUT_TX, P2C_NEXT_TX_DELAY_MS);

	#ifdef DEBUG
		Serial.printf("@%d(hw%d>i2c%d): %c.%d.%d.%d (try%d)=%d\n",millis(),i2c_hw_index(i2cPort),toAddress ,(char)rxData->msg.cmd, rxData->msg.node, rxData->msg.id, rxData->msg.val, tries,  ret);
	#endif

	if(ret<0){
		health--;
		// nextTimerMillis(NW_TIMEOUT_TX, 7);
	} else {
		health++;
		if(health>P2C_RESET_ON_ERR_COUNT) health = P2C_RESET_ON_ERR_COUNT;
 
	}

	if(ret == PICO_ERROR_GENERIC) return ERR_TWI_NOT_READY ; 	// -48
	if(ret == PICO_ERROR_TIMEOUT) return ERR_TX_TIMEOUT; 		// -41
	return ret;
}


int NetwPico::getWriteAvailable(void){

	return i2c_get_write_available( i2cPort  );
}
int NetwPico::getReadAvailable(void){

	return i2c_get_read_available( i2cPort  );
}

 

void NetwPico::begin( int slaveAddress)
{

	if(slaveAddress>0){

		slave_address = slaveAddress;
	}

    i2c_init( i2cPort, 100 * 1000);

	gpio_set_function(sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(scl_pin, GPIO_FUNC_I2C);

	i2c_set_slave_mode(i2cPort, true, slave_address);

	if(masterSlave){
    	i2c_init( i2cPort2, 100 * 1000);

		gpio_set_function(sda2_pin, GPIO_FUNC_I2C);
		gpio_set_function(scl2_pin, GPIO_FUNC_I2C);
	} 

	sleep_ms(1);

	// if(pullUp){
	// 	gpio_pull_up(sda_pin);
	// 	gpio_pull_up(scl_pin);
	// }

	// id=isParent?81:82;
	txAutoCommit = false;
	isMeshEnabled = !isParent;

	/* twi bit rate formula from atmega128 manual pg 204
	SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
	note: TWBR should be 10 or higher for master mode
	It is 72 for a 16mhz Wiring board with 100kHz TWI */

 
	// Serial.printf("i2c.begin hwIdx:%d nodeId:%d\n",i2c_hw_index(i2cPort) , nodeId  );

}
 
int NetwPico::upload(int id)
{
	if(uploadFunc==0 ) return 0;

	int ret=0;
	// Serial.print("NetwBase::upload "); 	Serial.println(id ); 
	switch( id )
	{
 
	case 140: ret=uploadFunc(id, err40Count, millis() ); break;
	case 141: ret=uploadFunc(id, err41Count, millis() ); break;
	case 142: ret=uploadFunc(id, err42Count, millis() ); break;
	// case 143: ret=uploadFunc(id, err43Count, millis() ); break;
	case 144: ret=uploadFunc(id, err44Count, millis() ); break;
	case 148: ret=uploadFunc(id, err48Count, millis() ); break;
	case 149: ret=uploadFunc(id, err49Count, millis() ); break;
	case 150: ret=uploadFunc(id, err50Count, millis() ); break;
	case 151: ret=uploadFunc(id, err51Count, millis() ); break;
	case 152: ret=uploadFunc(id, err52Count, millis() ); break;
	case 153: ret=uploadFunc(id, err53Count, millis() ); break;

	default: ret= NetwBase::upload(id);break;
	}
	return ret;
}

void NetwPico::localCmd(int cmd, long val)
{
	uint8_t* BSSID;

	switch ( cmd)
	{
	case 1:
		nextDebugId2ForUpload = 0;
		nextTimer(NW_TIMER2_UPLOAD_DEBUG, 0);
		nextDebugIdForUpload = 0;
		nextTimer(NW_TIMER_UPLOAD_DEBUG, 0);
        break;
	default:
		NetwBase::localCmd(cmd-100, val);
		break;
	}
}

void NetwPico::trace(char* id)
{
	Serial.print(F("@"));
	Serial.print(millis()/1000);
	Serial.print(F(" "));Serial.print(id);
	Serial.print(F(": pi="));	 Serial.print(nodeId);
	Serial.print(F(", addr:"));	 Serial.print(slave_address);
	Serial.print(F(", mesh="));	 Serial.print(isMeshEnabled);

	// Serial.print(F(", busy=")); Serial.print( isBusy()  );
	Serial.print(F(", rdy=")); Serial.print( isReady()   );
	Serial.print(F(", stat=")); Serial.print( i2cPort->hw->status  );
	Serial.print(F(", hw=")); Serial.print( i2cPort->hw->enable ); 
	Serial.print(F(", nwtmRx=")); Serial.print( timers3[NW_TIMEOUT_RX]/1000 ); 
	Serial.print(F(", nwtmTx=")); Serial.print( timers3[NW_TIMEOUT_TX]/1000 ); 
	Serial.print(F(", rdAvl=")); Serial.print( getReadAvailable()    ); 
	// Serial.print(F(", png@")); Serial.print(  timers[NW_TIMER_PING]/1000 );	

	Serial.print(F(", rx[")); Serial.print( rxBufIn );
	Serial.print(F("-")); Serial.print( rxBufOut );
	Serial.print(F("-")); Serial.print( rxFiFo[rxBufOut].timestamp>0 );
	Serial.print(F("]#")); Serial.print( receiveCount );
	// Serial.print(F(" av")); Serial.print( readAvailable );
	// Serial.print(F(", ping="));Serial.print( autoPing );

	Serial.print(F(", tx[")); Serial.print( txBufIn );
	Serial.print(F("-")); Serial.print( txBufOut );
	Serial.print(F("-")); Serial.print( txFiFo[txBufOut].timestamp>0 );
	Serial.print(F("]#")); Serial.print( sendCount );
	Serial.print(F("r")); Serial.print( sendRetryCount );
	Serial.print(F("e")); Serial.print( sendErrorCount );
 
	Serial.print(F(", hlth="));Serial.print( health );
	Serial.print(F(", lErr="));Serial.print( lastError );
	Serial.print(F(", #rst="));Serial.print( restarts );
	Serial.print(F(", ms="));Serial.print( masterSlave );


	Serial.println();
}


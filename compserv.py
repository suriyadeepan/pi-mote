#!/usr/bin/env python3

from i2clibraries import i2c_hmc5883l
import time
import socket

SLEEP_TIME = 0.2

#create an INET, STREAMing socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#socket.AF_INET, socket.SOCK_STREAM)
s.connect(("localhost", 5004))

i=0
val_av=0
	
while 1:

	hmc5883l = i2c_hmc5883l.i2c_hmc5883l(1)

	hmc5883l.setContinuousMode()
	hmc5883l.setDeclination(-1,11)

	time.sleep(SLEEP_TIME)

	val = int( str(hmc5883l) )

	if i>2:
		s.send( bytes( str(val_av/3) ,'UTF-8') )
		#print( int(val_av/3) )
		val_av=0
		i=0
	else:
		i+=1

	val_av +=val
		


# -*- coding: utf-8 -*-
import telnetlib, sys, select
from django.http import HttpResponse

from .auth import login_check

def _pre_process_cmd(cmd):
	if cmd.endswith( b"\r\n" ):
		return cmd
	elif cmd[-1] == b"\r":
		cmd += b"\n"
	elif cmd[-1] == b"\n":
		cmd = cmd[:-1] + b"\r\n"
	else:
		cmd += b"\r\n"
	return cmd

class TelnetConsole(object):
	def __init__( self, wsInst, host, port ):
		"""
		"""
		self.wsInst = wsInst
		self.host = host
		self.port = port
		self.consoleInst = None

	def close( self ):
		"""
		"""
		if self.consoleInst:
			self.consoleInst.close()
		self.consoleInst = None

		if self.wsInst:
			self.wsInst.close()
		self.wsInst = None
		
		self.host = ""
		self.port = 0

	def run( self ):
		"""
		"""
		try:
			self.consoleInst = telnetlib.Telnet( self.host, self.port )
		except Exception:
			self.wsInst.send("Server connection failed!\n")
			self.close()
			return
		
		self.onConnectedToConsole()

		try:
			tlfd = self.consoleInst.fileno()
			wsfd = self.wsInst.protocol.sock.fileno()
			rlist = [ tlfd, wsfd ]
			
			while True:
				rl, wl, xl = select.select(rlist, [], [], 0.1)
				if tlfd in rl:
					data = self.consoleInst.read_very_eager()
					if not data:
						break # socket closed
					if not self.onReceivedConsoleData( data ):
						break

				if wsfd in rl:
					data = self.wsInst.read()
					if data is None:
						break # socket closed
					if len(data) == 0:
						continue
					if not self.onReceivedClientData( data ):
						break
		except:
			sys.excepthook( *sys.exc_info() )

		self.close()
		#return HttpResponse("")
		return

		# test code
		"""
		try:
			for message in self.wsInst:
				self.wsInst.send(message)
		except:
			sys.excepthook( *sys.exc_info() )
		return
		"""

	def onConnectedToConsole( self ):
		"""
		template method.
		Callback when successfully connected to the telnet console
		"""
		pass

	def onReceivedConsoleData( self, data ):
		"""
		template method.
		Callback after receiving new data from the telenet console
		"""
		self.wsInst.send( data )
		return True
		
	def onReceivedClientData( self, data ):
		"""
		template method.
		Callback after receiving new data from the client
		"""
		if data == ":quit":
			self.wsInst.close()
			return False
		self.consoleInst.write( _pre_process_cmd( data ) )
		return True
		



class ProfileConsole(TelnetConsole):
	"""
	Console class for performance analysis
	"""
	def __init__( self, wsInst, host, port, command, sec, password):
		"""
		"""
		self.wsInst = wsInst
		self.host = host
		self.port = port
		self.consoleInst = None
		self.cmd = command.encode('utf-8')
		self.sec = sec.encode('utf-8')
		self.password = password.encode('utf-8')

	def onConnectedToConsole( self ):
		"""
		template method.
		Callback pytickprofile when successfully connected to the telnet console
		"""
		self.consoleInst.write( b"" + self.password + b"\r\n")
		self.consoleInst.write( b":"+self.cmd+b" "+self.sec+b"\r\n")

	def onReceivedConsoleData( self, data ):
		"""
		template method.
		Callback after receiving new data from the telenet console
		"""
		self.wsInst.send( data )
		return True
		
	def onReceivedClientData( self, data ):
		"""
		template method.
		Callback after receiving new data from the client
		"""
		if data == ":":
			self.wsInst.close()
			return False
		self.consoleInst.write( _pre_process_cmd( data ) )
		return True
		

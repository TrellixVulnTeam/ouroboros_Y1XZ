﻿namespace Ouroboros
{
	using UnityEngine;
	using System;
	using System.Net.Sockets;
	using System.Net;
	using System.Collections;
	using System.Collections.Generic;
	using System.Text;
	using System.Text.RegularExpressions;
	using System.Threading;

	using MessageID = System.UInt16;
	using MessageLength = System.UInt16;

	/// <summary>
	/// Network module
	/// handle connection, send and receive data
	/// </summary>
	public class NetworkInterfaceTCP : NetworkInterfaceBase
	{
		public override bool valid()
		{
			return ((_socket != null) && (_socket.Connected == true));
		}

		protected override Socket createSocket()
		{
			// Security.PrefetchSocketPolicy(ip, 843);
			Socket pSocket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			pSocket.SetSocketOption(System.Net.Sockets.SocketOptionLevel.Socket, SocketOptionName.ReceiveBuffer, OuroborosApp.app.getInitArgs().getTCPRecvBufferSize() * 2);
			pSocket.SetSocketOption(System.Net.Sockets.SocketOptionLevel.Socket, SocketOptionName.SendBuffer, OuroborosApp.app.getInitArgs().getTCPSendBufferSize() * 2);
			pSocket.NoDelay = true;
			//pSocket.Blocking = false;
			return pSocket;
		}

		protected override PacketReceiverBase createPacketReceiver()
		{
			return new PacketReceiverTCP(this);
		}

		protected override PacketSenderBase createPacketSender()
		{
			return new PacketSenderTCP(this);
		}

		protected override void onAsyncConnect(ConnectState state)
		{
			try
			{
				state.socket.Connect(state.connectIP, state.connectPort);
			}
			catch (Exception e)
			{
				Dbg.ERROR_MSG(string.Format("NetworkInterfaceTCP::_asyncConnect(), connect to '{0}:{1}' fault! error = '{2}'", state.connectIP, state.connectPort, e));
				state.error = e.ToString();
			}
		}
	}
}

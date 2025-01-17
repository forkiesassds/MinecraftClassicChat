#pragma once

#include "MainFormBase.h"
#define byte unsigned char

namespace MinecraftClassicChat 
{
	using namespace System;
	using namespace System::Windows::Forms;
	using namespace System::Net::Sockets;
	using namespace System::Threading;
	using namespace System::Text;
	using namespace System::Collections::Generic;
	using namespace System::IO;

	public ref class MainForm : public MainFormBase
	{
	private:
		Dictionary<int, int>^ packetSizeMap;
		Dictionary<char, int>^ textColors;
		bool connected;
		bool supportsLongerMessages;
		String^ clientName = "Classic Chat Client";
		String^ serverIP;
		array<String^>^ messagesToSend;
		int serverPort;
		short^ extCount;
		short recievedExts;
		TcpClient^ tcpClient;
		NetworkStream^ tcpStream;
		Thread^ tcpThread;

	public:
		MainForm() : MainFormBase()
		{
			packetSizeMap = gcnew Dictionary<int, int>();
			packetSizeMap->Add(0x00, 130);
			packetSizeMap->Add(0x01, 0);
			packetSizeMap->Add(0x02, 0);
			packetSizeMap->Add(0x03, 1027);
			packetSizeMap->Add(0x04, 6);
			packetSizeMap->Add(0x06, 7);
			packetSizeMap->Add(0x07, 73);
			packetSizeMap->Add(0x08, 9);
			packetSizeMap->Add(0x09, 6);
			packetSizeMap->Add(0x0a, 4);
			packetSizeMap->Add(0x0b, 3);
			packetSizeMap->Add(0x0c, 1);
			packetSizeMap->Add(0x0d, 65);
			packetSizeMap->Add(0x0e, 64);
			packetSizeMap->Add(0x0f, 1);
			packetSizeMap->Add(0x10, 66);
			packetSizeMap->Add(0x11, 68);
			packetSizeMap->Add(0x27, 5);

			textColors = gcnew Dictionary<char, int>();
			textColors->Add('0', 0);
			textColors->Add('1', 191);
			textColors->Add('2', 48896);
			textColors->Add('3', 49087);
			textColors->Add('4', 12517376);
			textColors->Add('5', 12517567);
			textColors->Add('6', 12566272);
			textColors->Add('7', 12566463);
			textColors->Add('8', 4210752);
			textColors->Add('9', 4210943);
			textColors->Add('a', 4259648);
			textColors->Add('b', 4259839);
			textColors->Add('c', 16728128);
			textColors->Add('d', 16728319);
			textColors->Add('e', 16777024);
			textColors->Add('f', 16777215);


			btnSend->Click += gcnew EventHandler(this, &MainForm::btnSend_Click);
			btnConnect->Click += gcnew EventHandler(this, &MainForm::btnConnect_Click);
			btnDisconnect->Click += gcnew EventHandler(this, &MainForm::btnDisconnect_Click);
			btnClearChat->Click += gcnew EventHandler(this, &MainForm::btnClearChat_Click);
			FormClosing += gcnew FormClosingEventHandler(this, &MainForm::MainForm_FormClosing);
		}

	private:
		List<byte>^ convertByteArray(array<byte>^ arr)
		{
			List<byte>^ lst = gcnew List<byte>();
			
			for (int entryIndex = 0; entryIndex < arr->Length; entryIndex++)
			{
				lst->Add(arr[entryIndex]);
			}

			return lst;
		}

		delegate void LogDelegate(String^, String^);
		void Log(String^ header, String^ body)
		{
			txtChat->AppendText("[" + header + "] ");
			// + body + "\n";
			for (int i = 0; i < body->Length; i++)
			{
				if (body->ToCharArray()[i] == '&' && i + 1 != body->Length)
				{
					if (textColors->ContainsKey(body->ToCharArray()[i + 1]))
					{
						Dictionary<char, int>::Enumerator iter = textColors->GetEnumerator();
						while (iter.Current.Key != body->ToCharArray()[i + 1])
						{
							iter.MoveNext();
						}
						int value = (int)iter.Current.Value;
						byte a = ((int)value & 0xff000000) >> 24;
						byte r = ((int)value & 0x00ff0000) >> 16;
						byte g = ((int)value & 0x0000ff00) >> 8;
						byte b = (int)value & 0x000000ff;
						txtChat->SelectionStart = txtChat->TextLength;
						txtChat->SelectionLength = 0;
						txtChat->SelectionColor = Color::FromArgb(a, r, g, b);
					}
				}
				else
				{
					if (body->ToCharArray()[i - 1 == -1 ? 0 : i - 1] != '&') txtChat->AppendText(Convert::ToString(body->ToCharArray()[i]));
				}
			}
			txtChat->AppendText("\n");
			txtChat->SelectionColor = txtChat->ForeColor;
			txtChat->SelectionStart = txtChat->Text->Length;
			txtChat->ScrollToCaret();
		}

		void Connect(String^ ip, int port, String^ username, String^ mppass)
		{
			try
			{
				serverIP = ip;
				serverPort = port;
				tcpClient = gcnew TcpClient();
				recievedExts = 0;

				btnConnect->Enabled = false;
				btnDisconnect->Enabled = false;
				btnSend->Enabled = false;

				Log("Connection", "Connecting to " + ip + ":" + port);
				tcpClient->Connect(ip, port);
				tcpThread = gcnew Thread(gcnew ThreadStart(this, &MainForm::tcpThread_Func));
				tcpStream = tcpClient->GetStream();

				btnConnect->Enabled = false;
				btnDisconnect->Enabled = true;
				btnSend->Enabled = true;

				connected = true;
				tcpThread->Start();
				Log("Connection", "Connected to " + ip + ":" + port);

				// Login packet
				Log("Connection", "Authenticating as " + username + " (MPPass: " + mppass + ")");
				username = username->PadRight(64, ' ');
				mppass = mppass->PadRight(64, ' ');
				List<byte>^ packet = gcnew List<byte>();
				packet->AddRange(convertByteArray(gcnew array<byte> { 0x00, 0x07 }));
				packet->AddRange(convertByteArray(Encoding::ASCII->GetBytes(username)));
				packet->AddRange(convertByteArray(Encoding::ASCII->GetBytes(mppass)));
				packet->AddRange(convertByteArray(gcnew array<byte> { 0x42 }));
				tcpStream->Write(packet->ToArray(), 0, packet->ToArray()->Length);
			}
			catch (Exception^ ex)
			{
				connected = true;
				Disconnect();

				Log("Connection", "Unable to connect: " + ex->Message);
				MessageBox::Show("Unable to connect: " + ex->Message, "Connection Error", 
					MessageBoxButtons::OK, MessageBoxIcon::Error);
			}
		}

		delegate void DisconnectDelegate();
		void Disconnect()
		{
			if (!connected) return;
			connected = false;
			btnConnect->Enabled = true;
			btnDisconnect->Enabled = false;
			btnSend->Enabled = false;
			serverIP = nullptr;
			serverPort = 0;
			delete tcpClient;
			delete tcpStream;
			delete tcpThread;
			Log("Connection", "Disconnected");
		}

		void tcpThread_Func()
		{
			while (connected)
			{
				try
				{
					if (tcpStream->DataAvailable)
					{
						int packetID = tcpStream->ReadByte();
						if (packetSizeMap->ContainsKey(packetID))
						{
							int packetSize = packetSizeMap[packetID];
							array<byte>^ buffer = gcnew array<byte>(packetSize);

							int readBytesTotal = 0;
							int readBytes = readBytesTotal = tcpStream->Read(buffer, 0, packetSize);

							while (readBytesTotal < packetSize)
							{
								readBytes = tcpStream->Read(buffer, readBytes, packetSize - readBytes);
								readBytesTotal += readBytes;
							}

							this->Invoke(gcnew ReceivedPacketDelegate(this, &MainForm::ReceivedPacket), (byte)packetID, buffer);
						}
						else 
						{
							this->Invoke(gcnew LogDelegate(this, &MainForm::Log), 
								"Connection", "Received invalid packet " + packetID + "! Disconnecting...");
							throw gcnew SocketException();
						}
					} 
					else 
					{
						if (connected && tcpClient->Client->Poll(1, SelectMode::SelectRead))
						{
							array<byte>^ buffer = gcnew array<byte>(1);
							if (connected && tcpClient->Client->Receive(buffer, 1, SocketFlags::Peek) == 0)
							{
								throw gcnew SocketException();
							}
						}
					}
				}
				catch (Exception^ ex)
				{
					if (IsDisposed) return;
					this->Invoke(gcnew DisconnectDelegate(this, &MainForm::Disconnect));

					if (!(ex->GetType() == SocketException::typeid || 
						ex->GetType() == IOException::typeid))
					{
						this->Invoke(gcnew LogDelegate(this, &MainForm::Log), 
							"Error", "Something went wrong: " + ex->Message);
						MessageBox::Show("Something went wrong: " + ex->Message, "Error", 
							MessageBoxButtons::OK, MessageBoxIcon::Error);
					}
				}
			}
		}

		void SendChatMessage(String^ message)
		{
			if (!connected) return;
			int j = 0;
			if (supportsLongerMessages) 
			{
				for (int i = 0; i < message->Length; i += 64)
				{
					j++;
				}
				messagesToSend = gcnew array<String^>(j);
				j = 0;
				for (int i = 0; i < message->Length; i += 64)
				{
					messagesToSend[j] = message->Substring(i, Math::Min(64, message->Length - i));
					j++;
				}
			}
			else
			{
				messagesToSend = gcnew array<String^>(1);
				messagesToSend[0] = message;
			}
			for (int i = 0; i < messagesToSend->Length; i++)
			{
				message = messagesToSend[i]->PadRight(64, ' ')->Substring(0, 64);
				List<byte>^ packet = gcnew List<byte>();
				packet->AddRange(convertByteArray(gcnew array<byte> { 0x0d, i == messagesToSend->Length - 1 ? 0x00 : 0xFF }));
				packet->AddRange(convertByteArray(Encoding::ASCII->GetBytes(message)));
				tcpStream->Write(packet->ToArray(), 0, packet->ToArray()->Length);
			}
		}

		void SendCpeInfo()
		{
			if (!connected) return;
			List<byte>^ packet = gcnew List<byte>();
			packet->AddRange(convertByteArray(gcnew array<byte> { 0x10 }));
			packet->AddRange(convertByteArray(Encoding::ASCII->GetBytes(clientName->PadRight(64, ' '))));
			packet->AddRange(convertByteArray(gcnew array<byte> { 0x00, 0x05 }));
			tcpStream->Write(packet->ToArray(), 0, packet->ToArray()->Length);

			SendExtInfo("EmoteFix", 1);
			SendExtInfo("MessageTypes", 1);
			SendExtInfo("LongerMessages", 1);
			SendExtInfo("FullCP437", 1);
			SendExtInfo("TextColors", 1);
		}

		void SendExtInfo(String^ extName, int^ extVer)
		{
			if (!connected) return;
			List<byte>^ packet = gcnew List<byte>();
			packet->AddRange(convertByteArray(gcnew array<byte> { 0x11 }));
			packet->AddRange(convertByteArray(Encoding::ASCII->GetBytes(extName->PadRight(64, ' '))));
			byte byte1 = ((int)extVer & 0xff000000) >> 24;
			byte byte2 = ((int)extVer & 0x00ff0000) >> 16;
			byte byte3 = ((int)extVer & 0x0000ff00) >> 8;
			byte byte4 = (int)extVer & 0x000000ff;
			packet->AddRange(convertByteArray(gcnew array<byte> { byte1, byte2, byte3, byte4 }));
			tcpStream->Write(packet->ToArray(), 0, packet->ToArray()->Length);
		}

		delegate void ReceivedPacketDelegate(byte, array<byte>^);
		void ReceivedPacket(byte id, array<byte>^ data)
		{
			//Log("Connection", "Received packet " + id + ": " + BitConverter::ToString(data));

			// 0x00: Login packet
			// 0x0d: Chat packet
			// 0x0e: Kick packet
			// 0x10: CPE ExtInfo
			// 0x11: CPE ExtEntry
			// 0x27: CPE SetTextColor
			if (id == 0x00)
			{
				array<byte>^ serverNameRaw = gcnew array<byte>(64);
				array<byte>^ serverMOTDRaw = gcnew array<byte>(64);
				Array::Copy(data, 1, serverNameRaw, 0, 64);
				Array::Copy(data, 65, serverMOTDRaw, 0, 64);

				String^ serverName = Encoding::UTF8->GetString(serverNameRaw);
				String^ serverMOTD = Encoding::UTF8->GetString(serverMOTDRaw);
				Log("Connection", "Server name: " + serverName);
				Log("Connection", "Server MOTD: " + serverMOTD);
				MessageBox::Show(serverMOTD, serverName,
					MessageBoxButtons::OK, MessageBoxIcon::Information);
			}
			else if (id == 0x0d)
			{
				array<byte>^ chatMessageRaw = gcnew array<byte>(64);
				byte playerID = data[0];
				Array::Copy(data, 1, chatMessageRaw, 0, 64);
				String^ chatMessage = Encoding::UTF8->GetString(chatMessageRaw);
				switch (playerID)
				{
					case 1:
						Log("Status1", chatMessage);
					case 2:
						Log("Status2", chatMessage);
					case 3:
						Log("Status3", chatMessage);
					case 11:
						Log("BottomStatus1", chatMessage);
					case 12:
						Log("BottomStatus2", chatMessage);
					case 13:
						Log("BottomStatus3", chatMessage);
					case 100:
						Log("Announcement", chatMessage);
					case 101:
						Log("BigAnnouncement", chatMessage);
					case 102:
						Log("SmallAnnouncement", chatMessage);
					default:
						Log("Chat", chatMessage);
				}
			}
			else if (id == 0x0e)
			{
				Disconnect();
				String^ kickReason = Encoding::UTF8->GetString(data);
				Log("Connection", "You have been kicked: " + kickReason);
				MessageBox::Show(kickReason, "You have been kicked",
					MessageBoxButtons::OK, MessageBoxIcon::Warning);
			}
			else if (id == 0x10)
			{
				array<byte>^ serverSoftwareRaw = gcnew array<byte>(64);
				array<byte>^ extCountRaw = gcnew array<byte>(2);
				Array::Copy(data, 0, serverSoftwareRaw, 0, 64);
				Array::Copy(data, 64, extCountRaw, 0, 2);

				String^ serverSoftwareName = Encoding::UTF8->GetString(serverSoftwareRaw);
				Log("CPE", "Server software: " + serverSoftwareName);
				extCount = (short)(((extCountRaw[0]) << 8) | extCountRaw[1]);
			}
			else if (id == 0x11)
			{
				array<byte>^ extNameRaw = gcnew array<byte>(64);
				array<byte>^ extVerRaw = gcnew array<byte>(4);
				Array::Copy(data, 0, extNameRaw, 0, 64);
				Array::Copy(data, 64, extVerRaw, 0, 4);

				String^ extName = Encoding::UTF8->GetString(extNameRaw);
				int extVer = (extVerRaw[0] << 24) | (extVerRaw[1] << 16) | (extVerRaw[2] << 8) | extVerRaw[3];
				CheckExtension(extName, extVer);
				recievedExts++;
				if (recievedExts == (short)extCount) SendCpeInfo();
			}
			else if (id == 0x27)
			{
				byte r = data[0];
				byte g = data[1];
				byte b = data[2];
				byte a = data[3];
				byte code = data[4];
				int argb = (a << 24) | (r << 16) | (g << 8) | b;
				char toDict = Convert::ToChar(code);
				if (textColors->ContainsKey(toDict))
				{
					textColors->Remove(toDict);
				}
				textColors->Add(toDict, argb);
			}
		}

		void CheckExtension(String^ extName, int^ extVer)
		{
			if (extName->Contains("LongerMessages"))
			{
				supportsLongerMessages = true;
				txtInput->MaxLength = Int32::MaxValue;
			}
			//TODO: ADD MORE EXTENSIONS HERE
		}

		Void btnSend_Click(Object^ sender, EventArgs^ e)
		{
			String^ message = txtInput->Text;
			txtInput->Text = nullptr;
			if (String::IsNullOrEmpty(message))
			{
				MessageBox::Show("The specified message is invalid!", "Error", 
					MessageBoxButtons::OK, MessageBoxIcon::Error);
				return;
			}
			SendChatMessage(message);
		}

		Void btnConnect_Click(Object^ sender, EventArgs^ e)
		{
			String^ serverIP = txtServer->Text;
			String^ username = txtUsername->Text;
			String^ mppass = txtMPPass->Text;
			int serverPort = 25565;
			if (serverIP->Contains(":"))
			{
				if (serverIP->Split(':')->Length != 2 || 
					!int::TryParse(serverIP->Split(':')[1], serverPort) || 
					serverPort < 1 || 
					serverPort > 65535)
				{
					MessageBox::Show("The specified server port is invalid!", "Error", 
						MessageBoxButtons::OK, MessageBoxIcon::Error);
					return;
				}
				serverIP = serverIP->Split(':')[0];
			}

			if (String::IsNullOrEmpty(serverIP))
			{
				MessageBox::Show("The specified server IP is invalid!", "Error", 
					MessageBoxButtons::OK, MessageBoxIcon::Error);
				return;
			}

			if (String::IsNullOrEmpty(username))
			{
				MessageBox::Show("The specified username is invalid!", "Error", 
					MessageBoxButtons::OK, MessageBoxIcon::Error);
				return;
			}

			Connect(serverIP, serverPort, username, mppass);
		}

		Void btnDisconnect_Click(Object^ sender, EventArgs^ e)
		{
			Disconnect();
		}

		Void btnClearChat_Click(Object^ sender, EventArgs^ e)
		{
			txtChat->Text = nullptr;
		}

		Void MainForm_FormClosing(Object^ sender, FormClosingEventArgs^ e)
		{
			Disconnect();
		}
	};
}
//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.

// urfd -- The universal reflector
// Copyright © 2021 Thomas A. Early N7TAE
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "Main.h"
#include <string.h>
#include "CodecStream.h"
#include "DVFramePacket.h"
#include "Reflector.h"

////////////////////////////////////////////////////////////////////////////////////////
// constructor

CCodecStream::CCodecStream(CPacketStream *PacketStream, uint16_t streamid, ECodecType type, std::shared_ptr<CUnixDgramReader> reader)
{
	keep_running = true;
	m_uiStreamId = streamid;
	m_uiPid = 0;
	m_eCodecIn = type;
	m_fPingMin = -1;
	m_fPingMax = -1;
	m_fPingSum = 0;
	m_fPingCount = 0;
	m_uiTotalPackets = 0;
	m_uiTimeoutPackets = 0;
	m_PacketStream = PacketStream;
	m_TCReader = reader;
}

////////////////////////////////////////////////////////////////////////////////////////
// destructor

CCodecStream::~CCodecStream()
{
	// close socket
	m_TCReader->Close();

	// kill threads
	keep_running = false;
	if ( m_Future.valid() )
	{
		m_Future.get();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// initialization

bool CCodecStream::Init()
{
	m_TCWriter.SetUp("ReftoTC");
	keep_running = true;
	m_Future = std::async(std::launch::async, &CCodecStream::Thread, this);

	return true;
}

void CCodecStream::Close(void)
{
	// close socket
	keep_running = false;
	m_TCReader->Close();

	// kill threads
	if ( m_Future.valid() )
	{
		m_Future.get();
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// get

bool CCodecStream::IsEmpty(void) const
{
	return (m_LocalQueue.empty() && m_PacketStream->empty());
}

////////////////////////////////////////////////////////////////////////////////////////
// thread

void CCodecStream::Thread()
{
	while (keep_running)
	{
		Task();
	}
}

void CCodecStream::Task(void)
{
	STCPacket pack;

	// any packet from transcoder
	if ( ! m_TCReader->Receive(&pack, 5) )
	{
		// tickle
		m_TimeoutTimer.start();

		// update statistics
		double ping = m_StatsTimer.time();
		if ( m_fPingMin == -1 )
		{
			m_fPingMin = ping;
			m_fPingMax = ping;

		}
		else
		{
			m_fPingMin = MIN(m_fPingMin, ping);
			m_fPingMax = MAX(m_fPingMax, ping);

		}
		m_fPingSum += ping;
		m_fPingCount += 1;

		// pop the original packet
		if ( !m_LocalQueue.empty() )
		{
			auto Packet = m_LocalQueue.pop();
			auto Frame = (CDvFramePacket *)Packet.get();
			// todo: check the PID
			// update content with transcoded data
			Frame->SetCodecData(&pack);
			// and push it back to client
			m_PacketStream->Lock();
			m_PacketStream->push(Packet);
			m_PacketStream->Unlock();
		}
		else
		{
			std::cout << "Unexpected transcoded packet received from transcoder" << std::endl;
		}
	}

	// anything in our queue
	while ( !empty() )
	{
		// yes, pop it from queue
		auto Packet = pop();
		auto Frame = (CDvFramePacket *)Packet.get();

		// yes, send to transcoder
		// this assume that thread pushing the Packet
		// have verified that the CodecStream is connected
		// and that the packet needs transcoding
		m_StatsTimer.start();
		m_uiTotalPackets++;
		m_TCWriter.Send(Frame->GetCodecPacket());

		// and push to our local queue
		m_LocalQueue.push(Packet);
	}

	// handle timeout
	if ( !m_LocalQueue.empty() && (m_TimeoutTimer.time() >= (TRANSCODER_AMBEPACKET_TIMEOUT/1000.0f)) )
	{
		//std::cout << "transcoder packet timeout" << std::endl;
		m_uiTimeoutPackets++;
	}
}
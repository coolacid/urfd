//  Copyright © 2015 Jean-Luc Deltombe (LX3JL). All rights reserved.

// ulxd -- The universal reflector
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
#include "DPlusClient.h"


////////////////////////////////////////////////////////////////////////////////////////
// constructors

CDplusClient::CDplusClient()
{
	m_bDextraDongle = false;
}

CDplusClient::CDplusClient(const CCallsign &callsign, const CIp &ip, char reflectorModule)
	: CClient(callsign, ip, reflectorModule)
{
	m_bDextraDongle = false;
}

CDplusClient::CDplusClient(const CDplusClient &client)
	: CClient(client)
{
	m_bDextraDongle = client.m_bDextraDongle;
}

////////////////////////////////////////////////////////////////////////////////////////
// status

bool CDplusClient::IsAlive(void) const
{
	return (m_LastKeepaliveTime.time() < DPLUS_KEEPALIVE_TIMEOUT);
}

void CDplusClient::SetMasterOfModule(char c)
{
	CClient::SetMasterOfModule(c);
	SetReflectorModule(c);
}

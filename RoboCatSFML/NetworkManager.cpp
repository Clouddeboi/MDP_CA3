#include "RoboCatPCH.hpp"

NetworkManager::NetworkManager() :
	m_bytes_sent_this_frame(0),
	m_drop_packet_chance(0.f),
	m_simulated_latency(0.f)
{
}

NetworkManager::~NetworkManager()
{
}

bool NetworkManager::Init(uint16_t inPort)
{
	m_socket = SocketUtil::CreateUDPSocket(INET);
	SocketAddress ownAddress(INADDR_ANY, inPort);
	m_socket->Bind(ownAddress);

	LOG("Initializing NetworkManager at port %d", inPort);

	m_bytes_received_per_second = WeightedTimedMovingAverage(1.f);
	m_bytes_sent_per_second = WeightedTimedMovingAverage(1.f);

	//did we bind okay?
	if (m_socket == nullptr)
	{
		return false;
	}

	if (m_socket->SetNonBlockingMode(true) != NO_ERROR)
	{
		return false;
	}

	return true;
}

void NetworkManager::ProcessIncomingPackets()
{
	ReadIncomingPacketsIntoQueue();

	ProcessQueuedPackets();

	UpdateBytesSentLastFrame();

}

void NetworkManager::ReadIncomingPacketsIntoQueue()
{
	//should we just keep a static one?
	char packetMem[1500];
	int packetSize = sizeof(packetMem);
	InputMemoryBitStream inputStream(packetMem, packetSize * 8);
	SocketAddress fromAddress;

	//keep reading until we don't have anything to read ( or we hit a max number that we'll process per frame )
	int receivedPacketCount = 0;
	int totalReadByteCount = 0;

	while (receivedPacketCount < kMaxPacketsPerFrameCount)
	{
		int readByteCount = m_socket->ReceiveFrom(packetMem, packetSize, fromAddress);
		if (readByteCount == 0)
		{
			//nothing to read
			break;
		}
		else if (readByteCount == -WSAECONNRESET)
		{
			//port closed on other end, so DC this person immediately
			HandleConnectionReset(fromAddress);
		}
		else if (readByteCount > 0)
		{
			inputStream.ResetToCapacity(readByteCount);
			++receivedPacketCount;
			totalReadByteCount += readByteCount;

			//now, should we drop the packet?
			if (RoboMath::GetRandomFloat() >= m_drop_packet_chance)
			{
				//we made it
				//shove the packet into the queue and we'll handle it as soon as we should...
				//we'll pretend it wasn't received until simulated latency from now
				//this doesn't sim jitter, for that we would need to.....

				float simulatedReceivedTime = Timing::sInstance.GetTimef() + m_simulated_latency;
				m_packet_queue.emplace(simulatedReceivedTime, inputStream, fromAddress);
			}
			else
			{
				LOG("Dropped packet!", 0);
				//dropped!
			}
		}
		else
		{
			//uhoh, error? exit or just keep going?
		}
	}

	if (totalReadByteCount > 0)
	{
		m_bytes_received_per_second.UpdatePerSecond(static_cast<float>(totalReadByteCount));
	}
}

void NetworkManager::ProcessQueuedPackets()
{
	//look at the front packet...
	while (!m_packet_queue.empty())
	{
		ReceivedPacket& nextPacket = m_packet_queue.front();
		if (Timing::sInstance.GetTimef() > nextPacket.GetReceivedTime())
		{
			ProcessPacket(nextPacket.GetPacketBuffer(), nextPacket.GetFromAddress());
			m_packet_queue.pop();
		}
		else
		{
			break;
		}

	}

}

void NetworkManager::SendPacket(const OutputMemoryBitStream& inOutputStream, const SocketAddress& inFromAddress)
{
	int sentByteCount = m_socket->SendTo(inOutputStream.GetBufferPtr(), inOutputStream.GetByteLength(), inFromAddress);
	if (sentByteCount > 0)
	{
		m_bytes_sent_this_frame += sentByteCount;
	}
}

void NetworkManager::UpdateBytesSentLastFrame()
{
	if (m_bytes_sent_this_frame > 0)
	{
		m_bytes_sent_per_second.UpdatePerSecond(static_cast<float>(m_bytes_sent_this_frame));

		m_bytes_sent_this_frame = 0;
	}

}


NetworkManager::ReceivedPacket::ReceivedPacket(float inReceivedTime, InputMemoryBitStream& ioInputMemoryBitStream, const SocketAddress& inFromAddress) :
	m_received_time(inReceivedTime),
	m_from_address(inFromAddress),
	m_packet_buffer(ioInputMemoryBitStream)
{
}


void NetworkManager::AddNetworkIdToGameObjectMap(GameObjectPtr inGameObject)
{
	m_network_id_to_game_object_map[inGameObject->GetNetworkId()] = inGameObject;
}

void NetworkManager::RemoveNetworkIdToGameObjectMap(GameObjectPtr inGameObject)
{
	m_network_id_to_game_object_map.erase(inGameObject->GetNetworkId());
}
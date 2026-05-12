#include "RoboCatServerPCH.hpp"

void ReplicationManagerServer::ReplicateCreate(int inNetworkId, uint32_t inInitialDirtyState)
{
	mNetworkIdToReplicationCommand[inNetworkId] = ReplicationCommand(inInitialDirtyState);
}

void ReplicationManagerServer::ReplicateDestroy(int inNetworkId)
{
	mNetworkIdToReplicationCommand[inNetworkId].SetDestroy();
}

void ReplicationManagerServer::RemoveFromReplication(int inNetworkId)
{
	mNetworkIdToReplicationCommand.erase(inNetworkId);
}

void ReplicationManagerServer::SetStateDirty(int inNetworkId, uint32_t inDirtyState)
{
	mNetworkIdToReplicationCommand[inNetworkId].AddDirtyState(inDirtyState);
}

void ReplicationManagerServer::HandleCreateAckd(int inNetworkId)
{
	mNetworkIdToReplicationCommand[inNetworkId].HandleCreateAckd();
}

void ReplicationManagerServer::Write(OutputMemoryBitStream& inOutputStream, ReplicationManagerTransmissionData* ioTransmissionData)
{
	//Stay under the UDP safe payload limit (~1400 bytes)
	//Any objects that don't fit will remain dirty and be sent on the next tick
	const uint32_t kMaxPacketBytes = 1200;

	for (auto& pair : mNetworkIdToReplicationCommand)
	{
		if (inOutputStream.GetByteLength() >= kMaxPacketBytes)
		{
			break;
		}

		ReplicationCommand& replicationCommand = pair.second;
		if (replicationCommand.HasDirtyState())
		{
			int networkId = pair.first;

			inOutputStream.Write(networkId);

			ReplicationAction action = replicationCommand.GetAction();
			inOutputStream.Write(action, 2);

			uint32_t writtenState = 0;
			uint32_t dirtyState = replicationCommand.GetDirtyState();

			switch (action)
			{
			case RA_Create:
				writtenState = WriteCreateAction(inOutputStream, networkId, dirtyState);
				break;
			case RA_Update:
				writtenState = WriteUpdateAction(inOutputStream, networkId, dirtyState);
				break;
			case RA_Destroy:
				writtenState = WriteDestroyAction(inOutputStream, networkId, dirtyState);
				break;
			}

			ioTransmissionData->AddTransmission(networkId, action, writtenState);
			replicationCommand.ClearDirtyState(writtenState);
		}
	}
}

uint32_t ReplicationManagerServer::WriteCreateAction(OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState)
{
	GameObjectPtr gameObject = NetworkManagerServer::sInstance->GetGameObject(inNetworkId);
	inOutputStream.Write(gameObject->GetClassId());
	return gameObject->Write(inOutputStream, inDirtyState);
}

uint32_t ReplicationManagerServer::WriteUpdateAction(OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState)
{
	GameObjectPtr gameObject = NetworkManagerServer::sInstance->GetGameObject(inNetworkId);
	uint32_t writtenState = gameObject->Write(inOutputStream, inDirtyState);
	return writtenState;
}

uint32_t ReplicationManagerServer::WriteDestroyAction(OutputMemoryBitStream& inOutputStream, int inNetworkId, uint32_t inDirtyState)
{
	(void)inOutputStream;
	(void)inNetworkId;
	(void)inDirtyState;
	return inDirtyState;
}

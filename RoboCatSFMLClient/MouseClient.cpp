#include "RoboCatClientPCH.hpp"

namespace
{
	const char* kBlobTextures[] = {
		"GreenBlob",
		"PurpleBlob",
		"RedBlob",
		"YellowBlob"
	};
	const int kBlobTextureCount = 4;
}

MouseClient::MouseClient()
{
	mSpriteComponent.reset(new SpriteComponent(this));

	int startIndex = rand() % kBlobTextureCount;
	for (int i = 0; i < kBlobTextureCount; ++i)
	{
		int index = (startIndex + i) % kBlobTextureCount;
		TexturePtr tex = TextureManager::sInstance->GetTexture(kBlobTextures[index]);
		if (tex)
		{
			mSpriteComponent->SetTexture(tex);
			return;
		}
	}
}

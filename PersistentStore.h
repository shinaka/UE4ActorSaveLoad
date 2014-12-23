// jweinhart

#pragma once
#include "PersistentStore.generated.h"

/* We need to set ArIsSaveGame and use the ObjectAndNameAsString Proxy */
struct GALAXY_API FSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
	/**
	* Creates and initializes a new instance.
	*
	* @param InInnerArchive - The inner archive to proxy.
	*/
	FSaveGameArchive(FArchive& InInnerArchive)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};


/* Our Serialized Actor Data struct - ActorData is populated from the result of Actor->Serialize */
USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()
	FTransform ActorTransform;
	FString ActorClass;
	TArray<uint8> ActorData;

	friend FArchive& operator<<(FArchive& Ar, FActorSaveData& ActorData);
};

/* Struct for the "Root" save file */
USTRUCT()
struct FSaveGameData
{
	GENERATED_USTRUCT_BODY()
	FName GameID;
	FDateTime Timestamp;
	TArray<FActorSaveData> SavedActors;

	friend FArchive& operator<<(FArchive& Ar, FSaveGameData& GameData);

};

class GALAXY_API PersistentStore
{
public:
	static void SaveGame();
	static AActor* GetDebugTarget();
	static TArray<AActor*> GetEligibleActorsToSave();
	static void LoadGame();
};

// jweinhart

#include "Galaxy.h"
#include "PersistentStore.h"
#include "Engine/World.h"

#define SAVE_FILE "Test.sav"
#define GAME_ID "TestGame"

FArchive& operator<<(FArchive& Ar, FSaveGameData& GameData)
{
	Ar << GameData.GameID;
	Ar << GameData.Timestamp;
	Ar << GameData.SavedActors;

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FActorSaveData& ActorData)
{
	Ar << ActorData.ActorTransform;
	Ar << ActorData.ActorClass;
	Ar << ActorData.ActorData;

	return Ar;
}

void PersistentStore::LoadGame()
{
	TArray<uint8> LoadArray;
	if (!FFileHelper::LoadFileToArray(LoadArray, *FString(TEXT(SAVE_FILE))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Load Failed!"));
		return;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Load Succeeded!"));
	}

	if (LoadArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loaded file empty!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Save File Size: %d"), LoadArray.Num());

	FMemoryReader FromBinary = FMemoryReader(LoadArray, true);
	FromBinary.Seek(0);

	FSaveGameData SaveData;

	FromBinary << SaveData;

	UE_LOG(LogTemp, Warning, TEXT("Loaded data with Timestamp: %s"), *SaveData.Timestamp.ToString());
	FromBinary.FlushCache();
	LoadArray.Empty();
	FromBinary.Close();

	for (FActorSaveData ActorRecord : SaveData.SavedActors)
	{
		FVector SpawnPos = ActorRecord.ActorTransform.GetLocation();
		FRotator SpawnRot = ActorRecord.ActorTransform.Rotator();
		UClass* Result = FindObject<UClass>(ANY_PACKAGE, *ActorRecord.ActorClass);
		if (Result)
		{
			AActor* NewActor = GWorld->SpawnActor(Result, &SpawnPos, &SpawnRot);
			FMemoryReader MemoryReader(ActorRecord.ActorData, true);
			FSaveGameArchive Ar(MemoryReader);
			NewActor->Serialize(Ar);
			NewActor->SetActorTransform(ActorRecord.ActorTransform);
		}
	}
}

void PersistentStore::SaveGame()
{
	TArray<FActorSaveData> SavedActors;

	TArray<AActor*> ActorsToSave = GetEligibleActorsToSave();

	for (AActor* Actor : ActorsToSave)
	{
		if (Actor == nullptr)
		{
			continue;
		}

		FActorSaveData ActorRecord;
		ActorRecord.ActorClass = Actor->GetClass()->GetPathName();
		ActorRecord.ActorTransform = Actor->GetTransform();

		FMemoryWriter MemoryWriter(ActorRecord.ActorData, true);
		FSaveGameArchive Ar(MemoryWriter);

		Actor->Serialize(Ar);
		SavedActors.Add(ActorRecord);
		Actor->Destroy();
	}

	FSaveGameData SaveData;

	SaveData.GameID = TEXT(GAME_ID);
	SaveData.Timestamp = FDateTime::Now();
	SaveData.SavedActors = SavedActors;

	FBufferArchive ToBinary;

	ToBinary << SaveData;

	if (ToBinary.Num() <= 0)
	{
		return;
	}

	if (FFileHelper::SaveArrayToFile(ToBinary, *FString(TEXT(SAVE_FILE))))
	{
		UE_LOG(LogTemp, Warning, TEXT("Save Success!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Save Failed!"));
	}

	ToBinary.FlushCache();
	ToBinary.Empty();

	LoadGame();
	return;
}

TArray<AActor*> PersistentStore::GetEligibleActorsToSave()
{
	TArray<AActor*> Actors;
	
	//Hook up your actual actor aggregation here (find actors of a specific interface, etc)
	Actors.Add(GetDebugTarget());

	return Actors;
}

AActor* PersistentStore::GetDebugTarget()
{
	/* Stupid debug function to just get the actor we're looking at */
	APlayerController* PlayerController = GWorld->GetFirstPlayerController();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PlayerController);
	QueryParams.AddIgnoredActor(PlayerController->GetPawn());
	QueryParams.bTraceComplex = true;
	QueryParams.TraceTag = TEXT("GetDebugTarget");

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FVector CameraLoc = PlayerController->PlayerCameraManager->GetCameraLocation();
	FVector CameraRot = PlayerController->PlayerCameraManager->GetCameraRotation().Vector();
	FVector CheckEndPos = CameraLoc + CameraRot * 500.0f;

	FHitResult HitResult;

	AActor* DebugTarget = nullptr;

	if (GWorld->LineTraceSingle(HitResult, CameraLoc, CheckEndPos, QueryParams, ObjectQueryParams))
	{
		DebugTarget = HitResult.Actor.Get();
	}

	return DebugTarget;
}


#include "ActorPoolComponent.h"

#include "PoolableActorInterface.h"
#include "Engine/World.h"

UActorPoolComponent::UActorPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UActorPoolComponent::BeginPlay()
{
	Super::BeginPlay();
	PrewarmPool();
}

void UActorPoolComponent::PrewarmPool()
{
	if (!PooledActorClass)
	{
		return;
	}

	while (AvailableActors.Num() + ActiveActors.Num() < InitialPoolSize)
	{
		CreatePooledActor();
	}
}

AActor* UActorPoolComponent::AcquireActor(const FTransform& SpawnTransform)
{
	AActor* ActorToAcquire = nullptr;

	while (AvailableActors.Num() > 0 && !IsValid(ActorToAcquire))
	{
		ActorToAcquire = AvailableActors.Pop();
	}

	if (!IsValid(ActorToAcquire) && bAllowPoolExpansion)
	{
		ActorToAcquire = CreatePooledActor();
		if (IsValid(ActorToAcquire))
		{
			AvailableActors.Remove(ActorToAcquire);
		}
	}

	if (!IsValid(ActorToAcquire))
	{
		return nullptr;
	}

	PrepareActorForAcquire(ActorToAcquire, SpawnTransform);
	ActiveActors.AddUnique(ActorToAcquire);
	return ActorToAcquire;
}

void UActorPoolComponent::ReleaseActor(AActor* ActorToRelease)
{
	if (!IsValid(ActorToRelease))
	{
		return;
	}

	ActiveActors.Remove(ActorToRelease);
	PrepareActorForRelease(ActorToRelease);
	AvailableActors.AddUnique(ActorToRelease);
}

int32 UActorPoolComponent::GetAvailableCount() const
{
	return AvailableActors.Num();
}

int32 UActorPoolComponent::GetActiveCount() const
{
	return ActiveActors.Num();
}

AActor* UActorPoolComponent::CreatePooledActor()
{
	UWorld* World = GetWorld();
	if (!World || !PooledActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewActor = World->SpawnActor<AActor>(PooledActorClass, FTransform::Identity, SpawnParameters);
	if (!IsValid(NewActor))
	{
		return nullptr;
	}

	PrepareActorForRelease(NewActor);
	AvailableActors.AddUnique(NewActor);
	return NewActor;
}

void UActorPoolComponent::PrepareActorForAcquire(AActor* ActorToAcquire, const FTransform& SpawnTransform)
{
	ActorToAcquire->SetActorLocationAndRotation(
		SpawnTransform.GetLocation(),
		SpawnTransform.GetRotation(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	ActorToAcquire->SetActorHiddenInGame(false);
	ActorToAcquire->SetActorEnableCollision(true);
	ActorToAcquire->SetActorTickEnabled(true);

	if (ActorToAcquire->GetClass()->ImplementsInterface(UPoolableActorInterface::StaticClass()))
	{
		IPoolableActorInterface::Execute_OnAcquiredFromPool(ActorToAcquire);
	}
}

void UActorPoolComponent::PrepareActorForRelease(AActor* ActorToRelease)
{
	if (ActorToRelease->GetClass()->ImplementsInterface(UPoolableActorInterface::StaticClass()))
	{
		IPoolableActorInterface::Execute_OnReleasedToPool(ActorToRelease);
	}

	ActorToRelease->SetActorTickEnabled(false);
	ActorToRelease->SetActorEnableCollision(false);
	ActorToRelease->SetActorHiddenInGame(true);
	ActorToRelease->SetActorLocation(FVector(0.0, 0.0, -100000.0), false, nullptr, ETeleportType::TeleportPhysics);
}

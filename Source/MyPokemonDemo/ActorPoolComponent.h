#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorPoolComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPOKEMONDEMO_API UActorPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActorPoolComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Object Pool")
	TSubclassOf<AActor> PooledActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Object Pool", meta = (ClampMin = "0"))
	int32 InitialPoolSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Object Pool")
	bool bAllowPoolExpansion = true;

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void PrewarmPool();

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	AActor* AcquireActor(const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ReleaseActor(AActor* ActorToRelease);

	UFUNCTION(BlueprintPure, Category = "Object Pool")
	int32 GetAvailableCount() const;

	UFUNCTION(BlueprintPure, Category = "Object Pool")
	int32 GetActiveCount() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> AvailableActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActiveActors;

	AActor* CreatePooledActor();
	void PrepareActorForAcquire(AActor* ActorToAcquire, const FTransform& SpawnTransform);
	void PrepareActorForRelease(AActor* ActorToRelease);
};

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PoolableActorInterface.generated.h"

UINTERFACE(BlueprintType)
class MYPOKEMONDEMO_API UPoolableActorInterface : public UInterface
{
	GENERATED_BODY()
};

class MYPOKEMONDEMO_API IPoolableActorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Object Pool")
	void OnAcquiredFromPool();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Object Pool")
	void OnReleasedToPool();
};

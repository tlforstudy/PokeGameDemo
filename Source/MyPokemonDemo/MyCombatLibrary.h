// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyCombatLibrary.generated.h"

/**
 * 战斗工具函数库
 */
UCLASS()
class MYPOKEMONDEMO_API UMyCombatLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // 按速度从高到低排序 Actor 数组（直接修改原数组）
    UFUNCTION(BlueprintCallable, Category = "Combat")
    static void SortActorsBySpeed(UPARAM(ref) TArray<AActor*>& Actors);
};
// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCombatLibrary.h"
#include "UObject/UnrealType.h"

namespace
{
    int32 GetActorSpeed(const AActor& Actor)
    {
        int32 Speed = 0;

        FIntProperty* SpeedProperty = CastField<FIntProperty>(Actor.GetClass()->FindPropertyByName(TEXT("Speed")));
        if (SpeedProperty)
        {
            Speed = SpeedProperty->GetPropertyValue_InContainer(&Actor);
        }

        return Speed;
    }
}

void UMyCombatLibrary::SortActorsBySpeed(TArray<AActor*>& Actors)
{
    Actors.Sort([](const AActor& A, const AActor& B) {
        return GetActorSpeed(A) > GetActorSpeed(B);
    });

    // Shuffle only actors with the same speed after sorting.
    for (int32 GroupStart = 0; GroupStart < Actors.Num();)
    {
        const int32 GroupSpeed = GetActorSpeed(*Actors[GroupStart]);
        int32 GroupEnd = GroupStart + 1;

        while (GroupEnd < Actors.Num() && GetActorSpeed(*Actors[GroupEnd]) == GroupSpeed)
        {
            ++GroupEnd;
        }

        for (int32 Index = GroupEnd - 1; Index > GroupStart; --Index)
        {
            Actors.Swap(Index, FMath::RandRange(GroupStart, Index));
        }

        GroupStart = GroupEnd;
    }
}
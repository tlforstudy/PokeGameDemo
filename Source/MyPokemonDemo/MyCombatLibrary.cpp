// Fill out your copyright notice in the Description page of Project Settings.

#include "MyCombatLibrary.h"
#include "UObject/UnrealType.h"

void UMyCombatLibrary::SortActorsBySpeed(TArray<AActor*>& Actors)
{
    // 使用 C++ 的 Sort 算法，配合 Lambda 表达式
    Actors.Sort([](const AActor& A, const AActor& B) {
        // 使用反射获取 Speed 属性值（int 类型）
        int32 SpeedA = 0, SpeedB = 0;

        // 查找 A 中的 Speed 属性
        FIntProperty* PropA = CastField<FIntProperty>(A.GetClass()->FindPropertyByName(TEXT("Speed")));
        if (PropA)
        {
            SpeedA = PropA->GetPropertyValue_InContainer(&A);
        }

        // 查找 B 中的 Speed 属性
        FIntProperty* PropB = CastField<FIntProperty>(B.GetClass()->FindPropertyByName(TEXT("Speed")));
        if (PropB)
        {
            SpeedB = PropB->GetPropertyValue_InContainer(&B);
        }

        // 速度高的排在前面（降序）
        return SpeedA > SpeedB;
    });
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KismetMultiFPSLibrary.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FDeathMatchPlayerData
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite)
	FName PlayerName;

	UPROPERTY(BlueprintReadWrite)
	int PlayerScore;

	FDeathMatchPlayerData()
	{
		PlayerName=TEXT("");
		PlayerScore=0;
	}
};

UCLASS()
class MULTIFPSDEMO_API UKismetMultiFPSLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable,Category="Sort")
	static void SortValue(UPARAM(ref)TArray<FDeathMatchPlayerData>& Values);//告诉蓝图用引用传递
};

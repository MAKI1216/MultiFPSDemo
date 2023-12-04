// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundCue.h"
#include "WeaponBaseClient.generated.h"

UCLASS()
class MULTIFPSDEMO_API AWeaponBaseClient : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBaseClient();

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsFireAnimMontage;//开枪手臂动画

	UPROPERTY(EditAnywhere)
	UAnimMontage* ClientArmsReloadAnimMontage;//换弹手臂动画
	
	UPROPERTY(EditAnywhere)
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditAnywhere)
	int FPArmsBlendPose;//动画混合的数字是多少，持那种枪

	UPROPERTY(EditAnywhere)
	float FiledOfAimingView;//开镜视野多大
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent,Category="FPGunAnimation")
	void PlayShootAnimation();//射击枪的动画


	UFUNCTION(BlueprintImplementableEvent,Category="FPGunAnimation")
	void PlayReloadAnimation();//换弹枪的动画
	
	void DisplayWeaponEffect();
};


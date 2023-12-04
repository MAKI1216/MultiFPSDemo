// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WeaponBaseClient.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "WeaponBaseServer.generated.h"

UENUM()
enum class EWeaponType :uint8
{
	Ak47 UMETA(DisplayName="AK47"),
	M4A1 UMETA(DisplayName="M4A1"),
	MP7 UMETA(DisplayName="MP7"),
	DesertEagle UMETA(DisplayName="DesertEagle"),
	Sniper UMETA(DisplayName="Sniper"),
	EEND
};

UCLASS()
class MULTIFPSDEMO_API AWeaponBaseServer : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBaseServer();

	UPROPERTY(EditAnywhere)
	EWeaponType KindOfWeapon;

	UPROPERTY(EditAnywhere)
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditAnywhere)
	USphereComponent* SphereCollision;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere)
	int32 GunCurrentAmmo;//枪体当前子弹数量

	UPROPERTY(EditAnywhere,Replicated)//服务器改变，服务器页改变
	int32 ClipCurrentAmmo;//弹夹当前子弹数量

	UPROPERTY(EditAnywhere)
	int32 ClipMaxAmmo;//弹夹最大子弹数量

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTpBoysShootAnimMontage;//第三人称射击动画蒙太奇

	UPROPERTY(EditAnywhere)
	UAnimMontage* ServerTpBoysReloadAnimMontage;//第三人称换弹动画蒙太奇
	
	UPROPERTY(EditAnywhere)
	float BulletDistance;//子弹射击距离

	UPROPERTY(EditAnywhere)
	UMaterialInterface* BulletDecalMaterial;//弹孔贴花

	UPROPERTY(EditAnywhere)
	float BaseDamage;//基础伤害

	UPROPERTY(EditAnywhere)
	bool IsAutomatic;//是否自动射击

	UPROPERTY(EditAnywhere)
	float AutomaticRate;//自动射击频率

	UPROPERTY(EditAnywhere)
	UCurveFloat* VerticalRecoilCurve;//垂直后座力曲线

	UPROPERTY(EditAnywhere)
	UCurveFloat* HorizontalRecoilCurve;//水平后座力曲线

	UPROPERTY(EditAnywhere)
	float MovingFireRandomRange;//跑步射击时候用于计算射线检测的偏移值范围
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSubclassOf<AWeaponBaseClient> ClientWeaponBaseBPClass;

	UPROPERTY(EditAnywhere,Category="SpreadWeaponData")
	float SpreadWeaponShootCallBackRate;//清空散射调用的时间(手枪用)

	UPROPERTY(EditAnywhere,Category="SpreadWeaponData")
	float SpreadWeaponMaxIndex;//散射上限每次增加多少(手枪用)

	UPROPERTY(EditAnywhere,Category="SpreadWeaponData")
	float SpreadWeaponMinIndex;//散射下线每次增加多少(手枪用)
	UFUNCTION()
	void OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool bFromSweep,
	                                                  const FHitResult& SweepResult);

	UFUNCTION()
	void EquipWeapon();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiShottingEffect();//枪口闪光效果多播
	void MultiShottingEffect_Implementation();
	bool MultiShottingEffect_Validate();
};

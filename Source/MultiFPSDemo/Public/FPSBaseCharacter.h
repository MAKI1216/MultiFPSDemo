// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MultiFPSPlayerController.h"
#include "WeaponBaseServer.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "FPSBaseCharacter.generated.h"

UCLASS()
class MULTIFPSDEMO_API AFPSBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AFPSBaseCharacter();

	UFUNCTION()
	void DelayBeginPlayCallBack();//开始创建蓝图不成功延时几秒再创建
private:
#pragma region Component
	UPROPERTY(Category=Character,VisibleAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	UCameraComponent* PlayerCamera;

	UPROPERTY(Category=Character,VisibleAnywhere,BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	USkeletalMeshComponent* FPArmMesh;

	UPROPERTY(Category=Character,BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	UAnimInstance* ClientArmsAnimBP;//手臂动画蓝图
	UPROPERTY(Category=Character,BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	UAnimInstance* ServerBodysAnimBP;//身体动画蓝图

	UPROPERTY(BlueprintReadOnly,meta=(AllowPrivateAccess="true"))
	AMultiFPSPlayerController* FPSPlayerController;

	UPROPERTY(EditAnywhere)
	EWeaponType TestStartWeapon;
#pragma endregion 
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#pragma region InputEvent
	void MoveForward(float value);
	void MoveRight(float value);
	void JumpAction();
	void StopJumpAction();
	void LowSeppedWalkAction();
	void NormalSpeedWalkAction();
	
	void InputFirePressed();
	void InputFireReleased();

	void InputReload();

	void InputAimingPressed();
	void InputAimingReleased();
#pragma endregion


private:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
#pragma region Fire
public:
	//计时器
	FTimerHandle AutomaticFireTimerHandle;
	void AutomaticFire();

	//后座力
	float NewVerticalRecoilAmount;//上一帧垂直后座力数值
	float OldVerticalRecoilAmount;//本次垂直后座力数值
	float VerticalRecoilAmount;//垂直后座力数值
	float RecoilXCoordPerShoot;//每次射击在曲线图中的x坐标

	float NewHorizontalRecoilAmount;//上一帧水平后座力数值
	float OldHorizontalRecoilAmount;//本次水平后座力数值
	float HorizontalRecoilAmount;//水平后座力数值
	
	void ResetRecoil();//重新设置后座力相关变量

	float PistolSpreadMin=0;//手枪散射最小值
	float PistolSpreadMax=0;//手枪散射最大值
	
	//步枪射击相关方法
	void FireWeaponPrimary();
	void StopFireWeaponPrimary();
	void RifleLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//步枪射线检测

	//手枪射击相关方法
	void FireWeaponSecondary();
	void StopFireWeaponSecondary();
	void PistolLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//手枪射线检测
	UFUNCTION()
	void DelaySpreadWeaponShootCallBack();//清空散射范围

	//狙击枪射击相关
	void FireWeaponSniper();
	void StopFireWeaponSniper();
	void SniperLineTrace(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//狙击枪射线检测
	UPROPERTY(Replicated)
	bool IsAiming;//是否瞄准
	UPROPERTY(VisibleAnywhere,Category="SniperUI")
	UUserWidget* WidgetScope;//瞄准镜ui
	UPROPERTY(EditAnywhere,Category="SniperUI")
	TSubclassOf<UUserWidget> SniperScopeBPClass;//瞄准镜ui蓝图类
	UFUNCTION()
	void DelaySniperShootCallBack();//狙击枪射击后回调修改isfiring
	
	
	//Reload
	UPROPERTY(Replicated)
	bool IsFiring;//是否在射击，需要同步
	UPROPERTY(Replicated)
	bool IsReloading;//是否在换弹，需要同步

	UFUNCTION()
	void DelayPlayArmReloadCallBack();//播放完手臂换弹动作的回调
	
	void DamagePlayer(UPhysicalMaterial* PhysicalMaterial,AActor* DamagedActor,FVector HitFromDirection,FHitResult& HitInfo);

	UFUNCTION()
	void OnHit(AActor* DamagedActor, float Damage, class AController* InstigatedBy, FVector HitLocation, class UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection ,const class UDamageType* DamageType, AActor* DamageCauser );

	UPROPERTY(EditAnywhere)
	float Health;

	void DeathMatchDeath(AActor* DamageActor);//死亡竞赛死亡逻辑
#pragma endregion 

#pragma region Weapon
public:
	void EquipPrimary(AWeaponBaseServer* WeaponBaseServer);//装备主武器
	void EquipSecondary(AWeaponBaseServer* WeaponBaseServer);//装备副武器

	UFUNCTION(BlueprintImplementableEvent)
	void UpdateFPArmsBlendPose(int NewIndex);//修改手臂动画混合（持哪一种枪）
	
private:
	UPROPERTY(meta=(AllowPrivateAccess="true"),Replicated)
	EWeaponType ActiveWeapon;
	
	UPROPERTY(meta=(AllowPrivateAccess="true"))
	AWeaponBaseServer* ServerPrimaryWeapon;//服务端主武器
	
	UPROPERTY(meta=(AllowPrivateAccess="true"))
	AWeaponBaseServer* ServerSecondaryWeapon;//服务端副武器
	
	UPROPERTY(meta=(AllowPrivateAccess="true"))
	AWeaponBaseClient* ClientPrimaryWeapon;//客户端主武器

	UPROPERTY(meta=(AllowPrivateAccess="true"))
	AWeaponBaseClient* ClientSecondaryWeapon;//客户端副武器

	AWeaponBaseClient* GetCurrentClientFPArmsWeaponActor();//获取客户端当前武器
	AWeaponBaseServer* GetCurrentServerFPArmsWeaponActor();//获取服务端当前武器
	
	void StartWithKindOfWeapon();

	void PurchaseWeapon(EWeaponType WeaponType);
#pragma endregion 

#pragma region NetWorking
public:
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerLowSpeedWalkAction();
	void ServerLowSpeedWalkAction_Implementation();
	bool ServerLowSpeedWalkAction_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerNormalSpeedWalkAction();
	void ServerNormalSpeedWalkAction_Implementation();
	bool ServerNormalSpeedWalkAction_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireRifleWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//步枪服务端逻辑
	void ServerFireRifleWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFireRifleWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFirePistolWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//手枪服务端逻辑
	void ServerFirePistolWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFirePistolWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerFireSniperWeapon(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);//狙击枪服务端逻辑
	void ServerFireSniperWeapon_Implementation(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);
	bool ServerFireSniperWeapon_Validate(FVector CameraLocation,FRotator CameraRotation,bool IsMoving);

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerReloadPrimary();//主武器重装子弹
	void ServerReloadPrimary_Implementation();
	bool ServerReloadPrimary_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerReloadSecondary();//副武器重装子弹
	void ServerReloadSecondary_Implementation();
	bool ServerReloadSecondary_Validate();
	
	UFUNCTION(Server,Reliable,WithValidation)
	void ServerStopFiring();//停止射击，修改isfiring变量
	void ServerStopFiring_Implementation();
	bool ServerStopFiring_Validate();

	UFUNCTION(Server,Reliable,WithValidation)
	void ServerSetAiming(bool AimingState);//修改isaiming变量
	void ServerSetAiming_Implementation(bool AimingState);
	bool ServerSetAiming_Validate(bool AimingState);
	
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiShotting();//射击多播(身体动画多播)
	void MultiShotting_Implementation();
	bool MultiShotting_Validate();

	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiReloadAnimation();//换弹多播(身体换弹动画多播)
	void MultiReloadAnimation_Implementation();
	bool MultiReloadAnimation_Validate();
	
	UFUNCTION(NetMulticast,Reliable,WithValidation)
	void MultiSpawnBulletDecal(FVector Location,FRotator Rotation);//射击生成弹孔多播
	void MultiSpawnBulletDecal_Implementation(FVector Location,FRotator Rotation);
	bool MultiSpawnBulletDecal_Validate(FVector Location,FRotator Rotation);
	
	UFUNCTION(Client,Reliable)
	void ClientEquipFPArmsPrimary();//装备主武器到手上

	UFUNCTION(Client,Reliable)
	void ClientEquipFPArmsSecondary();//装备副武器到手上
	
	UFUNCTION(Client,Reliable)
	void ClientFire();

	UFUNCTION(Client,Reliable)
	void ClientUpdateAmmoUI(int32 ClipCurrentAmmo,int32 GunCurrentAmmo);

	UFUNCTION(Client,Reliable)
	void ClientUpdateHealthUI(float NewHealth);//更新血量ui

	UFUNCTION(Client,Reliable)
	void ClientRecoil();//客户端后座力

	UFUNCTION(Client,Reliable)
	void ClientReload();//客户端重装子弹

	UFUNCTION(Client,Reliable)
	void ClientAiming();//客户端瞄准

	UFUNCTION(Client,Reliable)
	void ClientEndAiming();//客户端结束瞄准

	UFUNCTION(Client,Reliable)
	void ClientDeathMatchDeath();//客户端死亡竞赛模式死亡删除枪械
#pragma  endregion 
};




// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiFPSDemo/Public/FPSBaseCharacter.h"

#include "Blueprint/UserWidget.h"
#include "Components/DecalComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

// Sets default values
AFPSBaseCharacter::AFPSBaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

#pragma region Component
	PlayerCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	if(PlayerCamera)
	{
		PlayerCamera->SetupAttachment(RootComponent);
	}
	FPArmMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FPArmMesh"));
	if(FPArmMesh)
	{
		FPArmMesh->SetupAttachment(PlayerCamera);
		FPArmMesh->SetOnlyOwnerSee(true);
	}

	Mesh->SetOwnerNoSee(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//仅仅用于射线检测，物理世界碰撞用的是胶囊体
	Mesh->SetCollisionObjectType(ECC_Pawn);
#pragma endregion 
}

void AFPSBaseCharacter::DelayBeginPlayCallBack()
{
	FPSPlayerController=Cast<AMultiFPSPlayerController>(GetController());
	if(FPSPlayerController)
	{
		FPSPlayerController->CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo LatentActionInfo;
		LatentActionInfo.CallbackTarget=this;//触发物体为自身
		LatentActionInfo.ExecutionFunction=FName("DelayBeginPlayCallBack");
		LatentActionInfo.UUID=FMath::Rand();
		LatentActionInfo.Linkage=0;
		//延迟到0.5s完进行回调
		UKismetSystemLibrary::Delay(this,0.5,LatentActionInfo);
	}
}

#pragma region Engine
// Called when the game starts or when spawned
void AFPSBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	IsReloading=false;
	IsFiring=false;
	IsAiming=false;
	
	Health=100.0f;
	OnTakePointDamage.AddDynamic(this,&AFPSBaseCharacter::OnHit);//给应用伤害添加回调


	ClientArmsAnimBP=FPArmMesh->GetAnimInstance();//获取动画蓝图
	ServerBodysAnimBP=Mesh->GetAnimInstance();//获取身体动画蓝图
	
	FPSPlayerController=Cast<AMultiFPSPlayerController>(GetController());
	if(FPSPlayerController)
	{
		FPSPlayerController->CreatePlayerUI();
	}
	else
	{
		FLatentActionInfo LatentActionInfo;
		LatentActionInfo.CallbackTarget=this;//触发物体为自身
		LatentActionInfo.ExecutionFunction=FName("DelayBeginPlayCallBack");
		LatentActionInfo.UUID=FMath::Rand();
		LatentActionInfo.Linkage=0;
		//延迟到0.5s完进行回调
		UKismetSystemLibrary::Delay(this,0.5,LatentActionInfo);
	}

	StartWithKindOfWeapon();
}

void AFPSBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//添加需要同步的属性
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,IsFiring,COND_None);//同步添加，类，属性，同步条件
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,IsReloading,COND_None);//同步添加，类，属性，同步条件
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,ActiveWeapon,COND_None);//同步添加，类，属性，同步条件
	DOREPLIFETIME_CONDITION(AFPSBaseCharacter,IsAiming,COND_None);//同步添加，类，属性，同步条件
}

// Called every frame
void AFPSBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AFPSBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
		
	PlayerInputComponent->BindAction(TEXT("Jump"),IE_Pressed,this,&AFPSBaseCharacter::JumpAction);
	PlayerInputComponent->BindAction(TEXT("Jump"),IE_Released,this,&AFPSBaseCharacter::StopJumpAction);
	PlayerInputComponent->BindAction(TEXT("LowSpeedWalk"),IE_Pressed,this,&AFPSBaseCharacter::LowSeppedWalkAction);
	PlayerInputComponent->BindAction(TEXT("LowSpeedWalk"),IE_Released,this,&AFPSBaseCharacter::LowSeppedWalkAction);
	PlayerInputComponent->BindAction(TEXT("Fire"),IE_Pressed,this,&AFPSBaseCharacter::InputFirePressed);
	PlayerInputComponent->BindAction(TEXT("Fire"),IE_Released,this,&AFPSBaseCharacter::InputFireReleased);
	PlayerInputComponent->BindAction(TEXT("Reload"),IE_Pressed,this,&AFPSBaseCharacter::InputReload);
	PlayerInputComponent->BindAction(TEXT("Aiming"),IE_Pressed,this,&AFPSBaseCharacter::InputAimingPressed);
	PlayerInputComponent->BindAction(TEXT("Aiming"),IE_Released,this,&AFPSBaseCharacter::InputAimingReleased);
	
	PlayerInputComponent->BindAxis(TEXT("MoveForward"),this,&AFPSBaseCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"),this,&AFPSBaseCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"),this,&AFPSBaseCharacter::AddControllerYawInput);
	PlayerInputComponent->BindAxis(TEXT("LookUp"),this,&AFPSBaseCharacter::AddControllerPitchInput);
}
#pragma endregion 

#pragma region InputEvent
void AFPSBaseCharacter::MoveForward(float value)
{
	AddMovementInput(GetActorForwardVector(),value,false);
}

void AFPSBaseCharacter::MoveRight(float value)
{
	AddMovementInput(GetActorRightVector(),value,false);
}

void AFPSBaseCharacter::JumpAction()
{
	Jump();
}

void AFPSBaseCharacter::StopJumpAction()
{
	StopJumping();
}

void AFPSBaseCharacter::LowSeppedWalkAction()
{
	CharacterMovement->MaxWalkSpeed=300;
	ServerLowSpeedWalkAction();
}

void AFPSBaseCharacter::NormalSpeedWalkAction()
{
	CharacterMovement->MaxWalkSpeed=600;
	ServerNormalSpeedWalkAction();
}

void AFPSBaseCharacter::InputFirePressed()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			FireWeaponPrimary();
			break;
		}
	case EWeaponType::M4A1:
		{
			FireWeaponPrimary();
			break;
		}
	case EWeaponType::MP7:
		{
			FireWeaponPrimary();
			break;
		}
	case EWeaponType::DesertEagle:
		{
			FireWeaponSecondary();
			break;
		}
	case EWeaponType::Sniper:
		{
			FireWeaponSniper();
			break;
		}
	}
	
}

void AFPSBaseCharacter::InputFireReleased()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			StopFireWeaponPrimary();
			break;
		}
	case EWeaponType::M4A1:
		{
			StopFireWeaponPrimary();
			break;
		}
	case EWeaponType::MP7:
		{
			StopFireWeaponPrimary();
			break;
		}
	case EWeaponType::DesertEagle:
		{
			StopFireWeaponSecondary();
			break;
		}
	case EWeaponType::Sniper:
		{
			StopFireWeaponSniper();
			break;
		}
	}
}

void AFPSBaseCharacter::InputReload()
{
	if(!IsReloading)
	{
		if(!IsFiring&&!IsAiming)
		{
			switch (ActiveWeapon)
			{
			case EWeaponType::Ak47:
				{
					ServerReloadPrimary();
					break;
				}
			case EWeaponType::M4A1:
				{
					ServerReloadPrimary();
					break;
				}
			case EWeaponType::MP7:
				{
					ServerReloadPrimary();
					break;
				}
			case EWeaponType::DesertEagle:
				{
					ServerReloadSecondary();
					break;
				}
			case EWeaponType::Sniper:
				{
					ServerReloadPrimary();
					break;
				}
			}
		}
	}
}

void AFPSBaseCharacter::InputAimingPressed()
{
	//贴瞄准镜的ui，关闭枪体可见性，摄像头距离拉远（客户端）
	//更改isaiming(服务端)
	if(ActiveWeapon==EWeaponType::Sniper&&!IsReloading)
	{
		ServerSetAiming(true);
		ClientAiming();
	}
}

void AFPSBaseCharacter::InputAimingReleased()
{
	//贴瞄准镜的ui删除，打开枪体可见性，摄像头距离拉近
	if(ActiveWeapon==EWeaponType::Sniper)
	{
		ServerSetAiming(false);
		ClientEndAiming();
	}
}
#pragma endregion

#pragma region Weapon
void AFPSBaseCharacter::DeathMatchDeath(AActor* DamageActor)
{
	AWeaponBaseClient* CurrentClientFPArmsWeaponActor = GetCurrentClientFPArmsWeaponActor();
	AWeaponBaseServer* CurrentServerFPArmsWeaponActor = GetCurrentServerFPArmsWeaponActor();
	if(CurrentServerFPArmsWeaponActor)
	{
		CurrentServerFPArmsWeaponActor->Destroy();
	}
	if(CurrentClientFPArmsWeaponActor)
	{
		CurrentClientFPArmsWeaponActor->Destroy();
	}
	ClientDeathMatchDeath();
	AMultiFPSPlayerController* CurController = Cast<AMultiFPSPlayerController>(GetController());
	if(CurController)
	{
		CurController->DeathMatchDeath(DamageActor);
	}
}

void AFPSBaseCharacter::EquipPrimary(AWeaponBaseServer* WeaponBaseServer)
{
	if(ServerPrimaryWeapon)
	{
		
	}
	else
	{
		ServerPrimaryWeapon=WeaponBaseServer;
		ServerPrimaryWeapon->SetOwner(this);
		ServerPrimaryWeapon->K2_AttachToComponent(Mesh,TEXT("Weapon_Rifle"),EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);
		ActiveWeapon=ServerPrimaryWeapon->KindOfWeapon;
		ClientEquipFPArmsPrimary();
	}
}

void AFPSBaseCharacter::EquipSecondary(AWeaponBaseServer* WeaponBaseServer)
{
	if(ServerSecondaryWeapon)
	{
		
	}
	else
	{
		ServerSecondaryWeapon=WeaponBaseServer;
		ServerSecondaryWeapon->SetOwner(this);
		ServerSecondaryWeapon->K2_AttachToComponent(Mesh,TEXT("Weapon_Rifle"),EAttachmentRule::SnapToTarget,
			EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);
		ActiveWeapon=ServerSecondaryWeapon->KindOfWeapon;
		ClientEquipFPArmsSecondary();
	}
}

AWeaponBaseClient* AFPSBaseCharacter::GetCurrentClientFPArmsWeaponActor()
{
	switch (ActiveWeapon)
	{
		case EWeaponType::Ak47:
			{
				return ClientPrimaryWeapon;
			}
		case EWeaponType::M4A1:
			{
				return ClientPrimaryWeapon;
			}
	case EWeaponType::MP7:
			{
				return ClientPrimaryWeapon;
			}
	case EWeaponType::DesertEagle:
			{
				return ClientSecondaryWeapon;
			}
	case EWeaponType::Sniper:
			{
				return ClientPrimaryWeapon;
			}
	}
	return nullptr;
}

AWeaponBaseServer* AFPSBaseCharacter::GetCurrentServerFPArmsWeaponActor()
{
	switch (ActiveWeapon)
	{
	case EWeaponType::Ak47:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::M4A1:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::MP7:
		{
			return ServerPrimaryWeapon;
		}
	case EWeaponType::DesertEagle:
		{
			return ServerSecondaryWeapon;
		}
	case EWeaponType::Sniper:
		{
			return ServerPrimaryWeapon;
		}
	}
	
	return nullptr;
}

void AFPSBaseCharacter::StartWithKindOfWeapon()
{
	if (HasAuthority())
	{
		//如果当前在服务端，执行买武器
		PurchaseWeapon(static_cast<EWeaponType>(UKismetMathLibrary::RandomIntegerInRange(0,static_cast<int8>(EWeaponType::EEND)-1)));
	}
}

void AFPSBaseCharacter::PurchaseWeapon(EWeaponType WeaponType)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner=this;
	SpawnInfo.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	switch (WeaponType)
	{
		case ::EWeaponType::Ak47:
		{
				//动态拿到ak47类
				UClass* BlueprintVar=StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrints/Weapon/AK47/ServerBP_AK47.ServerBP_AK47_C'"));
				//末尾要加_C才能拿到
				//服务器上是spawn出来的，不会发生碰撞，所以要主动调用
				AWeaponBaseServer* WeaponBaseServer = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,
                                                    GetActorTransform(),
                                                    SpawnInfo);
				WeaponBaseServer->EquipWeapon();
				//ActiveWeapon=EWeaponType::Ak47;
				EquipPrimary(WeaponBaseServer);
				//武器会复制到客户端，客户端的武器和角色发送碰撞，自动装备
				break;
		}
	case ::EWeaponType::M4A1:
			{
				//动态拿到M4A1类
				UClass* BlueprintVar=StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrints/Weapon/M4A1/ServerBP_M4A1.ServerBP_M4A1_C'"));
				//末尾要加_C才能拿到
				//服务器上是spawn出来的，不会发生碰撞，所以要主动调用
				AWeaponBaseServer* WeaponBaseServer = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,
													GetActorTransform(),
													SpawnInfo);
				WeaponBaseServer->EquipWeapon();
				//ActiveWeapon=EWeaponType::M4A1;
				EquipPrimary(WeaponBaseServer);
				//武器会复制到客户端，客户端的武器和角色发送碰撞，自动装备
				break;
			}
	case ::EWeaponType::MP7:
			{
				//动态拿到MP7类
				UClass* BlueprintVar=StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrints/Weapon/MP7/ServerBP_MP7.ServerBP_MP7_C'"));
				//末尾要加_C才能拿到
				//服务器上是spawn出来的，不会发生碰撞，所以要主动调用
				AWeaponBaseServer* WeaponBaseServer = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,
													GetActorTransform(),
													SpawnInfo);
				WeaponBaseServer->EquipWeapon();
				//ActiveWeapon=EWeaponType::MP7;
				EquipPrimary(WeaponBaseServer);
				//武器会复制到客户端，客户端的武器和角色发送碰撞，自动装备
				break;
			}
	case ::EWeaponType::DesertEagle:
			{
				//动态拿到DesertEagle类
				UClass* BlueprintVar=StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrints/Weapon/DesertEagle/ServerBP_DesertEagle.ServerBP_DesertEagle_C'"));
				//末尾要加_C才能拿到
				//服务器上是spawn出来的，不会发生碰撞，所以要主动调用
				AWeaponBaseServer* WeaponBaseServer = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,
													GetActorTransform(),
													SpawnInfo);
				WeaponBaseServer->EquipWeapon();
				//ActiveWeapon=EWeaponType::DesertEagle;
				EquipSecondary(WeaponBaseServer);
				//武器会复制到客户端，客户端的武器和角色发送碰撞，自动装备
				break;
			}
	case ::EWeaponType::Sniper:
			{
				//动态拿到Sniper类
				UClass* BlueprintVar=StaticLoadClass(AWeaponBaseServer::StaticClass(),nullptr,TEXT("Blueprint'/Game/BluePrints/Weapon/Sniper/ServerBP_Sniper.ServerBP_Sniper_C'"));
				//末尾要加_C才能拿到
				//服务器上是spawn出来的，不会发生碰撞，所以要主动调用
				AWeaponBaseServer* WeaponBaseServer = GetWorld()->SpawnActor<AWeaponBaseServer>(BlueprintVar,
													GetActorTransform(),
													SpawnInfo);
				WeaponBaseServer->EquipWeapon();
				//ActiveWeapon=EWeaponType::Sniper;
				EquipSecondary(WeaponBaseServer);
				//武器会复制到客户端，客户端的武器和角色发送碰撞，自动装备
				break;
			}
		default:
			{
				
			}
	}
}
#pragma endregion 

#pragma region NetWorking

void AFPSBaseCharacter::ServerLowSpeedWalkAction_Implementation()
{
	CharacterMovement->MaxWalkSpeed=300;
}

bool AFPSBaseCharacter::ServerLowSpeedWalkAction_Validate()
{
	return true;
}

void AFPSBaseCharacter::ServerNormalSpeedWalkAction_Implementation()
{
	CharacterMovement->MaxWalkSpeed=600;
}

bool AFPSBaseCharacter::ServerNormalSpeedWalkAction_Validate()
{
	return true;
}

void AFPSBaseCharacter::ServerFireRifleWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	if(ServerPrimaryWeapon)
	{
		//多播(必须在服务端执行，谁调用谁多播)
		ServerPrimaryWeapon->MultiShottingEffect();
		ServerPrimaryWeapon->ClipCurrentAmmo-=1;

		//多播，播放身体动画蒙太奇
		MultiShotting();
		
		//客户端更新UI
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);
	}
	IsFiring=true;
	
	RifleLineTrace(CameraLocation,CameraRotation,IsMoving);
}

bool AFPSBaseCharacter::ServerFireRifleWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}

void AFPSBaseCharacter::ServerFirePistolWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	if(ServerSecondaryWeapon)
	{
		//过0.5清空散射范围
		FLatentActionInfo LatentActionInfo;
		LatentActionInfo.CallbackTarget=this;//触发物体为自身
		LatentActionInfo.ExecutionFunction=FName("DelaySpreadWeaponShootCallBack");
		LatentActionInfo.UUID=FMath::Rand();
		LatentActionInfo.Linkage=0;
		//延迟到手臂换弹动画执行完进行回调
		UKismetSystemLibrary::Delay(this,ServerSecondaryWeapon->SpreadWeaponShootCallBackRate,LatentActionInfo);
		
		//多播(必须在服务端执行，谁调用谁多播)
		ServerSecondaryWeapon->MultiShottingEffect();
		ServerSecondaryWeapon->ClipCurrentAmmo-=1;

		//多播，播放身体动画蒙太奇
		MultiShotting();
		
		//客户端更新UI
		ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo,ServerSecondaryWeapon->GunCurrentAmmo);

		IsFiring=true;
	
		PistolLineTrace(CameraLocation,CameraRotation,IsMoving);
	}
	
}

bool AFPSBaseCharacter::ServerFirePistolWeapon_Validate(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	return true;
}

void AFPSBaseCharacter::ServerFireSniperWeapon_Implementation(FVector CameraLocation, FRotator CameraRotation,
	bool IsMoving)
{
	if(ServerPrimaryWeapon)
	{
		//多播(必须在服务端执行，谁调用谁多播)
		ServerPrimaryWeapon->MultiShottingEffect();
		ServerPrimaryWeapon->ClipCurrentAmmo-=1;

		//多播，播放身体动画蒙太奇
		MultiShotting();
		
		//客户端更新UI
		ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);
	}

	if(ClientPrimaryWeapon)
	{
		FLatentActionInfo LatentActionInfo;
		LatentActionInfo.CallbackTarget=this;//触发物体为自身
		LatentActionInfo.ExecutionFunction=FName("DelaySniperShootCallBack");
		LatentActionInfo.UUID=FMath::Rand();
		LatentActionInfo.Linkage=0;
		//延迟到手臂射击动画执行完进行回调
		UKismetSystemLibrary::Delay(this,ClientSecondaryWeapon->ClientArmsFireAnimMontage->GetPlayLength(),LatentActionInfo);
	}
	IsFiring=true;
	
	SniperLineTrace(CameraLocation,CameraRotation,IsMoving);
}

bool AFPSBaseCharacter::ServerFireSniperWeapon_Validate(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	return true;
}


void AFPSBaseCharacter::ServerReloadPrimary_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		//如果枪体子弹大于0且弹夹子弹小于弹夹最大子弹才能换弹
		if (ServerPrimaryWeapon->GunCurrentAmmo>0&&
			ServerPrimaryWeapon->ClipCurrentAmmo<ServerPrimaryWeapon->ClipMaxAmmo)
		{
			//客户端手臂播放动画
			//服务器身体播放动画
			//数据更新
			//子弹ui更新
			ClientReload();
	
			//多播换弹动画
			MultiReloadAnimation();
			IsReloading=true;
			if(ClientPrimaryWeapon)
			{
				FLatentActionInfo LatentActionInfo;
				LatentActionInfo.CallbackTarget=this;//触发物体为自身
				LatentActionInfo.ExecutionFunction=FName("DelayPlayArmReloadCallBack");
				LatentActionInfo.UUID=FMath::Rand();
				LatentActionInfo.Linkage=0;
				//延迟到手臂换弹动画执行完进行回调
				UKismetSystemLibrary::Delay(this,ClientPrimaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(),LatentActionInfo);
			}
		}
	}
}

bool AFPSBaseCharacter::ServerReloadPrimary_Validate()
{
	return true;
}

void AFPSBaseCharacter::ServerReloadSecondary_Implementation()
{
	if(ServerSecondaryWeapon)
	{
		//如果枪体子弹大于0且弹夹子弹小于弹夹最大子弹才能换弹
		if (ServerSecondaryWeapon->GunCurrentAmmo>0&&
			ServerSecondaryWeapon->ClipCurrentAmmo<ServerSecondaryWeapon->ClipMaxAmmo)
		{
			//客户端手臂播放动画
			//服务器身体播放动画
			//数据更新
			//子弹ui更新
			ClientReload();
	
			//多播换弹动画
			MultiReloadAnimation();
			IsReloading=true;
			if(ClientSecondaryWeapon)
			{
				FLatentActionInfo LatentActionInfo;
				LatentActionInfo.CallbackTarget=this;//触发物体为自身
				LatentActionInfo.ExecutionFunction=FName("DelayPlayArmReloadCallBack");
				LatentActionInfo.UUID=FMath::Rand();
				LatentActionInfo.Linkage=0;
				//延迟到手臂换弹动画执行完进行回调
				UKismetSystemLibrary::Delay(this,ClientSecondaryWeapon->ClientArmsReloadAnimMontage->GetPlayLength(),LatentActionInfo);
			}
		}
	}
}

bool AFPSBaseCharacter::ServerReloadSecondary_Validate()
{
	return true;
}

void AFPSBaseCharacter::ServerStopFiring_Implementation()
{
	IsFiring=false;
}

bool AFPSBaseCharacter::ServerStopFiring_Validate()
{
	return true;
}

void AFPSBaseCharacter::ServerSetAiming_Implementation(bool AimingState)
{
	IsAiming=AimingState;
}

bool AFPSBaseCharacter::ServerSetAiming_Validate(bool AimingState)
{
	return true;
}

void AFPSBaseCharacter::MultiShotting_Implementation()
{
	AWeaponBaseServer* CurrentServerFPWeaponActor=GetCurrentServerFPArmsWeaponActor();//获取当前武器
	if (ServerBodysAnimBP)
	{
		if(CurrentServerFPWeaponActor)
		{
			ServerBodysAnimBP->Montage_Play(CurrentServerFPWeaponActor->ServerTpBoysShootAnimMontage);
		}
	}
}

bool AFPSBaseCharacter::MultiShotting_Validate()
{
	return true;
}

void AFPSBaseCharacter::MultiReloadAnimation_Implementation()
{
	AWeaponBaseServer* CurrentServerFPWeaponActor=GetCurrentServerFPArmsWeaponActor();//获取当前武器
	if(ServerBodysAnimBP)
	{
		if(CurrentServerFPWeaponActor)
		{
			//多播换弹身体动画
			ServerBodysAnimBP->Montage_Play(CurrentServerFPWeaponActor->ServerTpBoysReloadAnimMontage);
		}
	}
}

bool AFPSBaseCharacter::MultiReloadAnimation_Validate()
{
	return true;
}

void AFPSBaseCharacter::MultiSpawnBulletDecal_Implementation(FVector Location,FRotator Rotation)
{
	AWeaponBaseServer* CurrentServerFPWeaponActor=GetCurrentServerFPArmsWeaponActor();//获取当前武器
	if (CurrentServerFPWeaponActor)
	{
		UDecalComponent* Decal=UGameplayStatics::SpawnDecalAtLocation(GetWorld(),CurrentServerFPWeaponActor->BulletDecalMaterial,FVector(8,8,8),Location,Rotation,10);
		if(Decal)
		{
			Decal->SetFadeScreenSize(0.001);//设置距离屏幕暗淡距离
		}
	}
	
}

bool AFPSBaseCharacter::MultiSpawnBulletDecal_Validate(FVector Location,FRotator Rotation)
{
	return true;
}

void AFPSBaseCharacter::ClientEquipFPArmsSecondary_Implementation()
{
	if(ServerSecondaryWeapon)
	{
		if(ClientSecondaryWeapon)//如果不加这个，rpc之前执行一次，rpc后又执行一次，本地客户端就会有两个ak
			{
			
			}
		else
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner=this;
			SpawnInfo.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientSecondaryWeapon=GetWorld()->SpawnActor<AWeaponBaseClient>(ServerSecondaryWeapon->ClientWeaponBaseBPClass,
				GetActorTransform(),
				SpawnInfo);
			//不同武器的插槽不一样
			FName WeaponSocket=TEXT("WeaponSocket");
			ClientSecondaryWeapon->K2_AttachToComponent(FPArmMesh,WeaponSocket,EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);

			ClientUpdateAmmoUI(ServerSecondaryWeapon->ClipCurrentAmmo,ServerSecondaryWeapon->GunCurrentAmmo);//装备上武器就修改ui
			//手臂动画混合
			if(ClientSecondaryWeapon)
			{
				UpdateFPArmsBlendPose(ClientSecondaryWeapon->FPArmsBlendPose);
			}
		}
	}
}

void AFPSBaseCharacter::ClientReload_Implementation()
{
	//手臂换弹动画
	AWeaponBaseClient* CurrentClientFPArmsWeaponActor=GetCurrentClientFPArmsWeaponActor();//获取当前武器
	if(CurrentClientFPArmsWeaponActor)
	{
		UAnimMontage* ClientArmsReloadAnimMontage=CurrentClientFPArmsWeaponActor->ClientArmsReloadAnimMontage;
		ClientArmsAnimBP->Montage_Play(ClientArmsReloadAnimMontage);//手臂动画
		CurrentClientFPArmsWeaponActor->PlayReloadAnimation();//枪的动画
	}
	
}

void AFPSBaseCharacter::ClientAiming_Implementation()
{
	if(ClientPrimaryWeapon)
	{
		//设置枪和手臂不可见
		ClientPrimaryWeapon->SetActorHiddenInGame(true);
		if (FPArmMesh)
		{
			FPArmMesh->SetHiddenInGame(true);
		}

		//拉近相机视野
		if(PlayerCamera)
		{
			PlayerCamera->SetFieldOfView(ClientPrimaryWeapon->FiledOfAimingView);
		}
	}
	WidgetScope=CreateWidget(GetWorld(),SniperScopeBPClass);
	WidgetScope->AddToViewport();
}

void AFPSBaseCharacter::ClientEndAiming_Implementation()
{
	if(ClientPrimaryWeapon)
	{
		//设置枪和手臂可见
		ClientPrimaryWeapon->SetActorHiddenInGame(false);
		if (FPArmMesh)
		{
			FPArmMesh->SetHiddenInGame(false);
		}

		//还原相机视野
		if(PlayerCamera)
		{
			PlayerCamera->SetFieldOfView(90);//默认视野是90
		}
	}
	if(WidgetScope)
	{
		WidgetScope->RemoveFromViewport();
	}
}

void AFPSBaseCharacter::ClientDeathMatchDeath_Implementation()
{
	AWeaponBaseClient* CurrentClientFPArmsWeaponActor = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientFPArmsWeaponActor)
	{
		CurrentClientFPArmsWeaponActor->Destroy();
	}
}

void AFPSBaseCharacter::ClientRecoil_Implementation()
{
	UCurveFloat* VerticalRecoilCurve =nullptr;
	UCurveFloat* HorizontalRecoilCurve =nullptr;
	if(ServerPrimaryWeapon)
	{
		VerticalRecoilCurve=ServerPrimaryWeapon->VerticalRecoilCurve;
		HorizontalRecoilCurve=ServerPrimaryWeapon->HorizontalRecoilCurve;
	}
	RecoilXCoordPerShoot+=0.1;
	if(VerticalRecoilCurve)
	{
		NewVerticalRecoilAmount=VerticalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
	}

	if(HorizontalRecoilCurve)
	{
		NewHorizontalRecoilAmount=HorizontalRecoilCurve->GetFloatValue(RecoilXCoordPerShoot);
	}
	
	VerticalRecoilAmount=NewVerticalRecoilAmount-OldVerticalRecoilAmount;
	HorizontalRecoilAmount=NewHorizontalRecoilAmount-OldHorizontalRecoilAmount;
	
	if(FPSPlayerController)
	{
		FRotator ControllerRotator=FPSPlayerController->GetControlRotation();
		FPSPlayerController->SetControlRotation(FRotator(ControllerRotator.Pitch+VerticalRecoilAmount
			,ControllerRotator.Yaw+HorizontalRecoilAmount,
			ControllerRotator.Roll));
	}
	
	OldVerticalRecoilAmount=NewVerticalRecoilAmount;
}

void AFPSBaseCharacter::ClientUpdateHealthUI_Implementation(float NewHealth)
{
	if(FPSPlayerController)
	{
		FPSPlayerController->UpdateHealthUI(NewHealth);
	}
}


void AFPSBaseCharacter::ClientUpdateAmmoUI_Implementation(int32 ClipCurrentAmmo, int32 GunCurrentAmmo)
{
	if(FPSPlayerController)
	{
		FPSPlayerController->UpdateAmmoUI(ClipCurrentAmmo,GunCurrentAmmo);
	}
}

void AFPSBaseCharacter::ClientEquipFPArmsPrimary_Implementation()
{
	if(ServerPrimaryWeapon)
	{
		if(ClientPrimaryWeapon)//如果不加这个，rpc之前执行一次，rpc后又执行一次，本地客户端就会有两个ak
		{
			
		}
		else
		{
			FActorSpawnParameters SpawnInfo;
			SpawnInfo.Owner=this;
			SpawnInfo.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ClientPrimaryWeapon=GetWorld()->SpawnActor<AWeaponBaseClient>(ServerPrimaryWeapon->ClientWeaponBaseBPClass,
				GetActorTransform(),
				SpawnInfo);
			//不同武器的插槽不一样
			FName WeaponSocket=TEXT("WeaponSocket");
			if(ActiveWeapon==EWeaponType::M4A1)
			{
				WeaponSocket=TEXT("M4A1_Socket");
			}
			if(ActiveWeapon==EWeaponType::Sniper)
			{
				WeaponSocket=TEXT("AWP_Socket");
			}
			ClientPrimaryWeapon->K2_AttachToComponent(FPArmMesh,WeaponSocket,EAttachmentRule::SnapToTarget,
				EAttachmentRule::SnapToTarget,EAttachmentRule::SnapToTarget,true);

			ClientUpdateAmmoUI(ServerPrimaryWeapon->ClipCurrentAmmo,ServerPrimaryWeapon->GunCurrentAmmo);//装备上武器就修改ui
			//手臂动画混合
			if(ClientPrimaryWeapon)
			{
				UpdateFPArmsBlendPose(ClientPrimaryWeapon->FPArmsBlendPose);
			}
		}
	}
}

void AFPSBaseCharacter::ClientFire_Implementation()
{

	AWeaponBaseClient* CurrentClientFPArmsWeaponActor = GetCurrentClientFPArmsWeaponActor();
	if(CurrentClientFPArmsWeaponActor)
	{
		//枪体播放动画
		CurrentClientFPArmsWeaponActor->PlayShootAnimation();
		
		//手臂动画
		UAnimMontage* ClientArmsFireAnimMontage=CurrentClientFPArmsWeaponActor->ClientArmsFireAnimMontage;
		ClientArmsAnimBP->Montage_SetPlayRate(ClientArmsFireAnimMontage,1);
		ClientArmsAnimBP->Montage_Play(ClientArmsFireAnimMontage);

		//播放射击声音和动画
		CurrentClientFPArmsWeaponActor->PlayShootAnimation();
		CurrentClientFPArmsWeaponActor->DisplayWeaponEffect();
		
		//镜头抖动
		AMultiFPSPlayerController* MultiFPSPlayerController = Cast<AMultiFPSPlayerController>(GetController());
		if(MultiFPSPlayerController)
		{
			MultiFPSPlayerController->ClientPlayCameraShake(CurrentClientFPArmsWeaponActor->CameraShakeClass);

			//准星扩散动画
			MultiFPSPlayerController->DoCrosshairRecoil();
		}
		
	}
	
}
#pragma  endregion

#pragma region Fire

void AFPSBaseCharacter::AutomaticFire()
{
	if (ServerPrimaryWeapon->ClipCurrentAmmo>0)
	{
		//服务端(枪口闪光，声音，减少弹药，传建子弹UI，射线检测(步枪，手枪，狙击枪)，伤害，弹孔生成)
		if(UKismetMathLibrary::VSize(GetVelocity())>0.1f)
		{
			//移动时射击
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		}
		else
		{
			//静止时射击
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);
		}
	
		//客户端（枪体播放动画，手播放动画，声音，屏幕抖动，后座力，枪口粒子特效）
		//客户端(十字瞄准ui，初始化ui，瞄准十字线扩散)
		ClientFire();

		ClientRecoil();
	}
	else
	{
		StopFireWeaponPrimary();
	}
}

void AFPSBaseCharacter::ResetRecoil()
{
	 NewVerticalRecoilAmount=0;//上一帧后座力数值
	 OldVerticalRecoilAmount=0;//本次后座力数值
	 VerticalRecoilAmount=0;//后座力数值
	 NewHorizontalRecoilAmount=0;//上一帧水平后座力数值
	 OldHorizontalRecoilAmount=0;//本次水平后座力数值
	 HorizontalRecoilAmount=0;//水平后座力数值
	 RecoilXCoordPerShoot=0;//每次射击在曲线图中的x坐标
}

void AFPSBaseCharacter::FireWeaponPrimary()
{
	//判断子弹是否足够且没有在换弹
	if (ServerPrimaryWeapon->ClipCurrentAmmo>0 && !IsReloading)
	{
		if(UKismetMathLibrary::VSize(GetVelocity())>0.1f)
		{
			//移动时射击
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		}
		else
		{
			//静止时射击
			ServerFireRifleWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);
		}
		//服务端(枪口闪光，声音，减少弹药，传建子弹UI，射线检测(步枪，手枪，狙击枪)，伤害，弹孔生成)
		
	
		//客户端（枪体播放动画，手播放动画，声音，屏幕抖动，后座力，枪口粒子特效）
		//客户端(十字瞄准ui，初始化ui，瞄准十字线扩散)
		ClientFire();

		ClientRecoil();
		
		if(ServerPrimaryWeapon->IsAutomatic)
		{
			//开启计时器，隔固定时间重新射击
			GetWorldTimerManager().SetTimer(AutomaticFireTimerHandle,this,&AFPSBaseCharacter::AutomaticFire,0.2,true);
		}
		//连击系统开发
	}
	
}

void AFPSBaseCharacter::StopFireWeaponPrimary()
{
	//更改IsFiring变量，在服务器做
	ServerStopFiring();
	
	//关闭计时器
	GetWorldTimerManager().ClearTimer(AutomaticFireTimerHandle);

	//重置后座力相关变量
	ResetRecoil();
}

void AFPSBaseCharacter::RifleLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	if (ServerPrimaryWeapon)
	{
		//是否移动会导致不同的检测计算
		if(IsMoving)
		{
			//X,Y,Z加上随机的偏移量
			FVector Vector=CameraLocation+CameraForwardVector*ServerPrimaryWeapon->BulletDistance;
			float RandomX=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation=CameraLocation+FVector(Vector.X+RandomX,Vector.Y+RandomY,Vector.Z+RandomZ);
		}
		else
		{
		
			EndLocation=CameraLocation+CameraForwardVector*ServerPrimaryWeapon->BulletDistance;
		}
	}
	bool HitSuccess=UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);
	if(HitSuccess)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hitactorname %s"),*HitResult.GetActor()->GetName()));
		
		AFPSBaseCharacter* FPSBaseCharacter=Cast<AFPSBaseCharacter>(HitResult.GetActor());
		if(FPSBaseCharacter)
		{
			//打到玩家应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);//打到谁，从哪打，结果信息
		}
		else
		{
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);//保存法线的前向向量
			//打到别的生成弹孔
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
		
	}
}

void AFPSBaseCharacter::FireWeaponSecondary()
{
	//判断子弹是否足够且没有在换弹
	if (ServerSecondaryWeapon->ClipCurrentAmmo>0 && !IsReloading)
	{
		if(UKismetMathLibrary::VSize(GetVelocity())>0.1f)
		{
			//移动时射击
			ServerFirePistolWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		}
		else
		{
			//静止时射击
			ServerFirePistolWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);
		}
		//服务端(枪口闪光，声音，减少弹药，传建子弹UI，射线检测(步枪，手枪，狙击枪)，伤害，弹孔生成)
		
	
		//客户端（枪体播放动画，手播放动画，声音，屏幕抖动，后座力，枪口粒子特效）
		//客户端(十字瞄准ui，初始化ui，瞄准十字线扩散)
		ClientFire();
	}
}

void AFPSBaseCharacter::StopFireWeaponSecondary()
{
	//更改IsFiring变量，在服务器做
	ServerStopFiring();
}

void AFPSBaseCharacter::PistolLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	if (ServerSecondaryWeapon)
	{
		FVector CameraForwardVector;
		//是否移动会导致不同的检测计算
		if(IsMoving)
		{
			//旋转加一个随机偏移，根据射击的快慢决定,连续越快，偏移越大
			FRotator Rotator;
			Rotator.Roll=CameraRotation.Roll;
			Rotator.Pitch=CameraRotation.Pitch+UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			Rotator.Yaw=CameraRotation.Yaw+UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			CameraForwardVector=UKismetMathLibrary::GetForwardVector(Rotator);
			//X,Y,Z加上随机的偏移量(跑打的偏移)
			FVector Vector=CameraLocation+CameraForwardVector*ServerSecondaryWeapon->BulletDistance;
			float RandomX=UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange,ServerSecondaryWeapon->MovingFireRandomRange);
			float RandomY=UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange,ServerSecondaryWeapon->MovingFireRandomRange);
			float RandomZ=UKismetMathLibrary::RandomFloatInRange(-ServerSecondaryWeapon->MovingFireRandomRange,ServerSecondaryWeapon->MovingFireRandomRange);
			EndLocation=CameraLocation+FVector(Vector.X+RandomX,Vector.Y+RandomY,Vector.Z+RandomZ);
		}
		else
		{
			//旋转加一个随机偏移，根据射击的快慢决定,连续越快，偏移越大
			FRotator Rotator;
			Rotator.Roll=CameraRotation.Roll;
			Rotator.Pitch=CameraRotation.Pitch+UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			Rotator.Yaw=CameraRotation.Yaw+UKismetMathLibrary::RandomFloatInRange(PistolSpreadMin,PistolSpreadMax);
			CameraForwardVector=UKismetMathLibrary::GetForwardVector(Rotator);
			EndLocation=CameraLocation+CameraForwardVector*ServerSecondaryWeapon->BulletDistance;
		}
		//连续射击每次手枪散射范围变大
		PistolSpreadMax+=ServerSecondaryWeapon->SpreadWeaponMaxIndex;
		PistolSpreadMin-=ServerSecondaryWeapon->SpreadWeaponMinIndex;
	}
	
	bool HitSuccess=UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);
	if(HitSuccess)
	{
		UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hitactorname %s"),*HitResult.GetActor()->GetName()));
		
		AFPSBaseCharacter* FPSBaseCharacter=Cast<AFPSBaseCharacter>(HitResult.GetActor());
		if(FPSBaseCharacter)
		{
			//打到玩家应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);//打到谁，从哪打，结果信息
		}
		else
		{
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);//保存法线的前向向量
			//打到别的生成弹孔
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
		
	}
}

void AFPSBaseCharacter::DelaySpreadWeaponShootCallBack()
{
	//清空散射范围
	PistolSpreadMax=0;
	PistolSpreadMin=0;
}

void AFPSBaseCharacter::FireWeaponSniper()
{
	//判断子弹是否足够且没有在换弹且没有在射击
	if (ServerPrimaryWeapon->ClipCurrentAmmo>0 && !IsReloading &&!IsFiring)
	{
		if(UKismetMathLibrary::VSize(GetVelocity())>0.1f)
		{
			//移动时射击
			ServerFireSniperWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),true);
		}
		else
		{
			//静止时射击
			ServerFireSniperWeapon(PlayerCamera->GetComponentLocation(),PlayerCamera->GetComponentRotation(),false);
		}
		//服务端(枪口闪光，声音，减少弹药，传建子弹UI，射线检测(步枪，手枪，狙击枪)，伤害，弹孔生成)
		
		//客户端（枪体播放动画，手播放动画，声音，屏幕抖动，后座力，枪口粒子特效）
		//客户端(十字瞄准ui，初始化ui，瞄准十字线扩散)
		ClientFire();
		
	}
}

void AFPSBaseCharacter::StopFireWeaponSniper()
{
	
}

void AFPSBaseCharacter::SniperLineTrace(FVector CameraLocation, FRotator CameraRotation, bool IsMoving)
{
	FVector EndLocation;
	FVector CameraForwardVector = UKismetMathLibrary::GetForwardVector(CameraRotation);
	TArray<AActor*> IgnoreArray;
	IgnoreArray.Add(this);
	FHitResult HitResult;
	if (ServerPrimaryWeapon)
	{
		//是否瞄准也会影响
		if (IsAiming)
		{
			//是否移动会导致不同的检测计算
			if(IsMoving)
			{
				//X,Y,Z加上随机的偏移量
				FVector Vector=CameraLocation+CameraForwardVector*ServerPrimaryWeapon->BulletDistance;
				float RandomX=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				float RandomY=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				float RandomZ=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
				EndLocation=CameraLocation+FVector(Vector.X+RandomX,Vector.Y+RandomY,Vector.Z+RandomZ);
			}
			else
			{
		
				EndLocation=CameraLocation+CameraForwardVector*ServerPrimaryWeapon->BulletDistance;
			}
			ClientEndAiming();
		}
		else
		{
			//X,Y,Z加上随机的偏移量
			FVector Vector=CameraLocation+CameraForwardVector*ServerPrimaryWeapon->BulletDistance;
			float RandomX=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomY=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			float RandomZ=UKismetMathLibrary::RandomFloatInRange(-ServerPrimaryWeapon->MovingFireRandomRange,ServerPrimaryWeapon->MovingFireRandomRange);
			EndLocation=CameraLocation+FVector(Vector.X+RandomX,Vector.Y+RandomY,Vector.Z+RandomZ);
		}
	}
	bool HitSuccess=UKismetSystemLibrary::LineTraceSingle(GetWorld(),CameraLocation,EndLocation,ETraceTypeQuery::TraceTypeQuery1,false,IgnoreArray,EDrawDebugTrace::None,HitResult,true,FLinearColor::Red,FLinearColor::Green,3.f);
	if(HitSuccess)
	{
		//UKismetSystemLibrary::PrintString(GetWorld(),FString::Printf(TEXT("Hitactorname %s"),*HitResult.GetActor()->GetName()));
		
		AFPSBaseCharacter* FPSBaseCharacter=Cast<AFPSBaseCharacter>(HitResult.GetActor());
		if(FPSBaseCharacter)
		{
			//打到玩家应用伤害
			DamagePlayer(HitResult.PhysMaterial.Get(),HitResult.GetActor(),CameraLocation,HitResult);//打到谁，从哪打，结果信息
		}
		else
		{
			FRotator XRotator = UKismetMathLibrary::MakeRotFromX(HitResult.Normal);//保存法线的前向向量
			//打到别的生成弹孔
			MultiSpawnBulletDecal(HitResult.Location,XRotator);
		}
		
	}
}

void AFPSBaseCharacter::DelaySniperShootCallBack()
{
	IsFiring=false;
}

void AFPSBaseCharacter::DelayPlayArmReloadCallBack()
{
	AWeaponBaseServer* CurrentServerFPArmsWeaponActor = GetCurrentServerFPArmsWeaponActor();
	if(CurrentServerFPArmsWeaponActor)
	{
		int32 GunCurrentAmmo = CurrentServerFPArmsWeaponActor->GunCurrentAmmo;
		int32 ClipCurrentAmmo =CurrentServerFPArmsWeaponActor->ClipCurrentAmmo;
		int32 const ClipMaxAmmo = CurrentServerFPArmsWeaponActor->ClipMaxAmmo;

		IsReloading=false;
		//是否装填全部枪体子弹
		if (ClipMaxAmmo-ClipCurrentAmmo>=GunCurrentAmmo)
		{
			ClipCurrentAmmo+=GunCurrentAmmo;
			GunCurrentAmmo=0;
		}
		else{
			GunCurrentAmmo-=ClipMaxAmmo-ClipCurrentAmmo;
			ClipCurrentAmmo=ClipMaxAmmo;
		}
		CurrentServerFPArmsWeaponActor->GunCurrentAmmo= GunCurrentAmmo;
		CurrentServerFPArmsWeaponActor->ClipCurrentAmmo=ClipCurrentAmmo;
	
		ClientUpdateAmmoUI(ClipCurrentAmmo,GunCurrentAmmo);
	}
	
}

void AFPSBaseCharacter::DamagePlayer(UPhysicalMaterial* PhysicalMaterial,AActor* DamagedActor,FVector HitFromDirection,FHitResult& HitInfo)
{
	//5个位置受到不同的伤害
	AWeaponBaseServer* CurrentServerFPArmsWeaponActor = GetCurrentServerFPArmsWeaponActor();
	if (CurrentServerFPArmsWeaponActor)
	{
		switch(PhysicalMaterial->SurfaceType)
		{
		case EPhysicalSurface::SurfaceType1:
			{
				//head
				//打了别人调用这个函数
				UGameplayStatics::ApplyPointDamage(DamagedActor,CurrentServerFPArmsWeaponActor->BaseDamage * 4,HitFromDirection,HitInfo,
			GetController(),this,UDamageType::StaticClass());
				break;
			}
		case EPhysicalSurface::SurfaceType2:
			{
				//body
				UGameplayStatics::ApplyPointDamage(DamagedActor,CurrentServerFPArmsWeaponActor->BaseDamage * 1,HitFromDirection,HitInfo,
		GetController(),this,UDamageType::StaticClass());
				break;
			}
		case EPhysicalSurface::SurfaceType3:
			{
				//arm
				UGameplayStatics::ApplyPointDamage(DamagedActor,CurrentServerFPArmsWeaponActor->BaseDamage * 0.8,HitFromDirection,HitInfo,
GetController(),this,UDamageType::StaticClass());
				break;
			}
		case EPhysicalSurface::SurfaceType4:
			{
				//leg
				UGameplayStatics::ApplyPointDamage(DamagedActor,CurrentServerFPArmsWeaponActor->BaseDamage * 0.7,HitFromDirection,HitInfo,
GetController(),this,UDamageType::StaticClass());
				break;
			}
		}
	}
	
}

void AFPSBaseCharacter::OnHit(AActor* DamagedActor, float Damage, AController* InstigatedBy,
	FVector HitLocation, UPrimitiveComponent* FHitComponent, FName BoneName, FVector ShotFromDirection,
	const UDamageType* DamageType, AActor* DamageCauser)
{
	Health-=Damage;
	ClientUpdateHealthUI(Health);
	if(Health<=0)
	{
		//死亡逻辑
		DeathMatchDeath(DamageCauser);
	}
}


#pragma endregion 

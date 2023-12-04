// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBaseServer.h"

#include "FPSBaseCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeaponBaseServer::AWeaponBaseServer()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WeaponMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent=WeaponMesh;
	
	SphereCollision=CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollision"));
	SphereCollision->SetupAttachment(RootComponent);

	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionObjectType(ECC_WorldStatic);

	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereCollision->SetCollisionObjectType(ECC_WorldDynamic);

	WeaponMesh->SetOwnerNoSee(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetSimulatePhysics(true);

	SphereCollision->OnComponentBeginOverlap.AddDynamic(this,&AWeaponBaseServer::OnOtherBeginOverlap);

	bReplicates=true;
}

void AWeaponBaseServer::OnOtherBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFPSBaseCharacter* FPSCharacter=Cast<AFPSBaseCharacter>(OtherActor);
	if(FPSCharacter)
	{
		//玩家逻辑
		EquipWeapon();
		//如果是手枪装备副武器，步枪装备主武器
		if(KindOfWeapon==EWeaponType::DesertEagle)
		{
			FPSCharacter->EquipSecondary(this);
		}
		else
		{
			FPSCharacter->EquipPrimary(this);
		}
		
	}
}

//被拾取时修改武器碰撞相关属性
void AWeaponBaseServer::EquipWeapon()
{
	WeaponMesh->SetEnableGravity(false);
	WeaponMesh->SetSimulatePhysics(false);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void AWeaponBaseServer::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeaponBaseServer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeaponBaseServer::MultiShottingEffect_Implementation()
{
	if(GetOwner()!=UGameplayStatics::GetPlayerPawn(GetWorld(),0))
	{
		FName MuzzleFlashSocketName=TEXT("Fire_FX_Slot");
		//如果调用多播的客户端等于当前客户端就不执行闪光效果
		if (KindOfWeapon==EWeaponType::M4A1)
		{
			MuzzleFlashSocketName=TEXT("MuzzleSocket");
		}
		UGameplayStatics::SpawnEmitterAttached(MuzzleFlash,WeaponMesh,MuzzleFlashSocketName,FVector::Zero(),
		FRotator::ZeroRotator,FVector::OneVector,
		EAttachLocation::KeepRelativeOffset,true,
		EPSCPoolMethod::None,true
		);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(),FireSound,GetActorLocation());
	}
}

bool AWeaponBaseServer::MultiShottingEffect_Validate()
{
	return true;
}

void AWeaponBaseServer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AWeaponBaseServer,ClipCurrentAmmo,COND_None);//同步添加，类，属性，同步条件
}


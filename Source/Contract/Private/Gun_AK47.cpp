// Fill out your copyright notice in the Description page of Project Settings.  

#include "Gun_AK47.h"  

#include "NiagaraFunctionLibrary.h"  
#include "NiagaraComponent.h"  

#include "Components/ArrowComponent.h"  
#include "GameFramework/PlayerController.h"  

#include "Bullet.h"  

// Sets default values  
AGun_AK47::AGun_AK47() : isFire(false)  
{  
   // Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.  
   PrimaryActorTick.bCanEverTick = true;  

   muzzle = CreateDefaultSubobject<UArrowComponent>(TEXT("Muzzle"));  
   gunMuzzleFireNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("GunFireNiagara"));

   muzzle->SetupAttachment(GetMesh(), TEXT("Muzzle"));
   gunMuzzleFireNiagara->SetupAttachment(GetMesh(), TEXT("Muzzle"));
}  

// Called when the game starts or when spawned  
void AGun_AK47::BeginPlay()  
{  
   Super::BeginPlay();  

   player = GetWorld()->GetFirstPlayerController()->GetPawn();  
   playerController = GetWorld()->GetFirstPlayerController();  

   if (player == nullptr)  
   {  
       GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Player does not exist."));  
       return;  
   }  

   if (playerController == nullptr)  
   {  
       GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayerController does not exist."));  
       return;  
   }  

   UInputComponent* inputComponent = playerController->InputComponent;  

   if (inputComponent == nullptr)  
   {  
       GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("InputComponent does not exist."));  
       return;  
   }  

   if (bulletBlueprint == nullptr)  
   {  
       GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("BulletBlueprint does not exist."));  
       return;  
   } 
   // Niagara ����Ʈ ���� Ȯ��
   if (gunMuzzleFireNiagara == nullptr)
   {
       GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Gun Fire Niagara does not exist."));
       return;
   }

   // ���� ���� Ȯ��
   if (GetMesh() == nullptr || !GetMesh()->DoesSocketExist(FName(TEXT("GunFire"))))
   {
       GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("GunFire socket is not exist!!"));
       return;
   }

   muzzle->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(TEXT("GunFire"))); 
   gunMuzzleFireNiagara->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName(TEXT("GunFire")));
   muzzle->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));

   EnableInput(playerController);  

   // �Է� ���ε� ����  
   inputComponent->BindAction(TEXT("GunFire"), IE_Pressed, this, &AGun_AK47::StartFire);  
   inputComponent->BindAction(TEXT("GunFire"), IE_Released, this, &AGun_AK47::StopFire);  
}  

void AGun_AK47::Fire()  
{  
   FVector muzzleLocation = muzzle->GetComponentLocation();  
   FRotator muzzleRotation = muzzle->GetComponentRotation();  
   FRotator gunFireRotation = gunMuzzleFireNiagara->GetComponentRotation();

   UNiagaraFunctionLibrary::SpawnSystemAtLocation
   (  
       GetWorld(),  
       gunMuzzleFireNiagara->GetAsset(),
       muzzleLocation,  
       gunFireRotation
   );  

   FActorSpawnParameters params;  

   params.Owner = this;  
   params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;  

   GetWorld()->SpawnActor<ABullet>(bulletBlueprint, muzzleLocation, muzzleRotation, params);  
}  

void AGun_AK47::StartFire()  
{  
   isFire = true;  

   // ó�� �� �� ��� �߻�  
   Fire();  

   // ���� FireRate �������� Fire() �Լ��� �ݺ� ȣ�� (FireRate�� �߻� ����)  
   GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoFire, this, &AGun_AK47::Fire, fireRate, true);  
}  

void AGun_AK47::StopFire()  
{  
   isFire = false;  

   // Ÿ�̸� �ڵ��� �̿��Ͽ� �ݺ� ȣ�� ����  
   GetWorld()->GetTimerManager().ClearTimer(TimerHandle_AutoFire);  
}  

// Called every frame  
void AGun_AK47::Tick(float DeltaTime)  
{  
   Super::Tick(DeltaTime);  
}  

// Called to bind functionality to input  
void AGun_AK47::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)  
{  
   Super::SetupPlayerInputComponent(PlayerInputComponent);  
}

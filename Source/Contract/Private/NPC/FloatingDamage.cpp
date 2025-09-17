#include "FloatingDamage.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"

// Sets default values
AFloatingDamage::AFloatingDamage()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = mesh;

	damageWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	damageWidget->SetupAttachment(RootComponent);

	damage = 0;
}

void AFloatingDamage::SetDamageValue(int32 NewDamage)
{
	damage = NewDamage;
}

// Called when the game starts or when spawned
void AFloatingDamage::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AFloatingDamage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
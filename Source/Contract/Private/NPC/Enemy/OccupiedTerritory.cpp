#include "OccupiedTerritory.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AOccupiedTerritory::AOccupiedTerritory()
{
    PrimaryActorTick.bCanEverTick = false;

    // 정육면체 메시 컴포넌트 생성
    cubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
    RootComponent = cubeMesh;

    // 기본 정육면체 메시 설정
    static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
    if (cubeMeshAsset.Succeeded())
        cubeMesh->SetStaticMesh(cubeMeshAsset.Object);

    // 트리거 충돌 설정
    cubeMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    cubeMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    cubeMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    cubeMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
    cubeMesh->SetGenerateOverlapEvents(true);
    
    // 물리 시뮬레이션 비활성화
    cubeMesh->SetSimulatePhysics(false);
    cubeMesh->SetEnableGravity(false);
    cubeMesh->SetMobility(EComponentMobility::Static);

    // 기본 설정
    territoryOwner = ETerritoryOwner::Neutral;
    opacity = 0.3f;
}

void AOccupiedTerritory::BeginPlay()
{
    Super::BeginPlay();
    UpdateColorByOwner();
}

void AOccupiedTerritory::ApplyColorToMesh()
{
    if (!cubeMesh) return;

    UMaterialInterface* currentMaterial = cubeMesh->GetMaterial(0);
    if (currentMaterial && currentMaterial->GetName().Contains(TEXT("AreaColor")))
    {
        FLinearColor territoryColor;
        switch (territoryOwner)
        {
        case ETerritoryOwner::Neutral:
            territoryColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
            break;
        case ETerritoryOwner::Friendly:
            territoryColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f); // 파란색
            break;
        case ETerritoryOwner::Enemy:
            territoryColor = FLinearColor(1.0f, 0.2f, 0.2f, 1.0f); // 빨간색
            break;
        }

        FVector colorVector(territoryColor.R, territoryColor.G, territoryColor.B);
        cubeMesh->SetVectorParameterValueOnMaterials(FName("BaseColor"), colorVector);
        cubeMesh->SetScalarParameterValueOnMaterials(FName("Opacity"), opacity);
    }
}

#if WITH_EDITOR
void AOccupiedTerritory::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
    Super::PostEditChangeProperty(propertyChangedEvent);

    if (propertyChangedEvent.Property)
    {
        FString propertyName = propertyChangedEvent.Property->GetName();
        if (propertyName == TEXT("territoryOwner"))
        {
            UpdateColorByOwner();
        }
    }
}
#endif

void AOccupiedTerritory::SetOpacity(float newOpacity)
{
    opacity = FMath::Clamp(newOpacity, 0.0f, 1.0f);
    ApplyColorToMesh();
}

void AOccupiedTerritory::SetTerritoryOwner(ETerritoryOwner newOwner)
{
    territoryOwner = newOwner;
    UpdateColorByOwner();
}

void AOccupiedTerritory::UpdateColorByOwner()
{
    ApplyColorToMesh();
}
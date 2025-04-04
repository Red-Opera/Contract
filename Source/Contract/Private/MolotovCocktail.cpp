#include "MolotovCocktail.h"
#include "PlayerInventory.h"

void AMolotovCocktail::BeginPlay()
{
	Super::BeginPlay();

	playerInputComponent->BindAction(TEXT("Interaction"), IE_Pressed, this, &AMolotovCocktail::AddMolotovCocktail);
}

void AMolotovCocktail::AddMolotovCocktail()
{
	if (!CheckPlayerIsClose())
		return;

	// 플레이어 인벤토리에 수류탄 추가
	playerInventory->items.Add(AMolotovCocktail::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a Molotov Cocktail!"));

	Destroy();
}

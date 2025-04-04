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

	// �÷��̾� �κ��丮�� ����ź �߰�
	playerInventory->items.Add(AMolotovCocktail::StaticClass());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Got a Molotov Cocktail!"));

	Destroy();
}

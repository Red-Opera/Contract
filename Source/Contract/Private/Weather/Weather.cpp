#include "Weather.h"
#include "Components/DirectionalLightComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AWeather::AWeather()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeather::BeginPlay()
{
	Super::BeginPlay();

	FindDirectionalLight();

	if (directionalLight)
	{
		AActor* lightOwner = directionalLight->GetOwner();
		initialRotation = baseRotation;
		
		FRotator newRotation = initialRotation;
		
		lightOwner->SetActorRotation(newRotation);
		directionalLight->SetIntensity(dayIntensity);
	}
}

void AWeather::FindDirectionalLight()
{
	UWorld* world = GetWorld();

	if (world == nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Error (Null Reference) : World가 없습니다."));

		return;
	}

	for (TActorIterator<AActor> actorIterator(world); actorIterator; ++actorIterator)
	{
		AActor* actor = *actorIterator;

		if (actor && actor->GetName() == TEXT("DirectionalLight_0"))
		{
			UDirectionalLightComponent* lightComponent = actor->FindComponentByClass<UDirectionalLightComponent>();
			if (lightComponent)
			{
				directionalLight = lightComponent;
				break;
			}
		}
	}
}

void AWeather::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (directionalLight)
	{
		currentTime += deltaTime;
		
		// 새로운 사이클이 시작될 때 비 확률 체크
		if (currentTime >= dayCycleTime)
		{
			currentTime = 0.0f;
			
			// 확률적으로 비 결정
			float randomValue = FMath::RandRange(0.0f, 1.0f);

			if (randomValue <= rainProbability)
				StartRain();

			else
				StopRain();
		}

		float normalizedTime = currentTime / dayCycleTime;

		// 회전 조절 - Roll(X)=0, Yaw(Z)=90 고정, Pitch(Y)만 회전
		AActor* lightOwner = directionalLight->GetOwner();
		FRotator newRotation;
		newRotation.Roll = 45.0f;  // X(Roll) 고정
		newRotation.Pitch = initialRotation.Pitch + (normalizedTime * 360.0f);  // Y(Pitch) 회전
		newRotation.Yaw = 45.0f;  // Z(Yaw) 고정
		lightOwner->SetActorRotation(newRotation);

		// 밝기 조절 - 50%에서 증가 시작, 75%에서 최대값, 100%에서 nightIntensity
		float brightness;

		// 0% -> 50%: nightIntensity 유지
		if (normalizedTime < 0.5f)
			brightness = nightIntensity;

		// 50% -> 75%: night -> day 증가
		else if (normalizedTime <= 0.75f)
		{
			float progress = (normalizedTime - 0.5f) / 0.25f;
			brightness = FMath::Lerp(nightIntensity, dayIntensity, progress);
		}

		// 75% -> 100%: day -> night 감소
		else
		{
			float progress = (normalizedTime - 0.75f) / 0.25f;
			brightness = FMath::Lerp(dayIntensity, nightIntensity, progress);
		}

		directionalLight->SetIntensity(brightness);
	}
}

void AWeather::StartRain()
{
	isRaining = true;
	// 비 효과 시작 로직 추가 가능
}

void AWeather::StopRain()
{
	isRaining = false;
	// 비 효과 중지 로직 추가 가능
}

void AWeather::SetRainEnabled(bool isEnabled)
{
	this->isRaining = isEnabled;
}

bool AWeather::IsRaining() const
{
	return isRaining;
}
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/DirectionalLightComponent.h"
#include "Weather.generated.h"

UCLASS()
class CONTRACT_API AWeather : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeather();

protected:
	virtual void BeginPlay() override;

	// 씬의 DirectionalLight 참조
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	class UDirectionalLightComponent* directionalLight;

	// DirectionalLight를 찾는 함수
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void FindDirectionalLight();

	// 빛 강도 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day/Night")
	float dayIntensity = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day/Night")
	float nightIntensity = 2.0f;

	// 사이클 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day/Night")
	float dayCycleTime = 10.0f;

	// 초기 회전값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Day/Night")
	FRotator baseRotation = FRotator(0.0f, 0.0f, 90.0f);

	// 비 효과 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain")
	bool isRaining = false;

	// 비 확률 설정 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float rainProbability = 0.3f;

	// 비 효과 제어 함수들
	UFUNCTION(BlueprintCallable, Category = "Rain")
	void StartRain();

	UFUNCTION(BlueprintCallable, Category = "Rain")
	void StopRain();

	UFUNCTION(BlueprintCallable, Category = "Rain")
	void SetRainEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Rain")
	bool IsRaining() const;

private:
	float currentTime = 0.0f;
	FRotator initialRotation;

public:	
	virtual void Tick(float DeltaTime) override;
};

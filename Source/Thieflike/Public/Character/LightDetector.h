// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LightDetector.generated.h"

UCLASS()
class THIEFLIKE_API ALightDetector : public AActor
{
	GENERATED_BODY()

public:
	ALightDetector();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "LightDetection")
	float GetCurrentBrightness() const { return CachedBrightness; }

protected:
	// 내부 연산용 함수
	void CalculateBrightness();
	void ProcessRenderTexture(UTextureRenderTarget2D* TargetTexture, float& OutLocalMax);

	// 타이머 핸들
	FTimerHandle BrightnessTimerHandle;

	// 계산 빈도 (초 단위, 0.1초 = 10fps)
	const float CalculationInterval = 0.1f;

	// 최종 계산된 밝기 값 캐싱
	float CachedBrightness = 0.0f;

	// 픽셀 데이터 버퍼 (매번 생성하지 않도록 재사용)
	TArray<FColor> PixelBuffer;

public:
	UPROPERTY(EditAnywhere, Category = "LightDetection")
	UTextureRenderTarget2D* DetectorTextureTop;

	UPROPERTY(EditAnywhere, Category = "LightDetection")
	UTextureRenderTarget2D* DetectorTextureBottom;
};
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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
	// Internal calculation function
	void CalculateBrightness();
	void ProcessRenderTexture(UTextureRenderTarget2D* TargetTexture, float& OutLocalMax);

	// Timer handle
	FTimerHandle BrightnessTimerHandle;

	// Calculation frequency (in seconds, 0.1s = 10fps)
	const float CalculationInterval = 0.1f;

	// Final calculated brightness value (cached)
	float CachedBrightness = 0.0f;

	// Pixel data buffer (reused to avoid repeated allocations)
	TArray<FColor> PixelBuffer;

public:
	UPROPERTY(EditAnywhere, Category = "LightDetection")
	UTextureRenderTarget2D* DetectorTextureTop;

	UPROPERTY(EditAnywhere, Category = "LightDetection")
	UTextureRenderTarget2D* DetectorTextureBottom;
};
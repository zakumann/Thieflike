// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LightDetector.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

// Sets default values
ALightDetector::ALightDetector()
{
	PrimaryActorTick.bCanEverTick = false;

}

void ALightDetector::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(
		BrightnessTimerHandle,
		this,
		&ALightDetector::CalculateBrightness,
		CalculationInterval,
		true // Repeat
	);
}

void ALightDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ALightDetector::CalculateBrightness()
{
	if (!DetectorTextureTop || !DetectorTextureBottom)
	{
		CachedBrightness = 0.0f;
		return;
	}

	float TopBrightness = 0.0f;
	float BottomBrightness = 0.0f;

	ProcessRenderTexture(DetectorTextureTop, TopBrightness);
	ProcessRenderTexture(DetectorTextureBottom, BottomBrightness);

	CachedBrightness = FMath::Max(TopBrightness, BottomBrightness);
}

void ALightDetector::ProcessRenderTexture(UTextureRenderTarget2D* TargetTexture, float& OutLocalMax)
{
	if (!TargetTexture) return;

	FRenderTarget* RenderTarget = TargetTexture->GameThread_GetRenderTargetResource();
	if (!RenderTarget) return;

	// ReadPixels is still a blocking call, but we minimize its impact by reusing the PixelBuffer
	RenderTarget->ReadPixels(PixelBuffer);

	// Periodic cleanup to prevent unbounded growth
	if (PixelBuffer.Num() > 1024)
	{
		PixelBuffer.Empty();
		RenderTarget->ReadPixels(PixelBuffer);
	}

	float LocalMax = 0.0f;

	// Sample every 4th pixel for performance (25% sampling)
	// Change to "i++" for 100% sampling if needed
	for (int32 i = 0; i < PixelBuffer.Num(); i += 4)
	{
		const FColor& Pixel = PixelBuffer[i];

		// Luminance formula: Y = 0.299R + 0.587G + 0.114B
		// Result is in 0-255 range
		float PixelValue = (0.299f * Pixel.R) + (0.587f * Pixel.G) + (0.114f * Pixel.B);

		if (PixelValue > LocalMax)
		{
			LocalMax = PixelValue;
		}
	}

	OutLocalMax = LocalMax; // Keep original value (0-255)
}

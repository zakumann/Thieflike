// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "LightDetector.generated.h"

class UTextureRednerTarget2D;

UCLASS()
class THIEFLIKE_API ALightDetector : public AActor
{
	GENERATED_BODY()

	/*void ProcessRenderTexture(UTextureRenderTarget2D* Texture);

	TArray<FColor> pixelStorage;
	float pixelChannelR{ 0 };
	float pixelChannelG{ 0 };
	float pixelChannelB{ 0 };
	float brightnessOutput{ 0 };
	float currentPixelBrightness{ 0 };
	FRenderTarget* fRenderTarget;*/
	
public:	
	// Sets default values for this actor's properties
	ALightDetector();

	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "LightDetection")
	float CalculateBrightness();

private:
	
	// Helper to calculate brightness from a specific texture
	float SamplePixelsFromTarget(UTextureRenderTarget2D* InTexture);

	UPROPERTY(VisibleAnywhere, Category = "Ladder")
	class USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Ladder")
	class UStaticMeshComponent* LightGem;

	UPROPERTY(EditAnywhere, Category = "Ladder")
	class USpringArmComponent* SpringArm_Up;

	UPROPERTY(EditAnywhere, Category = "Ladder")
	class USceneCaptureComponent2D* CaptureComponent_Up;

	UPROPERTY(EditAnywhere, Category = "Ladder")
	class USpringArmComponent* SpringArm_Bottom;

	UPROPERTY(EditAnywhere, Category = "Ladder")
	class USceneCaptureComponent2D* CaptureComponent_Bottom;

	// The Render Textures we will be passing the CalculateBrightness() method
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stealth", meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* LightDetectionRenderTarget;

	TArray <FColor> PixelStorage;
};

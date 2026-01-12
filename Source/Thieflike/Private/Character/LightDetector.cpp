// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LightDetector.h"

// Sets default values
ALightDetector::ALightDetector()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DefaultSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultSceneRoot"));
	RootComponent = DefaultSceneRoot;

	LightGem = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LightGem"));
	LightGem->SetupAttachment(RootComponent);
	LightGem->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LightGem->SetCastShadow(false);

	SpringArm_Up = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm_Up"));
	SpringArm_Up->SetupAttachment(LightGem);
	SpringArm_Up->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	SpringArm_Up->TargetArmLength = 75.0f;
	SpringArm_Up->bDoCollisionTest = false;

	CaptureComponent_Up = CreateDefaultSubobject< USceneCaptureComponent2D>(TEXT("Up"));
	CaptureComponent_Up->SetupAttachment(SpringArm_Up);
	CaptureComponent_Up->FOVAngle = 60.0f;
	CaptureComponent_Up->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent_Up->PostProcessBlendWeight = 1.0f;
	CaptureComponent_Up->bCaptureEveryFrame = true;
	CaptureComponent_Up->bCaptureOnMovement = true;

	SpringArm_Bottom = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm_Bottom"));
	SpringArm_Bottom->SetupAttachment(LightGem);
	SpringArm_Bottom->SetRelativeRotation(FRotator(-90, 0.0f, 0.0f));
	SpringArm_Bottom->TargetArmLength = 75.0f;
	SpringArm_Bottom->bDoCollisionTest = false;

	CaptureComponent_Bottom = CreateDefaultSubobject< USceneCaptureComponent2D>(TEXT("Bottom"));
	CaptureComponent_Bottom->SetupAttachment(SpringArm_Bottom);
	CaptureComponent_Bottom->FOVAngle = 60.0f;
	CaptureComponent_Bottom->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent_Bottom->PostProcessBlendWeight = 1.0f;
	CaptureComponent_Bottom->bCaptureEveryFrame = true;
	CaptureComponent_Bottom->bCaptureOnMovement = true;

}

// Called when the game starts or when spawned
void ALightDetector::BeginPlay()
{
	Super::BeginPlay();

	if (LightDetectionRenderTarget)
	{
		CaptureComponent_Up->TextureTarget = LightDetectionRenderTarget;
	}
}

// Called every frame
void ALightDetector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


/*void ALightDetector::ProcessRenderTexture(UTextureRenderTarget2D* detectorTexture)
{
	// Read the pixels from our RenderTexture and store the data into our colour array
	// Note: ReadPixels is allegedly a very slow operation
	fRenderTarget = detectorTexture->GameThread_GetRenderTargetResource();
	fRenderTarget->ReadPixels(pixelStorage);

	// We iterate through every pixel we retrieved and find the brightest pixel
	for (int pixelNum = 0; pixelNum < pixelStorage.Num(); pixelNum++)
	{
		pixelChannelR = pixelStorage[pixelNum].R;
		pixelChannelG = pixelStorage[pixelNum].G;
		pixelChannelB = pixelStorage[pixelNum].B;

		// Use a formula to determine brightness based on pixel colour values. Source for Formula used:
		// www.stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
		currentPixelBrightness = ((0.299 * pixelChannelR) + (0.587 * pixelChannelG) + (0.114 * pixelChannelB));

		// If the current pixel we just processed is brighter than the previously brightest pixel, replace it with the new pixel
		if (currentPixelBrightness >= brightnessOutput)
		{
			brightnessOutput = currentPixelBrightness;
		}
	}
}*/


float ALightDetector::CalculateBrightness()
{
/*	// Ensure that the user has actually supplied us with RenderTextures
	if (detectorTextureTop == nullptr || detectorTextureBottom == nullptr)
	{
		return 0.0f;
	}

	// Reset our values for the next brightness test
	currentPixelBrightness = 0;
	brightnessOutput = 0;

	// Process our top and bottom RenderTextures
	ProcessRenderTexture(detectorTextureTop);
	ProcessRenderTexture(detectorTextureBottom);

	// At the end we return the brightest pixel we found in the RenderTextures
	return brightnessOutput;*/

	return SamplePixelsFromTarget(LightDetectionRenderTarget);
}

float ALightDetector::SamplePixelsFromTarget(UTextureRenderTarget2D* InTexture)
{
	if (!InTexture) return 0.0f;

	FRenderTarget* RenderTargetResource = InTexture->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource) return 0.0f;
	PixelStorage.Reset();
	RenderTargetResource->ReadPixels(PixelStorage);

	int32 Width = InTexture->SizeX;
	int32 Height = InTexture->SizeY;

	if (Width < 181 || Height < 181)
	{
		return 0.0f;
	}

	const int32 Coords[4][2] = {
		{45, 45},
		{90, 90},
		{135, 135},
		{180, 180}
	};

	float SumR = 0.0f;

	for (int i = 0; i < 4; i++)
	{
		int32 X = Coords[i][0];
		int32 Y = Coords[i][1];

		int32 Index = (Y * Width) + X;

		if (PixelStorage.IsValidIndex(Index))
		{
			float NormalizedR = PixelStorage[Index].R / 255.0f;
			SumR += NormalizedR;
		}
	}

	return SumR / 4.0f;
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/LightDetector.h"

// Sets default values
ALightDetector::ALightDetector()
{

}

void ALightDetector::BeginPlay()
{
}

void ALightDetector::Tick(float DeltaTime)
{
}

void ALightDetector::ProcessRenderTexture(UTextureRenderTarget2D* detectorTexture)
{
	// Read the pixels from our RenderTexture and store the data into our color array
	// Note: ReadPixels is allegedly a very slow operation
	fRenderTarget = detectorTexture->GameThread_GetRenderTargetResource();
	fRenderTarget->ReadPixels(pixelStorage);

	// We iterate through every pixel we retrieved and find the brightest pixel
	for (int pixelNum = 0; pixelNum < pixelStorage.Num(); pixelNum++) {
		pixelChannelR = pixelStorage[pixelNum].R;
		pixelChannelG = pixelStorage[pixelNum].G;
		pixelChannelB = pixelStorage[pixelNum].B;

		// Use a formula to determine brightness based on pixel color values. Source for Formula used:
		// www.stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
		currentPixelBrightness = ((0.299 * pixelChannelR) + (0.587 * pixelChannelG) + (0.114 * pixelChannelB));

		// If the current pixel we just processed is brighter than the previously brightest pixel, replace it with the new pixel
		if (currentPixelBrightness >= brightnessOutput) {
			brightnessOutput = currentPixelBrightness;
		}
	}
}

float ALightDetector::CalculateBrightness()
{
	// Ensure that the user has actually supplied us with RenderTextures
	if (detectorTextureTop == nullptr || detectorTextureBottom == nullptr) {
		return 0.0f;
	}
	// Reset our values for the next brightness test
	currentPixelBrightness = 0;
	brightnessOutput = 0;

	// Process our top and bottom RenderTextures
	ProcessRenderTexture(detectorTextureTop);
	ProcessRenderTexture(detectorTextureBottom);


	// At the end we return the brightest pixel we found in the RenderTextures
	return brightnessOutput;
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ladder.generated.h"

UCLASS()
class THIEFLIKE_API ALadder : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALadder();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	class USceneComponent* DefaultSceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	class UStaticMeshComponent* LadderBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder | Customize")
	class UStaticMesh* TopLadderMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ladder | Customize")
	class UStaticMesh* MidLadderMesh;

	UPROPERTY(EditAnywhere, Category = "Ladder | Customize",
		meta = (EditCondition="TopLadderMesh != nullptr && MidLadderMesh != nullptr", ClampMin="0", ClampMax="50"))
	int MidPartCount;

	UPROPERTY(EditAnywhere, Category = "Ladder | Customize", meta = (ClampMin="0", ClampMax="100"))
	float TopOffset = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	class UInstancedStaticMeshComponent* MiddleParts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ladder")
	class UBoxComponent* BoxCollision;

	/** Get the ladder's collision box for climbing bounds */
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	UBoxComponent* GetBoxCollision() const { return BoxCollision; }

	/** Get ladder's minimum Z positio (bottom) */
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	float GetLadderMinZ() const;

	/** Get ladder's maximum Z positino (top where Ladderbase ends) */
	UFUNCTION(BlueprintCallable, Category = "Ladder")
	float GetLadderMaxZ() const;

	virtual void OnConstruction(const FTransform& Transform) override;
};

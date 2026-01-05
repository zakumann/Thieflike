// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Crate.generated.h"

UCLASS()
class THIEFLIKE_API ACrate : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACrate();

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LidMesh;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnInteract();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crate Interaction")
	bool bIsOpen;

	// BlueprintNativeEvent: Can activate C++ Logic(_Implementation) and Override into BP
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnOpen();
	virtual void OnOpen_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void OnClose();
	virtual void OnClose_Implementation();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

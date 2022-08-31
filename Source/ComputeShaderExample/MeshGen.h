// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicMesh3.h"
#include "SimpleDynamicMeshComponent.h"
#include "MeshGen.generated.h"

UCLASS()
class COMPUTESHADEREXAMPLE_API AMeshGen : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeshGen();

	UPROPERTY(VisibleAnywhere)
	USimpleDynamicMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* UseMaterial = nullptr;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

//enum OctreeNodeType
//{
//	Node_None,
//	Node_Internal,
//	Node_Psuedo,
//	Node_Leaf,
//};
//
//class OctreeNode
//{
//public:
//
//	OctreeNode()
//		: type(Node_None)
//		, min(0, 0, 0)
//		, size(0)
//	{
//		for (int i = 0; i < 8; i++)
//		{
//			children[i] = nullptr;
//		}
//	}
//
//	OctreeNode(const OctreeNodeType _type)
//		: type(_type)
//		, min(0, 0, 0)
//		, size(0)
//	{
//		for (int i = 0; i < 8; i++)
//		{
//			children[i] = nullptr;
//		}
//	}
//
//	OctreeNodeType	type;
//	FVector			min;
//	int				size;
//	OctreeNode* children[8];
//};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicMesh3.h"
#include "SimpleDynamicMeshComponent.h"
#include "qef.h"
#include "MeshGenComponent.generated.h"


enum OctreeNodeType
{
	Node_None,
	Node_Internal,
	Node_Psuedo,
	Node_Leaf,
};
struct OctreeDrawInfo
{
	OctreeDrawInfo()
		: index(-1)
		, corners(0)
	{
	}

	int				index;
	int				corners;
	FVector			position;
	FVector			averageNormal;
	svd::QefData	qef;
	
};

class OctreeNode
{
public:

	OctreeNode()
		: type(Node_None)
		, min(0, 0, 0)
		, size(0)
		, drawInfo(nullptr)
	{
		for (int i = 0; i < 8; i++)
		{
			children[i] = nullptr;
		}
	}

	OctreeNode(const OctreeNodeType _type)
		: type(_type)
		, min(0, 0, 0)
		, size(0)
	{
		for (int i = 0; i < 8; i++)
		{
			children[i] = nullptr;
		}
	}

	OctreeNodeType	type;
	FVector			min;
	int				size;
	OctreeNode* children[8];
	OctreeDrawInfo* drawInfo;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTESHADEREXAMPLE_API UMeshGenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& event) override;
#endif
	// Sets default values for this component's properties
	UMeshGenComponent();
	UPROPERTY(VisibleAnywhere)
	USimpleDynamicMeshComponent* MeshComponent = nullptr;

	UPROPERTY(EditAnywhere)
	float simplifyThreshold = 1.0f;
	

	UPROPERTY(EditAnywhere)
	UMaterialInterface* UseMaterial = nullptr;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* UseMaterial1 = nullptr;

	OctreeNode* root;
	//FDynamicMesh3 * mymesh=nullptr;
	TArray<FVector> vertex;
	TArray<FVector> normal;
	TArray<int> index;
	TMap<int, int> v2i;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	OctreeNode* BuildOctree(TArray<FVector> &pos, TArray<FVector>& normals, TArray<int>& corners,int gsize);
	OctreeNode* BuildOctreeUpward(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners,int gsize);
	OctreeNode* BuildOctreeUpward1(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners,TArray<int>& trueind,TArray<CSQef>& qefs,int gsize,int validnum);
	void GenMeshIm();
		
};
void ContourCellProc(OctreeNode* node, TArray<int>& indexBuffer);



// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshGen.h"

#include "Generators/SphereGenerator.h"
#include "ComputeShaderTestComponent.h"
// Sets default values
AMeshGen::AMeshGen()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MeshComponent = CreateDefaultSubobject<USimpleDynamicMeshComponent>(TEXT("MeshComponent"), false);
	SetRootComponent(MeshComponent);
}

// Called when the game starts or when spawned
void AMeshGen::BeginPlay()
{
	Super::BeginPlay();
	FDynamicMesh3 mymesh = FDynamicMesh3();
	mymesh.AppendVertex(FVector3d(0.0, 0.0, 0.0));
	mymesh.AppendVertex(FVector3d(0.0, 0.0, 1.0));
	mymesh.AppendVertex(FVector3d(0.0, 1.0, 0.0));
	mymesh.AppendTriangle(FIndex3i(0, 1, 2));
	mymesh.EnableVertexColors(FVector3f::Zero());
	mymesh.SetVertexColor(0, FVector3f(1.0f, 0.0f, 0.0f));
	mymesh.SetVertexColor(1, FVector3f(0.0f, 1.0f, 0.0f));
	mymesh.SetVertexColor(2, FVector3f(0.0f, 0.0f, 1.0f));
	/*FSphereGenerator SphereGen;
	SphereGen.NumPhi = SphereGen.NumTheta = 8;
	SphereGen.Radius = 10.0;
	mymesh.Copy(&SphereGen.Generate());*/
	
	*(MeshComponent->GetMesh()) = mymesh;
	MeshComponent->NotifyMeshUpdated();
	UE_LOG(LogTemp, Warning, TEXT("Hello"));
	// update material on new section
	//UseMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	MeshComponent->SetMaterial(0, UseMaterial);
}

// Called every frame
void AMeshGen::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	//UComputeShaderTestComponent* boidsComponent = GetOwner()->FindComponentByClass<UComputeShaderTestComponent>();
	
}


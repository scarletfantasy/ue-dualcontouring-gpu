// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GlobalShader.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"

#include <atomic>
#include "qef.h"
#include "ComputeShaderTestComponent.generated.h"









class FDualContouringShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDualContouringShader, Global);

	FDualContouringShader() {}

	explicit FDualContouringShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);



public:

	LAYOUT_FIELD(FShaderResourceParameter, meshpoints);
	LAYOUT_FIELD(FShaderResourceParameter, meshnormals);
	LAYOUT_FIELD(FShaderResourceParameter, meshcorners);
	LAYOUT_FIELD(FShaderResourceParameter, debugint);
	LAYOUT_FIELD(FShaderParameter, gridsize);
	LAYOUT_FIELD(FShaderParameter, boxpos);
	LAYOUT_FIELD(FShaderParameter, boxscale);
	LAYOUT_FIELD(FShaderParameter, spherepos);
	LAYOUT_FIELD(FShaderParameter, spherescale);

	
};

class FCornerShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCornerShader, Global);

	FCornerShader() {}

	explicit FCornerShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);



public:



	LAYOUT_FIELD(FShaderResourceParameter,countutil);
	LAYOUT_FIELD(FShaderParameter, gridsize);
	LAYOUT_FIELD(FShaderParameter, boxpos);
	LAYOUT_FIELD(FShaderParameter, boxscale);
	LAYOUT_FIELD(FShaderParameter, spherepos);
	LAYOUT_FIELD(FShaderParameter, spherescale);


	
};

class FCornerIndShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCornerIndShader, Global);

	FCornerIndShader() {}

	explicit FCornerIndShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);



public:


	LAYOUT_FIELD(FShaderResourceParameter, meshcorners);
	LAYOUT_FIELD(FShaderResourceParameter, debugint);
	LAYOUT_FIELD(FShaderResourceParameter,countutil);
	LAYOUT_FIELD(FShaderParameter, gridsize);
	LAYOUT_FIELD(FShaderParameter, boxpos);
	LAYOUT_FIELD(FShaderParameter, boxscale);
	LAYOUT_FIELD(FShaderParameter, spherepos);
	LAYOUT_FIELD(FShaderParameter, spherescale);


	
};


class FQEFShader : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FQEFShader, Global);

	FQEFShader() {}

	explicit FQEFShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return GetMaxSupportedFeatureLevel(Parameters.Platform) >= ERHIFeatureLevel::SM5;
	};

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);



public:


	LAYOUT_FIELD(FShaderResourceParameter, meshpoints);
	LAYOUT_FIELD(FShaderResourceParameter, meshnormals);
	LAYOUT_FIELD(FShaderResourceParameter, meshcorners);
	LAYOUT_FIELD(FShaderResourceParameter, debugint);
	LAYOUT_FIELD(FShaderResourceParameter,meshqefs);
	LAYOUT_FIELD(FShaderParameter, gridsize);
	LAYOUT_FIELD(FShaderParameter, boxpos);
	LAYOUT_FIELD(FShaderParameter, boxscale);
	LAYOUT_FIELD(FShaderParameter, spherepos);
	LAYOUT_FIELD(FShaderParameter, spherescale);
	LAYOUT_FIELD(FShaderParameter, validnum);

	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class COMPUTESHADEREXAMPLE_API UComputeShaderTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UComputeShaderTestComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnRegister() override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& event) override;
#endif

	void RunComputeShader();
	void RunComputeShader1();


public:

	int gsize = 128;
	int validnum=0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector boxposition = FVector(10,10,10);
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float boxscalevalue = 1.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector sphereposition = FVector(16, 16, 16);
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float spherescalevalue = 1.0;


	TArray<FVector> outputPositions;
	TArray<FVector> meshPositions;
	TArray<FVector> meshNormals;
	TArray<int> meshCorners;
	TArray<int> trueInd;
	TArray<CSQef> meshQefs;
	bool isready;
	bool ischange;
	bool isfinish;
	bool isfirst;

	double lastRunTime = 0.0;
	FRenderCommandFence RenderCommandFence;

protected:
	// GPU side


	FStructuredBufferRHIRef _meshBuffer;
	FUnorderedAccessViewRHIRef _meshBufferUAV;

	FStructuredBufferRHIRef _cornerBuffer;
	FUnorderedAccessViewRHIRef _cornerBufferUAV;

	FStructuredBufferRHIRef _normalBuffer;
	FUnorderedAccessViewRHIRef _normalBufferUAV;

	FStructuredBufferRHIRef _debugintBuffer;
	FUnorderedAccessViewRHIRef _debugintBufferUAV;

	FStructuredBufferRHIRef _qefBuffer;
	FUnorderedAccessViewRHIRef _qefBufferUAV;

	FStructuredBufferRHIRef _countBuffer;
	FUnorderedAccessViewRHIRef _countBufferUAV;

	bool check;
};

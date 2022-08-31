// Fill out your copyright notice in the Description page of Project Settings.


#include "ComputeShaderTestComponent.h"
#include "MeshGenComponent.h"
#if ENGINE_MINOR_VERSION < 26

#include "ShaderParameterUtils.h"

#else

#include "ShaderCompilerCore.h"

#endif

#include "RHIStaticStates.h"
//#include "Android/AndroidInputInterface.h"

// Some useful links
// -----------------
// [Enqueue render commands using lambdas](https://github.com/EpicGames/UnrealEngine/commit/41f6b93892dcf626a5acc155f7d71c756a5624b0)
//



// Sets default values for this component's properties
UComputeShaderTestComponent::UComputeShaderTestComponent() 
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	check = true;
	ischange = false;
	isfinish=true;
	isfirst=true;
	// ...
	FRHICommandListImmediate& RHICommands = GRHICommandList.GetImmediateCommandList();

	FRandomStream rng;



	{
		TResourceArray<FVector> meshResourceArray;
		meshResourceArray.Init(FVector::ZeroVector, gsize * gsize * gsize);

		FRHIResourceCreateInfo createInfo;
		createInfo.ResourceArray = &meshResourceArray;

		_meshBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * gsize * gsize * gsize, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		
		_meshBufferUAV = RHICreateUnorderedAccessView(_meshBuffer, false, false);
	}


	{
		TResourceArray<FVector> normalResourceArray;
		normalResourceArray.Init(FVector::ZeroVector, gsize * gsize * gsize);

		FRHIResourceCreateInfo createInfo;
		createInfo.ResourceArray = &normalResourceArray;

		_normalBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * gsize * gsize * gsize, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		_normalBufferUAV = RHICreateUnorderedAccessView(_normalBuffer, false, false);
	}

	{
		TResourceArray<int> cornerResourceArray;
		cornerResourceArray.Init(0, gsize * gsize * gsize);

		FRHIResourceCreateInfo createInfo;
		createInfo.ResourceArray = &cornerResourceArray;

		_cornerBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * gsize * gsize * gsize, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		_cornerBufferUAV = RHICreateUnorderedAccessView(_cornerBuffer, false, false);
	}

	{
		TResourceArray<int> debugintResourceArray;
		debugintResourceArray.Init(0, gsize * gsize * gsize);

		FRHIResourceCreateInfo createInfo;
		createInfo.ResourceArray = &debugintResourceArray;

		_debugintBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * gsize * gsize * gsize, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		_debugintBufferUAV = RHICreateUnorderedAccessView(_debugintBuffer, false, false);
	}

	{
		TResourceArray<CSQef> qefResourceArray;
		CSQef tmp;
		qefResourceArray.Init(tmp,gsize * gsize * gsize);
		
		FRHIResourceCreateInfo createInfo;
		createInfo.ResourceArray = &qefResourceArray;

		_qefBuffer = RHICreateStructuredBuffer(sizeof(CSQef), sizeof(CSQef) * gsize * gsize * gsize, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
		_qefBufferUAV = RHICreateUnorderedAccessView(_qefBuffer, false, false);
	}

	



	if (meshPositions.Num() != gsize * gsize * gsize)
	{
		const FVector zero(0.0f);
		CSQef tmp;
		meshPositions.Init(zero, gsize * gsize * gsize);
		meshNormals.Init(zero, gsize * gsize * gsize);
		meshCorners.Init(0, gsize * gsize * gsize);
		meshQefs.Init(tmp, gsize * gsize * gsize);
		trueInd.Init(0, gsize * gsize * gsize);
	}

	//UMeshGenComponent* meshGenCompoennt = GetOwner()->FindComponentByClass<UMeshGenComponent>();
	
}

void UComputeShaderTestComponent::OnRegister()
{
	Super::OnRegister();
	// Limited to 30 fps
	/*if(FPlatformTime::Seconds() - lastRunTime > 1.0 / 30.0)
	{
		lastRunTime = FPlatformTime::Seconds();
		RunComputeShader();
	}*/
}
void UComputeShaderTestComponent::PostLoad()
{
	Super::PostLoad();
	RunComputeShader1();
}
// Called when the game starts
void UComputeShaderTestComponent::BeginPlay()
{
	Super::BeginPlay();
}
void UComputeShaderTestComponent::RunComputeShader()
{
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		if (!RenderCommandFence.IsFenceComplete()) return;
		ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
			[&](FRHICommandListImmediate& RHICommands)
			{
				const auto startSec = FPlatformTime::Seconds();
				//TShaderMapRef<FComputeShaderDeclaration1> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				//TShaderMapRef<FComputeShaderDeclaration1> cs1(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				TShaderMapRef<FDualContouringShader> dcs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));


				FRHIComputeShader* dccs = dcs.GetComputeShader();
				RHICommands.SetUAVParameter(dccs, dcs->meshpoints.GetBaseIndex(), _meshBufferUAV);
				RHICommands.SetUAVParameter(dccs, dcs->meshnormals.GetBaseIndex(), _normalBufferUAV);
				RHICommands.SetUAVParameter(dccs, dcs->meshcorners.GetBaseIndex(), _cornerBufferUAV);
				RHICommands.SetUAVParameter(dccs, dcs->debugint.GetBaseIndex(), _debugintBufferUAV);
				RHICommands.SetShaderParameter(dccs, dcs->gridsize.GetBufferIndex(), dcs->gridsize.GetBaseIndex(),
				                               4,
				                               &gsize);
				RHICommands.SetShaderParameter(dccs, dcs->boxpos.GetBufferIndex(), dcs->boxpos.GetBaseIndex(), 12,
				                               &boxposition);
				RHICommands.SetShaderParameter(dccs, dcs->boxscale.GetBufferIndex(), dcs->boxscale.GetBaseIndex(),
				                               4,
				                               &boxscalevalue);
				RHICommands.SetShaderParameter(dccs, dcs->spherepos.GetBufferIndex(), dcs->spherepos.GetBaseIndex(),
				                               12,
				                               &sphereposition);
				RHICommands.SetShaderParameter(dccs, dcs->spherescale.GetBufferIndex(),
				                               dcs->spherescale.GetBaseIndex(),
				                               12, &spherescalevalue);

				RHICommands.SetComputeShader(dccs);
				DispatchComputeShader(RHICommands, dcs, gsize / 4, gsize / 4, gsize / 4);


				uint8* meshdata = (uint8*)RHILockStructuredBuffer(_meshBuffer, 0,
				                                                  gsize * gsize * gsize * sizeof(FVector),
				                                                  RLM_ReadOnly);
				FMemory::Memcpy(meshPositions.GetData(), meshdata, gsize * gsize * gsize * sizeof(FVector));

				RHIUnlockStructuredBuffer(_meshBuffer);

				uint8* meshnormaldata = (uint8*)RHILockStructuredBuffer(
					_normalBuffer, 0, gsize * gsize * gsize * sizeof(FVector), RLM_ReadOnly);
				FMemory::Memcpy(meshNormals.GetData(), meshnormaldata, gsize * gsize * gsize * sizeof(FVector));

				RHIUnlockStructuredBuffer(_normalBuffer);

				uint8* meshcornerdata = (uint8*)RHILockStructuredBuffer(
					_cornerBuffer, 0, gsize * gsize * gsize * sizeof(int), RLM_ReadOnly);
				FMemory::Memcpy(meshCorners.GetData(), meshcornerdata, gsize * gsize * gsize * sizeof(int));

				RHIUnlockStructuredBuffer(_cornerBuffer);
				ischange = true;
				UMeshGenComponent* meshGenCompoennt = GetOwner()->FindComponentByClass<UMeshGenComponent>();

				const auto endSec = FPlatformTime::Seconds();
				UE_LOG(LogTemp, Warning, TEXT("compute shader time is %f"), (endSec - startSec) * 1000.0);
				meshGenCompoennt->GenMeshIm();
			});
		RenderCommandFence.BeginFence();
	});
}
void UComputeShaderTestComponent::RunComputeShader1()
{
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		if (!RenderCommandFence.IsFenceComplete()) return;
		ENQUEUE_RENDER_COMMAND(FComputeShaderRunner)(
			[&](FRHICommandListImmediate& RHICommands)
			{
				const auto sec0 = FPlatformTime::Seconds();
				//TShaderMapRef<FComputeShaderDeclaration1> cs(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				//TShaderMapRef<FComputeShaderDeclaration1> cs1(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				TShaderMapRef<FCornerShader> cornerCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				TShaderMapRef<FCornerIndShader> cornerIndCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				TShaderMapRef<FQEFShader> qefCS(GetGlobalShaderMap(ERHIFeatureLevel::SM5));
				{
					if(_countBuffer.IsValid())
					{
						
						_countBuffer->Release();
						_countBufferUAV->Release();	
					}
					TResourceArray<int> countResourceArray;
					
					countResourceArray.Init(0,20);
					
					FRHIResourceCreateInfo createInfo;
					createInfo.ResourceArray = &countResourceArray;

					_countBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int)*20, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
					_countBufferUAV = RHICreateUnorderedAccessView(_countBuffer, false, false);
				}


				validnum=0;
				FRHIComputeShader* cornerComputeShader = cornerCS.GetComputeShader();
			

				RHICommands.SetUAVParameter(cornerComputeShader,cornerCS->countutil.GetBaseIndex(),_countBufferUAV);
				RHICommands.SetShaderParameter(cornerComputeShader, cornerCS->gridsize.GetBufferIndex(), cornerCS->gridsize.GetBaseIndex(), 4, &gsize);
				RHICommands.SetShaderParameter(cornerComputeShader, cornerCS->boxpos.GetBufferIndex(), cornerCS->boxpos.GetBaseIndex(), 12, &boxposition);
				RHICommands.SetShaderParameter(cornerComputeShader, cornerCS->boxscale.GetBufferIndex(), cornerCS->boxscale.GetBaseIndex(), 4, &boxscalevalue);
				RHICommands.SetShaderParameter(cornerComputeShader, cornerCS->spherepos.GetBufferIndex(), cornerCS->spherepos.GetBaseIndex(), 12, &sphereposition);
				RHICommands.SetShaderParameter(cornerComputeShader, cornerCS->spherescale.GetBufferIndex(), cornerCS->spherescale.GetBaseIndex(), 4, &spherescalevalue);
				
				RHICommands.SetComputeShader(cornerComputeShader);
				DispatchComputeShader(RHICommands, cornerCS, gsize/4, gsize/4, gsize/4);
				
				

				TArray<int> tmpdata;
				tmpdata.Init(0,20);
				uint8* countdata = (uint8*)RHILockStructuredBuffer(_countBuffer, 0, 20 * sizeof(int), RLM_ReadOnly);
				FMemory::Memcpy(tmpdata.GetData(), countdata, 20 * sizeof(int));

				RHIUnlockStructuredBuffer(_countBuffer);
				const auto sec1 = FPlatformTime::Seconds();
				for(int i=0;i<16;++i)
				{
					validnum=validnum+tmpdata[i];
				}
				if(validnum>(_debugintBuffer->GetSize()/sizeof(int))||isfirst)
				{
					_debugintBuffer->Release();
					_debugintBufferUAV->Release();
					TResourceArray<int> debugintResourceArray;
					debugintResourceArray.Init(0, validnum);
					FRHIResourceCreateInfo createInfo;
					createInfo.ResourceArray = &debugintResourceArray;
					
					_debugintBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * validnum, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
					_debugintBufferUAV = RHICreateUnorderedAccessView(_debugintBuffer, false, false);
				}
				if(validnum>(_cornerBuffer->GetSize()/sizeof(int))||isfirst)
				{
					_cornerBuffer->Release();
					_cornerBufferUAV->Release();
					TResourceArray<int> cornerResourceArray;
					cornerResourceArray.Init(0, validnum);
					FRHIResourceCreateInfo createInfo;
					createInfo.ResourceArray = &cornerResourceArray;
					
					_cornerBuffer = RHICreateStructuredBuffer(sizeof(int), sizeof(int) * validnum, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
					_cornerBufferUAV = RHICreateUnorderedAccessView(_cornerBuffer, false, false);
				}
				FRHIComputeShader* cornerIndComputeShader = cornerIndCS.GetComputeShader();
			
				RHICommands.SetUAVParameter(cornerIndComputeShader, cornerIndCS->meshcorners.GetBaseIndex(), _cornerBufferUAV);
				RHICommands.SetUAVParameter(cornerIndComputeShader,cornerIndCS->countutil.GetBaseIndex(),_countBufferUAV);
				RHICommands.SetUAVParameter(cornerIndComputeShader,cornerIndCS->debugint.GetBaseIndex(),_debugintBufferUAV);
				RHICommands.SetShaderParameter(cornerIndComputeShader, cornerIndCS->gridsize.GetBufferIndex(), cornerIndCS->gridsize.GetBaseIndex(), 4, &gsize);
				RHICommands.SetShaderParameter(cornerIndComputeShader, cornerIndCS->boxpos.GetBufferIndex(), cornerIndCS->boxpos.GetBaseIndex(), 12, &boxposition);
				RHICommands.SetShaderParameter(cornerIndComputeShader, cornerIndCS->boxscale.GetBufferIndex(), cornerIndCS->boxscale.GetBaseIndex(), 4, &boxscalevalue);
				RHICommands.SetShaderParameter(cornerIndComputeShader, cornerIndCS->spherepos.GetBufferIndex(), cornerIndCS->spherepos.GetBaseIndex(), 12, &sphereposition);
				RHICommands.SetShaderParameter(cornerIndComputeShader, cornerIndCS->spherescale.GetBufferIndex(), cornerIndCS->spherescale.GetBaseIndex(), 4, &spherescalevalue);
				
				RHICommands.SetComputeShader(cornerIndComputeShader);
				DispatchComputeShader(RHICommands, cornerIndCS, gsize/4, gsize/4, gsize/4);
				const auto sec2 = FPlatformTime::Seconds();
				uint8* meshcornerdata = (uint8*)RHILockStructuredBuffer(_cornerBuffer, 0, _cornerBuffer->GetSize(), RLM_ReadOnly);
				FMemory::Memcpy(meshCorners.GetData(), meshcornerdata, validnum * sizeof(int));

				RHIUnlockStructuredBuffer(_cornerBuffer);

				uint8* meshinddata = (uint8*)RHILockStructuredBuffer(_debugintBuffer, 0, _debugintBuffer->GetSize(), RLM_ReadOnly);
				FMemory::Memcpy(trueInd.GetData(), meshinddata, validnum * sizeof(int));

				RHIUnlockStructuredBuffer(_debugintBuffer);


				countdata = (uint8*)RHILockStructuredBuffer(_countBuffer, 0, 20 * sizeof(int), RLM_ReadOnly);
				FMemory::Memcpy(tmpdata.GetData(), countdata, 20 * sizeof(int));

				RHIUnlockStructuredBuffer(_countBuffer);

				const auto sec3 = FPlatformTime::Seconds();
				
				if(validnum>(_meshBuffer->GetSize()/sizeof(FVector))||isfirst)
				{
					_meshBuffer->Release();
					_meshBufferUAV->Release();
					TResourceArray<FVector> meshResourceArray;
					meshResourceArray.Init(FVector::ZeroVector, validnum);

					FRHIResourceCreateInfo createInfo;
					createInfo.ResourceArray = &meshResourceArray;

					_meshBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * validnum, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
						
					_meshBufferUAV = RHICreateUnorderedAccessView(_meshBuffer, false, false);
				}

				if(validnum>(_normalBuffer->GetSize()/sizeof(FVector))||isfirst)
				{
					_normalBuffer->Release();
					_normalBufferUAV->Release();
					TResourceArray<FVector> normalResourceArray;
					normalResourceArray.Init(FVector::ZeroVector, validnum);

					FRHIResourceCreateInfo createInfo;
					createInfo.ResourceArray = &normalResourceArray;

					_normalBuffer = RHICreateStructuredBuffer(sizeof(FVector), sizeof(FVector) * validnum, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
					_normalBufferUAV = RHICreateUnorderedAccessView(_normalBuffer, false, false);
				}

				if(validnum>(_qefBuffer->GetSize()/sizeof(FVector))||isfirst)
				{
					_qefBuffer->Release();
					_qefBufferUAV->Release();
					TResourceArray<CSQef> qefResourceArray;
					CSQef tmp;
					qefResourceArray.Init(tmp,validnum);
					
					FRHIResourceCreateInfo createInfo;
					createInfo.ResourceArray = &qefResourceArray;

					_qefBuffer = RHICreateStructuredBuffer(sizeof(CSQef), sizeof(CSQef) * validnum, BUF_UnorderedAccess | BUF_ShaderResource, createInfo);
					_qefBufferUAV = RHICreateUnorderedAccessView(_qefBuffer, false, false);
				}
				const auto sec4 = FPlatformTime::Seconds();
				FRHIComputeShader* qefComputeShader = qefCS.GetComputeShader();

				
				RHICommands.SetUAVParameter(qefComputeShader, qefCS->meshpoints.GetBaseIndex(), _meshBufferUAV);
				RHICommands.SetUAVParameter(qefComputeShader, qefCS->meshnormals.GetBaseIndex(), _normalBufferUAV);
				RHICommands.SetUAVParameter(qefComputeShader, qefCS->meshcorners.GetBaseIndex(), _cornerBufferUAV);
				RHICommands.SetUAVParameter(qefComputeShader, qefCS->debugint.GetBaseIndex(), _debugintBufferUAV);
				RHICommands.SetUAVParameter(qefComputeShader, qefCS->meshqefs.GetBaseIndex(), _qefBufferUAV);
				RHICommands.SetShaderParameter(qefComputeShader, qefCS->gridsize.GetBufferIndex(), qefCS->gridsize.GetBaseIndex(), 4, &gsize);
				RHICommands.SetShaderParameter(qefComputeShader, qefCS->boxpos.GetBufferIndex(), qefCS->boxpos.GetBaseIndex(), 12, &boxposition);
				RHICommands.SetShaderParameter(qefComputeShader, qefCS->boxscale.GetBufferIndex(), qefCS->boxscale.GetBaseIndex(), 4, &boxscalevalue);
				RHICommands.SetShaderParameter(qefComputeShader, qefCS->spherepos.GetBufferIndex(), qefCS->spherepos.GetBaseIndex(), 12, &sphereposition);
				RHICommands.SetShaderParameter(qefComputeShader, qefCS->spherescale.GetBufferIndex(), qefCS->spherescale.GetBaseIndex(), 4, &spherescalevalue);
				RHICommands.SetShaderParameter(qefComputeShader, qefCS->validnum.GetBufferIndex(), qefCS->validnum.GetBaseIndex(), 4, &validnum);
				
				RHICommands.SetComputeShader(qefComputeShader);
				DispatchComputeShader(RHICommands, qefCS, validnum/32+1, 1, 1);
				const auto sec5 = FPlatformTime::Seconds();
				uint8* meshdata = (uint8*)RHILockStructuredBuffer(_meshBuffer, 0, _meshBuffer->GetSize(), RLM_ReadOnly);
				FMemory::Memcpy(meshPositions.GetData(), meshdata, validnum * sizeof(FVector));
				
				RHIUnlockStructuredBuffer(_meshBuffer);
				
				uint8* meshnormaldata = (uint8*)RHILockStructuredBuffer(_normalBuffer, 0, _normalBuffer->GetSize(), RLM_ReadOnly);
				FMemory::Memcpy(meshNormals.GetData(), meshnormaldata, validnum * sizeof(FVector));
				
				RHIUnlockStructuredBuffer(_normalBuffer);

				uint8* meshqefdata = (uint8*)RHILockStructuredBuffer(_qefBuffer, 0, _qefBuffer->GetSize(), RLM_ReadOnly);
				FMemory::Memcpy(meshQefs.GetData(), meshqefdata, validnum * sizeof(CSQef));
				
				RHIUnlockStructuredBuffer(_qefBuffer);


				
				ischange = true;
				isfirst=false;
				UMeshGenComponent* meshGenCompoennt = GetOwner()->FindComponentByClass<UMeshGenComponent>();
				
				const auto sec6 = FPlatformTime::Seconds();
				 UE_LOG(LogTemp, Warning, TEXT("compute shader analysis dispatch1:%f , dispatch2:%f ,readback2:%f,prepare3:%f ,dispatch3:%f ,readback3:%f"),
				 	(sec1-sec0) * 1000.0,(sec2-sec1) * 1000.0,(sec3-sec2) * 1000.0,(sec4-sec3) * 1000.0,(sec5-sec4) * 1000.0,(sec6-sec5)*1000);
				UE_LOG(LogTemp, Warning, TEXT("compute shader total:%f"),
					 (sec6-sec0) * 1000.0);
				meshGenCompoennt->GenMeshIm();
			});
		RenderCommandFence.BeginFence();
	});
}
// Called every frame
void UComputeShaderTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RunComputeShader1();

	
	


	
	


	
}
void UComputeShaderTestComponent::PostEditChangeProperty(FPropertyChangedEvent& event)
{
	RunComputeShader1();
}


FDualContouringShader::FDualContouringShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	meshpoints.Bind(Initializer.ParameterMap, TEXT("meshpoints"));
	debugint.Bind(Initializer.ParameterMap, TEXT("debugint"));
	meshnormals.Bind(Initializer.ParameterMap, TEXT("meshnormals"));
	meshcorners.Bind(Initializer.ParameterMap, TEXT("meshcorners"));
	gridsize.Bind(Initializer.ParameterMap, TEXT("gridsize"));
	boxpos.Bind(Initializer.ParameterMap, TEXT("boxpos"));
	boxscale.Bind(Initializer.ParameterMap, TEXT("boxscale"));
	spherepos.Bind(Initializer.ParameterMap, TEXT("spherepos"));
	spherescale.Bind(Initializer.ParameterMap, TEXT("spherescale"));
}

void FDualContouringShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

FCornerShader::FCornerShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	gridsize.Bind(Initializer.ParameterMap, TEXT("gridsize"));
	boxpos.Bind(Initializer.ParameterMap, TEXT("boxpos"));
	boxscale.Bind(Initializer.ParameterMap, TEXT("boxscale"));
	spherepos.Bind(Initializer.ParameterMap, TEXT("spherepos"));
	spherescale.Bind(Initializer.ParameterMap, TEXT("spherescale"));
	countutil.Bind(Initializer.ParameterMap,TEXT("countutil"));
}

void FCornerShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

FCornerIndShader::FCornerIndShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	meshcorners.Bind(Initializer.ParameterMap, TEXT("meshcorners"));
	countutil.Bind(Initializer.ParameterMap,TEXT("countutil"));
	debugint.Bind(Initializer.ParameterMap,TEXT("debugint"));
	gridsize.Bind(Initializer.ParameterMap, TEXT("gridsize"));
	boxpos.Bind(Initializer.ParameterMap, TEXT("boxpos"));
	boxscale.Bind(Initializer.ParameterMap, TEXT("boxscale"));
	spherepos.Bind(Initializer.ParameterMap, TEXT("spherepos"));
	spherescale.Bind(Initializer.ParameterMap, TEXT("spherescale"));
	
}

void FCornerIndShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

FQEFShader::FQEFShader(const ShaderMetaType::CompiledShaderInitializerType& Initializer) : FGlobalShader(Initializer)
{
	meshpoints.Bind(Initializer.ParameterMap, TEXT("meshpoints"));
	debugint.Bind(Initializer.ParameterMap, TEXT("debugint"));
	meshnormals.Bind(Initializer.ParameterMap, TEXT("meshnormals"));
	meshcorners.Bind(Initializer.ParameterMap, TEXT("meshcorners"));
	meshqefs.Bind(Initializer.ParameterMap,TEXT("meshqefs"));
	gridsize.Bind(Initializer.ParameterMap, TEXT("gridsize"));
	boxpos.Bind(Initializer.ParameterMap, TEXT("boxpos"));
	boxscale.Bind(Initializer.ParameterMap, TEXT("boxscale"));
	spherepos.Bind(Initializer.ParameterMap, TEXT("spherepos"));
	spherescale.Bind(Initializer.ParameterMap, TEXT("spherescale"));
	validnum.Bind(Initializer.ParameterMap,TEXT("validnum"));
}

void FQEFShader::ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
{
	FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	OutEnvironment.CompilerFlags.Add(CFLAG_StandardOptimization);
}

IMPLEMENT_SHADER_TYPE(, FDualContouringShader, TEXT("/ComputeShaderPlugin/DC.usf"), TEXT("MainComputeShader"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FCornerShader, TEXT("/ComputeShaderPlugin/DC1.usf"), TEXT("CornerCountComputeShader"), SF_Compute);
IMPLEMENT_SHADER_TYPE(, FCornerIndShader, TEXT("/ComputeShaderPlugin/DC1.usf"), TEXT("CornerIndComputeShader"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FQEFShader, TEXT("/ComputeShaderPlugin/DC1.usf"), TEXT("QEFComputeShader"), SF_Compute);

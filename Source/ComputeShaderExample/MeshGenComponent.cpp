// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshGenComponent.h"
#include "ComputeShaderTestComponent.h"
#include "MeshNormals.h"
const float QEF_ERROR = 1e-6f;
const int QEF_SWEEPS = 4;

void DestroyOctree(OctreeNode* node)
{
	if (!node)
	{
		return;
	}

	for (int i = 0; i < 8; i++)
	{
		DestroyOctree(node->children[i]);
		node->children[i]=NULL;
	}

	if (node->drawInfo)
	{
		delete node->drawInfo;
		node->drawInfo=NULL;
	}

	delete node;
}
OctreeNode* SimplifyOctree(OctreeNode* node, float threshold)
{
	if (!node)
	{
		return NULL;
	}

	if (node->type != Node_Internal)
	{
		// can't simplify!
		return node;
	}

	svd::QefSolver qef;
	int signs[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
	int midsign = -1;
	int edgeCount = 0;
	bool isCollapsible = true;

	for (int i = 0; i < 8; i++)
	{
		node->children[i] = SimplifyOctree(node->children[i], threshold);
		if (node->children[i])
		{
			OctreeNode* child = node->children[i];
			if (child->type == Node_Internal)
			{
				isCollapsible = false;
			}
			else
			{
				qef.add(child->drawInfo->qef);

				midsign = (child->drawInfo->corners >> (7 - i)) & 1; 
				signs[i] = (child->drawInfo->corners >> i) & 1; 

				edgeCount++;
			}
		}
	}

	if (!isCollapsible)
	{
		// at least one child is an internal node, can't collapse
		return node;
	}

	svd::Vec3 qefPosition;
	qef.solve(qefPosition, QEF_ERROR, QEF_SWEEPS, QEF_ERROR);
	float error = qef.getError();

	// convert to glm vec3 for ease of use
	FVector position(qefPosition.x, qefPosition.y, qefPosition.z);

	// at this point the masspoint will actually be a sum, so divide to make it the average
	if (error > threshold)
	{
		// this collapse breaches the threshold
		return node;
	}

	if (position.X < node->min.X || position.X > (node->min.X + node->size) ||
		position.Y < node->min.Y || position.Y > (node->min.Y + node->size) ||
		position.Z < node->min.Z || position.Z > (node->min.Z + node->size))
	{
		const auto& mp = qef.getMassPoint();
		position = FVector(mp.x, mp.y, mp.z);
	}

	// change the node from an internal node to a 'psuedo leaf' node
	OctreeDrawInfo* drawInfo = new OctreeDrawInfo;

	for (int i = 0; i < 8; i++)
	{
		if (signs[i] == -1)
		{
			// Undetermined, use centre sign instead
			drawInfo->corners |= (midsign << i);
		}
		else 
		{
			drawInfo->corners |= (signs[i] << i);
		}
	}

	drawInfo->averageNormal = FVector(0.f);
	for (int i = 0; i < 8; i++)
	{
		if (node->children[i])
		{
			OctreeNode* child = node->children[i];
			if (child->type == Node_Psuedo || 
				child->type == Node_Leaf)
			{
				drawInfo->averageNormal += child->drawInfo->averageNormal;
			}
		}
	}

	drawInfo->averageNormal = drawInfo->averageNormal.GetSafeNormal();
	drawInfo->position = position;
	drawInfo->qef = qef.getData();

	for (int i = 0; i < 8; i++)
	{
		DestroyOctree(node->children[i]);
		node->children[i] = nullptr;
	}

	node->type = Node_Psuedo;
	node->drawInfo = drawInfo;

	return node;
}
void GenerateVertexIndices(OctreeNode* node, TArray<FVector>& vertex,TArray<FVector>& normal)
{
	if (!node)
	{
		return;
	}

	if (node->type != Node_Leaf)
	{
		for (int i = 0; i < 8; i++)
		{
			GenerateVertexIndices(node->children[i], vertex,normal);
		}
	}

	if (node->type != Node_Internal)
	{
		OctreeDrawInfo* d = node->drawInfo;
		if (!d)
		{
			printf("Error! Could not add vertex!\n");
			exit(EXIT_FAILURE);
		}

		d->index = vertex.Num();
		vertex.Push(d->position);
		normal.Push(d->averageNormal);
	}
}
// Sets default values for this component's properties
UMeshGenComponent::UMeshGenComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	MeshComponent = CreateDefaultSubobject<USimpleDynamicMeshComponent>(TEXT("MeshComponent"), false);
	// ...
}


// Called when the game starts
void UMeshGenComponent::BeginPlay()
{
	Super::BeginPlay();
	/*FDynamicMesh3 mymesh = FDynamicMesh3();
	mymesh.AppendVertex(FVector3d(0.0, 0.0, 0.0));
	mymesh.AppendVertex(FVector3d(0.0, 0.0, 1.0));
	mymesh.AppendVertex(FVector3d(0.0, 1.0, 0.0));
	mymesh.AppendTriangle(FIndex3i(0, 1, 2));
	mymesh.EnableVertexColors(FVector3f::Zero());
	mymesh.SetVertexColor(0, FVector3f(1.0f, 0.0f, 0.0f));
	mymesh.SetVertexColor(1, FVector3f(0.0f, 1.0f, 0.0f));
	mymesh.SetVertexColor(2, FVector3f(0.0f, 0.0f, 1.0f));

	*(MeshComponent->GetMesh()) = mymesh;
	MeshComponent->NotifyMeshUpdated();
	UE_LOG(LogTemp, Warning, TEXT("Hello"));
	MeshComponent->SetMaterial(0, UseMaterial);*/
	// ...
	
}
void UMeshGenComponent::PostEditChangeProperty(FPropertyChangedEvent& event)
{
	
	GenMeshIm();
}

void UMeshGenComponent::GenMeshIm()
{
	UComputeShaderTestComponent* boidsComponent = GetOwner()->FindComponentByClass<UComputeShaderTestComponent>();
	
	const auto startSec = FPlatformTime::Seconds();
	vertex.Empty();
	normal.Empty();
	index.Empty();
	DestroyOctree(root);
	//root = BuildOctree(boidsComponent->meshPositions, boidsComponent->meshNormals, boidsComponent->meshCorners, boidsComponent->gsize);
	//BuildOctreeUpward(boidsComponent->meshPositions, boidsComponent->meshNormals, boidsComponent->meshCorners, boidsComponent->gsize);
	BuildOctreeUpward1(boidsComponent->meshPositions, boidsComponent->meshNormals, boidsComponent->meshCorners,boidsComponent->trueInd ,boidsComponent->meshQefs,boidsComponent->gsize,boidsComponent->validnum);
	SimplifyOctree(root,simplifyThreshold);
	GenerateVertexIndices(root,vertex,normal);
	ContourCellProc(root, index);
	
	 FDynamicMesh3 mymesh = FDynamicMesh3();
	 for (FVector vert : vertex)
	 {
		mymesh.AppendVertex(vert);
	 }
	 mymesh.EnableAttributes();
	 FDynamicMeshNormalOverlay* meshnormals=mymesh.Attributes()->PrimaryNormals();
	 for (FVector tnormal : normal)
	 {
		meshnormals->AppendElement(tnormal.GetSafeNormal());
	 }
	 for (int i = 0; i < index.Num(); i += 3)
	 {
		int32 tid=mymesh.AppendTriangle(FIndex3i(index[i], index[i + 1], index[i + 2]));
		if(tid<0)
		{
		 	continue;
		}
		meshnormals->SetTriangle(tid, FIndex3i(index[i], index[i + 1], index[i + 2]));
	 }
	 FMeshNormals::InitializeOverlayToPerVertexNormals(mymesh.Attributes()->PrimaryNormals(), false);
	 *(MeshComponent->GetMesh()) = mymesh;
	 MeshComponent->NotifyMeshUpdated();
	
	 MeshComponent->SetMaterial(0, UseMaterial);
	 MeshComponent->SetMaterial(1, UseMaterial1);
	boidsComponent->ischange = false;
	const auto endSec = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Warning, TEXT("mesh gen time is %f"),(endSec-startSec) * 1000);
	UE_LOG(LogTemp, Warning, TEXT("mesh vertex:%d,triangles:%d"),vertex.Num(),index.Num()/3);
	boidsComponent->isfinish=true;
	
}

// Called every frame
void UMeshGenComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	

	
	// ...
}
const int MATERIAL_AIR = 0;
const int MATERIAL_SOLID = 1;
const FVector CHILD_MIN_OFFSETS[] =
{
	// needs to match the vertMap from Dual Contouring impl
	FVector(0, 0, 0),
	FVector(0, 0, 1),
	FVector(0, 1, 0),
	FVector(0, 1, 1),
	FVector(1, 0, 0),
	FVector(1, 0, 1),
	FVector(1, 1, 0),
	FVector(1, 1, 1),
};

// ----------------------------------------------------------------------------
// data from the original DC impl, drives the contouring process

const int edgevmap[12][2] =
{
	{0,4},{1,5},{2,6},{3,7},	// x-axis 
	{0,2},{1,3},{4,6},{5,7},	// y-axis
	{0,1},{2,3},{4,5},{6,7}		// z-axis
};

const int edgemask[3] = { 5, 3, 6 };

const int vertMap[8][3] =
{
	{0,0,0},
	{0,0,1},
	{0,1,0},
	{0,1,1},
	{1,0,0},
	{1,0,1},
	{1,1,0},
	{1,1,1}
};

const int faceMap[6][4] = { {4, 8, 5, 9}, {6, 10, 7, 11},{0, 8, 1, 10},{2, 9, 3, 11},{0, 4, 2, 6},{1, 5, 3, 7} };
const int cellProcFaceMask[12][3] = { {0,4,0},{1,5,0},{2,6,0},{3,7,0},{0,2,1},{4,6,1},{1,3,1},{5,7,1},{0,1,2},{2,3,2},{4,5,2},{6,7,2} };
const int cellProcEdgeMask[6][5] = { {0,1,2,3,0},{4,5,6,7,0},{0,4,1,5,1},{2,6,3,7,1},{0,2,4,6,2},{1,3,5,7,2} };

const int faceProcFaceMask[3][4][3] = {
	{{4,0,0},{5,1,0},{6,2,0},{7,3,0}},
	{{2,0,1},{6,4,1},{3,1,1},{7,5,1}},
	{{1,0,2},{3,2,2},{5,4,2},{7,6,2}}
};

const int faceProcEdgeMask[3][4][6] = {
	{{1,4,0,5,1,1},{1,6,2,7,3,1},{0,4,6,0,2,2},{0,5,7,1,3,2}},
	{{0,2,3,0,1,0},{0,6,7,4,5,0},{1,2,0,6,4,2},{1,3,1,7,5,2}},
	{{1,1,0,3,2,0},{1,5,4,7,6,0},{0,1,5,0,4,1},{0,3,7,2,6,1}}
};

const int edgeProcEdgeMask[3][2][5] = {
	{{3,2,1,0,0},{7,6,5,4,0}},
	{{5,1,4,0,1},{7,3,6,2,1}},
	{{6,4,2,0,2},{7,5,3,1,2}},
};

const int processEdgeMask[3][4] = { {3,2,1,0},{7,5,6,4},{11,10,9,8} };

int coord2ind(int x, int y, int z, int gsize)
{
	return x * gsize * gsize + y * gsize + z;
}
FVector ind2coord(int ind, int gsize)
{
	FVector res;
	res.Z = ind % gsize;
	ind = ind / gsize;

	res.Y = ind % gsize;
	ind = ind / gsize;


	res.X = ind % gsize;

	return res;
}

OctreeNode* ConstructLeaf(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners, TArray<FVector>& vertex, TArray<FVector>& normal,OctreeNode* node,int gsize)
{
	int ind = coord2ind(node->min.X, node->min.Y, node->min.Z, gsize);
	if (!isnan(pos[ind].X))
	{
		if (corners[ind] == 0 || corners[ind] == 255)
		{
			delete node;
			return nullptr;
		}
		node->type = Node_Leaf;
		OctreeDrawInfo* drawInfo = new OctreeDrawInfo;
		
		drawInfo->position = pos[ind];
		drawInfo->corners = corners[ind];
		drawInfo->averageNormal = normals[ind];
		drawInfo->index = vertex.Num();
		vertex.Push(pos[ind]);
		normal.Push(normals[ind]);
		node->drawInfo = drawInfo;
		return node;

	}
	else
	{
		delete node;
		return nullptr;
	}

	return nullptr;
}
OctreeNode* ConstructOctreeNodes(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners, TArray<FVector>& vertex, TArray<FVector>& normal, OctreeNode* node,int gsize)
{
	if (!node)
	{
		return nullptr;
	}

	if (node->size == 1)
	{
		return ConstructLeaf(pos,normals,corners,vertex,normal,node,gsize);
	}

	const int childSize = node->size / 2;
	bool hasChildren = false;

	for (int i = 0; i < 8; i++)
	{
		OctreeNode* child = new OctreeNode;
		child->size = childSize;
		child->min = node->min + (CHILD_MIN_OFFSETS[i] * childSize);
		child->type = Node_Internal;

		node->children[i] = ConstructOctreeNodes(pos, normals, corners,vertex,normal,child,gsize);
		hasChildren |= (node->children[i] != nullptr);
	}

	if (!hasChildren)
	{
		delete node;
		return nullptr;
	}

	return node;
}

TArray<OctreeNode*> ConstructParents(TArray<OctreeNode*>& leafnodes, int gsize)
{
	TArray<OctreeNode*> res;
	return res;
}

OctreeNode* UMeshGenComponent::BuildOctree(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners,int gsize)
{
	OctreeNode* croot = new OctreeNode;
	croot->min = FVector(0,0,0);
	croot->size = gsize;
	croot->type = Node_Internal;

	ConstructOctreeNodes(pos, normals, corners,vertex,normal,croot,gsize);
	
	/*int childsize = 1;
	TArray<OctreeNode*> leafnodes;
	for (int i = 0; i < pos.Num(); ++i)
	{
		if (!isnan(pos[i].X))
		{
			OctreeNode* child = new OctreeNode;
			child->size = 1;
			child->min = ind2coord(i,gsize) + CHILD_MIN_OFFSETS[i]*childsize ;
			child->type = Node_Leaf;
			leafnodes.Push(child);
		}
	}
	while (childsize < gsize)
	{
		childsize *= 2;

	}*/
	
	return croot;
}

OctreeNode* UMeshGenComponent::BuildOctreeUpward(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners,int gsize)
{
	TQueue<OctreeNode*> utilqueue;
	TQueue<OctreeNode*> utilqueueback;
	TMap<FVector,OctreeNode*> utilmap;
	for(int i=0;i<corners.Num();++i)
	{
		if(corners[i]!=0&&corners[i]!=255&&!isnan(pos[i].X))
		{
			OctreeNode* node=new OctreeNode();
			node->type = Node_Leaf;
			node->min=ind2coord(i,gsize);
			node->size=1;
			OctreeDrawInfo* drawInfo = new OctreeDrawInfo;
		
			drawInfo->position = ind2coord(i,gsize);
			drawInfo->corners = corners[i];
			drawInfo->averageNormal = normals[i];
			drawInfo->index = vertex.Num();
			vertex.Push(pos[i]);
			normal.Push(normals[i]);
			node->drawInfo = drawInfo;
			utilqueue.Enqueue(node);
		}
	}
	while(!utilqueue.IsEmpty())
	{
		while(!utilqueue.IsEmpty())
		{
			OctreeNode *curnode=*(utilqueue.Peek());
			FVector curmin=curnode->min;
			int parentsize=curnode->size*2;
			FVector parentmin=FVector(int(curmin.X)/parentsize*parentsize,int(curmin.Y)/parentsize*parentsize,int(curmin.Z)/parentsize*parentsize);
			OctreeNode* parentnode=nullptr;
			int i=0;
			for(i=0;i<8;++i)
			{
				if(CHILD_MIN_OFFSETS[i]*curnode->size+parentmin==curmin)
				{
					break;
				}
			}
			if(utilmap.Find(parentmin))
			{
				parentnode=*utilmap.Find(parentmin);
				parentnode->children[i]=curnode;
			}
			else
			{
				parentnode=new OctreeNode();
				parentnode->min=parentmin;
				parentnode->type=Node_Internal;
				parentnode->size=parentsize;
				parentnode->children[i]=curnode;
				utilqueueback.Enqueue(parentnode);
				utilmap.Add(parentmin,parentnode);
				//utilmap[parentmin]=parentnode;
			}
			utilqueue.Dequeue(curnode);
		}
		while(!utilqueueback.IsEmpty())
		{
			utilqueue.Enqueue(*utilqueueback.Peek());
			utilqueueback.Dequeue(*utilqueueback.Peek());
		}
		if(utilmap.Num()==1)
		{
			OctreeNode *cur=utilmap.begin()->Value;
			if(cur->size==gsize)
			{
				root=cur;
				break;
			}
		}
		utilmap.Empty();
		
		
	}
	
	return nullptr;
	
}

OctreeNode* UMeshGenComponent::BuildOctreeUpward1(TArray<FVector>& pos, TArray<FVector>& normals, TArray<int>& corners,TArray<int>& trueind,TArray<CSQef>& qefs,int gsize,int validnum)
{
	TQueue<OctreeNode*> utilqueue;
	TQueue<OctreeNode*> utilqueueback;
	TMap<FVector,OctreeNode*> utilmap;
	for(int i=0;i<validnum;++i)
	{
		if(corners[i]!=0&&corners[i]!=255&&!isnan(pos[i].X))
		{
			OctreeNode* node=new OctreeNode();
			node->type = Node_Leaf;
			node->min=ind2coord(trueind[i],gsize);
			node->size=1;
			OctreeDrawInfo* drawInfo = new OctreeDrawInfo;
		
			drawInfo->position = pos[i];
			drawInfo->corners = corners[i];
			drawInfo->averageNormal = normals[i];
			drawInfo->index = vertex.Num();
			drawInfo->qef.set(qefs[i].ata_00,qefs[i].ata_01,qefs[i].ata_02,qefs[i].ata_11,qefs[i].ata_12,qefs[i].ata_22,qefs[i].atb_x,qefs[i].atb_y,qefs[i].atb_z,qefs[i].btb,
			pos[i].X,pos[i].Y,pos[i].Z,1
			);
			
			node->drawInfo = drawInfo;
			utilqueue.Enqueue(node);
		}
	}
	while(!utilqueue.IsEmpty())
	{
		while(!utilqueue.IsEmpty())
		{
			OctreeNode *curnode=*(utilqueue.Peek());
			FVector curmin=curnode->min;
			int parentsize=curnode->size*2;
			FVector parentmin=FVector(int(curmin.X)/parentsize*parentsize,int(curmin.Y)/parentsize*parentsize,int(curmin.Z)/parentsize*parentsize);
			OctreeNode* parentnode=nullptr;
			int i=0;
			for(i=0;i<8;++i)
			{
				if(CHILD_MIN_OFFSETS[i]*curnode->size+parentmin==curmin)
				{
					break;
				}
			}
			if(utilmap.Find(parentmin))
			{
				parentnode=*utilmap.Find(parentmin);
				parentnode->children[i]=curnode;
			}
			else
			{
				parentnode=new OctreeNode();
				parentnode->min=parentmin;
				parentnode->type=Node_Internal;
				parentnode->size=parentsize;
				parentnode->children[i]=curnode;
				utilqueueback.Enqueue(parentnode);
				utilmap.Add(parentmin,parentnode);
				//utilmap[parentmin]=parentnode;
			}
			utilqueue.Dequeue(curnode);
		}
		while(!utilqueueback.IsEmpty())
		{
			utilqueue.Enqueue(*utilqueueback.Peek());
			utilqueueback.Dequeue(*utilqueueback.Peek());
		}
		if(utilmap.Num()==1)
		{
			OctreeNode *cur=utilmap.begin()->Value;
			if(cur->size==gsize)
			{
				root=cur;
				break;
			}
		}
		utilmap.Empty();
		
		
	}
	
	return nullptr;
	
}



void ContourProcessEdge(OctreeNode* node[4], int dir, TArray<int>& indexBuffer)
{
	int minSize = 1000000;		// arbitrary big number
	int minIndex = 0;
	int indices[4] = { -1, -1, -1, -1 };
	bool flip = false;
	bool signChange[4] = { false, false, false, false };

	for (int i = 0; i < 4; i++)
	{
		const int edge = processEdgeMask[dir][i];
		const int c1 = edgevmap[edge][0];
		const int c2 = edgevmap[edge][1];

		const int m1 = (node[i]->drawInfo->corners >> c1) & 1;
		const int m2 = (node[i]->drawInfo->corners >> c2) & 1;

		if (node[i]->size < minSize)
		{
			minSize = node[i]->size;
			minIndex = i;
			flip = m1 != MATERIAL_AIR;
		}

		indices[i] = node[i]->drawInfo->index;

		signChange[i] =
			(m1 == MATERIAL_AIR && m2 != MATERIAL_AIR) ||
			(m1 != MATERIAL_AIR && m2 == MATERIAL_AIR);
	}

	if (signChange[minIndex])
	{
		if (!flip)
		{
			indexBuffer.Push(indices[0]);
			indexBuffer.Push(indices[1]);
			indexBuffer.Push(indices[3]);

			indexBuffer.Push(indices[0]);
			indexBuffer.Push(indices[3]);
			indexBuffer.Push(indices[2]);
		}
		else
		{
			indexBuffer.Push(indices[0]);
			indexBuffer.Push(indices[3]);
			indexBuffer.Push(indices[1]);

			indexBuffer.Push(indices[0]);
			indexBuffer.Push(indices[2]);
			indexBuffer.Push(indices[3]);
		}
	}
}

// ----------------------------------------------------------------------------

void ContourEdgeProc(OctreeNode* node[4], int dir, TArray<int>& indexBuffer)
{
	if (!node[0] || !node[1] || !node[2] || !node[3])
	{
		return;
	}

	if (node[0]->type != Node_Internal &&
		node[1]->type != Node_Internal &&
		node[2]->type != Node_Internal &&
		node[3]->type != Node_Internal)
	{
		ContourProcessEdge(node, dir, indexBuffer);
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			OctreeNode* edgeNodes[4];
			const int c[4] =
			{
				edgeProcEdgeMask[dir][i][0],
				edgeProcEdgeMask[dir][i][1],
				edgeProcEdgeMask[dir][i][2],
				edgeProcEdgeMask[dir][i][3],
			};

			for (int j = 0; j < 4; j++)
			{
				if (node[j]->type == Node_Leaf || node[j]->type == Node_Psuedo)
				{
					edgeNodes[j] = node[j];
				}
				else
				{
					edgeNodes[j] = node[j]->children[c[j]];
				}
			}

			ContourEdgeProc(edgeNodes, edgeProcEdgeMask[dir][i][4], indexBuffer);
		}
	}
}

// ----------------------------------------------------------------------------

void ContourFaceProc(OctreeNode* node[2], int dir, TArray<int>& indexBuffer)
{
	if (!node[0] || !node[1])
	{
		return;
	}

	if (node[0]->type == Node_Internal ||
		node[1]->type == Node_Internal)
	{
		for (int i = 0; i < 4; i++)
		{
			OctreeNode* faceNodes[2];
			const int c[2] =
			{
				faceProcFaceMask[dir][i][0],
				faceProcFaceMask[dir][i][1],
			};

			for (int j = 0; j < 2; j++)
			{
				if (node[j]->type != Node_Internal)
				{
					faceNodes[j] = node[j];
				}
				else
				{
					faceNodes[j] = node[j]->children[c[j]];
				}
			}

			ContourFaceProc(faceNodes, faceProcFaceMask[dir][i][2], indexBuffer);
		}

		const int orders[2][4] =
		{
			{ 0, 0, 1, 1 },
			{ 0, 1, 0, 1 },
		};
		for (int i = 0; i < 4; i++)
		{
			OctreeNode* edgeNodes[4];
			const int c[4] =
			{
				faceProcEdgeMask[dir][i][1],
				faceProcEdgeMask[dir][i][2],
				faceProcEdgeMask[dir][i][3],
				faceProcEdgeMask[dir][i][4],
			};

			const int* order = orders[faceProcEdgeMask[dir][i][0]];
			for (int j = 0; j < 4; j++)
			{
				if (node[order[j]]->type == Node_Leaf ||
					node[order[j]]->type == Node_Psuedo)
				{
					edgeNodes[j] = node[order[j]];
				}
				else
				{
					edgeNodes[j] = node[order[j]]->children[c[j]];
				}
			}

			ContourEdgeProc(edgeNodes, faceProcEdgeMask[dir][i][5], indexBuffer);
		}
	}
}

// ----------------------------------------------------------------------------

void ContourCellProc(OctreeNode* node, TArray<int>& indexBuffer)
{
	if (node == NULL)
	{
		return;
	}
	if ((node->min == FVector(4, 2, 4))&&(node->size==2))
	{
		int x = 1;
	}
	if (node->type == Node_Internal)
	{
		for (int i = 0; i < 8; i++)
		{
			ContourCellProc(node->children[i], indexBuffer);
		}

		for (int i = 0; i < 12; i++)
		{
			OctreeNode* faceNodes[2];
			const int c[2] = { cellProcFaceMask[i][0], cellProcFaceMask[i][1] };

			faceNodes[0] = node->children[c[0]];
			faceNodes[1] = node->children[c[1]];

			ContourFaceProc(faceNodes, cellProcFaceMask[i][2], indexBuffer);
		}

		for (int i = 0; i < 6; i++)
		{
			OctreeNode* edgeNodes[4];
			const int c[4] =
			{
				cellProcEdgeMask[i][0],
				cellProcEdgeMask[i][1],
				cellProcEdgeMask[i][2],
				cellProcEdgeMask[i][3],
			};

			for (int j = 0; j < 4; j++)
			{
				edgeNodes[j] = node->children[c[j]];
			}

			ContourEdgeProc(edgeNodes, cellProcEdgeMask[i][4], indexBuffer);
		}
	}
}






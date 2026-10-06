#include "World/SpearfishRegionBuilder.h"

#include "Boat/SpearfishStation.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Core/SpearfishSettings.h"
#include "Data/SpearfishDataRegistry.h"
#include "DayNight/SpearfishSkyController.h"
#include "Engine/World.h"
#include "FishAI/SpearfishAmbientSchool.h"
#include "FishAI/SpearfishFishSubsystem.h"
#include "HowToSpearfish.h"
#include "Loot/SpearfishGiantClam.h"
#include "Loot/SpearfishLootPickup.h"
#include "Loot/SpearfishPullable.h"
#include "Net/UnrealNetwork.h"
#include "ProceduralMeshComponent.h"
#include "World/SpearfishOceanSubsystem.h"
#include "World/SpearfishOceanSurface.h"
#include "World/SpearfishVisualSubsystem.h"

namespace SpearfishRegionPrivate
{
	enum ECollisionMode : int32
	{
		NoCollision = 0,
		FullCollision = 1,
		LineCollision = 2 // blocks harpoon shots and line wrapping, divers swim through
	};

	uint32 InstancerKey(int32 Shape, const FLinearColor& Color, int32 Mode)
	{
		const FColor Q = Color.ToFColor(false);
		return HashCombine(HashCombine(static_cast<uint32>(Shape), static_cast<uint32>(Mode)), HashCombine(HashCombine(Q.R, Q.G), Q.B));
	}

	/** Triangle with its face oriented toward Up (see Docs/ARCHITECTURE.md, procedural winding). */
	void AddOrientedTriangle(TArray<int32>& Triangles, const TArray<FVector>& Vertices, int32 A, int32 B, int32 C, const FVector& Up)
	{
		const FVector Normal = (Vertices[B] - Vertices[A]) ^ (Vertices[C] - Vertices[A]);
		if (FVector::DotProduct(Normal, Up) >= 0.0)
		{
			Triangles.Add(A);
			Triangles.Add(B);
			Triangles.Add(C);
		}
		else
		{
			Triangles.Add(A);
			Triangles.Add(C);
			Triangles.Add(B);
		}
	}
}

ASpearfishRegionBuilder::ASpearfishRegionBuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void ASpearfishRegionBuilder::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASpearfishRegionBuilder, RegionId);
	DOREPLIFETIME(ASpearfishRegionBuilder, Seed);
}

void ASpearfishRegionBuilder::Configure(FName InRegionId, int32 InSeed)
{
	RegionId = InRegionId;
	Seed = InSeed;
}

const FSpearfishRegionDef* ASpearfishRegionBuilder::GetRegionDef() const
{
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	return Registry ? Registry->FindRegion(RegionId) : nullptr;
}

void ASpearfishRegionBuilder::BeginPlay()
{
	Super::BeginPlay();
	BuildStatic();
}

void ASpearfishRegionBuilder::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AActor* Actor : LocalActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	ClearDailyContent();
	for (AActor* Actor : HubActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	if (USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this))
	{
		Fish->ClearWorldData();
	}
	Super::EndPlay(EndPlayReason);
}

void ASpearfishRegionBuilder::OnRep_Region()
{
	BuildStatic();
}

void ASpearfishRegionBuilder::BuildStatic()
{
	const FSpearfishRegionDef* Region = GetRegionDef();
	if (bBuilt || !Region || !HasActorBegunPlay())
	{
		return;
	}
	bBuilt = true;

	USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!Ocean)
	{
		return;
	}
	Ocean->SetRegion(RegionId, Region->Terrain, Seed);

	BuildTerrainMesh();
	ScatterScenery();
	for (const FSpearfishPOI& POI : Ocean->GetTerrain().GetLayout().POIs)
	{
		if (POI.Type == ESpearfishPOIType::Wreck)
		{
			BuildWreck(POI);
		}
		else if (POI.Type == ESpearfishPOIType::Cave)
		{
			BuildCave(POI);
		}
	}
	BuildIsland();
	SpawnLocalActors();

	Habitat = Ocean->GetTerrain().SampleHabitat(650.f, Ocean->GetSeaLevel());
	if (USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this))
	{
		Fish->SetHabitat(Habitat);
	}
	UE_LOG(LogSpearfish, Log, TEXT("Region %s built (seed %d, %d habitat points)"), *RegionId.ToString(), Seed, Habitat.Num());
}

UHierarchicalInstancedStaticMeshComponent* ASpearfishRegionBuilder::GetInstancer(int32 Shape, const FLinearColor& Color, int32 CollisionMode)
{
	using namespace SpearfishRegionPrivate;
	const uint32 Key = InstancerKey(Shape, Color, CollisionMode);
	if (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Existing = Instancers.Find(Key))
	{
		return *Existing;
	}
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	UHierarchicalInstancedStaticMeshComponent* Instancer = Visuals->AddInstancer(this, Root, static_cast<ESpearfishShape>(Shape), Color, CollisionMode == FullCollision);
	if (CollisionMode == LineCollision)
	{
		Instancer->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Instancer->SetCollisionObjectType(ECC_WorldStatic);
		Instancer->SetCollisionResponseToAllChannels(ECR_Ignore);
		Instancer->SetCollisionResponseToChannel(ECC_Harpoon, ECR_Block);
		Instancer->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Instancer->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	}
	Instancer->SetCullDistances(0, 14000);
	Instancers.Add(Key, Instancer);
	return Instancer;
}

// ------------------------------------------------------------------------------------ Terrain

void ASpearfishRegionBuilder::BuildTerrainMesh()
{
	using namespace SpearfishRegionPrivate;
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const FSpearfishRegionDef* Region = GetRegionDef();
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (!Ocean || !Region || !Visuals)
	{
		return;
	}
	const FSpearfishTerrain& Terrain = Ocean->GetTerrain();
	const float SeaLevel = Ocean->GetSeaLevel();

	TerrainMesh = NewObject<UProceduralMeshComponent>(this, TEXT("Seabed"));
	TerrainMesh->SetupAttachment(Root);
	TerrainMesh->bUseAsyncCooking = true;
	TerrainMesh->SetCollisionProfileName(TEXT("BlockAll"));
	TerrainMesh->RegisterComponent();
	AddInstanceComponent(TerrainMesh);

	// Fine grid over the playable square, coarse ring beyond it for the drop into the blue.
	const float Half = Terrain.GetHalfExtent() + 6000.f;
	const float Step = 200.f;
	const int32 Count = FMath::CeilToInt(2.f * Half / Step) + 1;

	TArray<FVector> Positions;
	TArray<FVector> Normals;
	Positions.Reserve(Count * Count);
	Normals.Reserve(Count * Count);
	for (int32 IY = 0; IY < Count; ++IY)
	{
		for (int32 IX = 0; IX < Count; ++IX)
		{
			const float X = -Half + IX * Step;
			const float Y = -Half + IY * Step;
			Positions.Add(FVector(X, Y, Terrain.GetSeabedZ(X, Y)));
			Normals.Add(Terrain.GetNormal(X, Y));
		}
	}

	// Four material sections: sand, rock (steep), deep sand, island grass.
	constexpr int32 SectionCount = 4;
	TArray<FVector> SectionVertices[SectionCount];
	TArray<FVector> SectionNormals[SectionCount];
	TArray<int32> SectionTriangles[SectionCount];
	TArray<FVector2D> SectionUVs[SectionCount];

	auto Classify = [SeaLevel](const FVector& A, const FVector& B, const FVector& C, const FVector& Normal) -> int32
	{
		const float Z = static_cast<float>((A.Z + B.Z + C.Z) / 3.0);
		if (Z > SeaLevel + 150.f)
		{
			return 3;
		}
		if (Normal.Z < 0.78)
		{
			return 1;
		}
		return (SeaLevel - Z) > 2600.f ? 2 : 0;
	};

	for (int32 IY = 0; IY + 1 < Count; ++IY)
	{
		for (int32 IX = 0; IX + 1 < Count; ++IX)
		{
			const int32 I00 = IY * Count + IX;
			const int32 I10 = IY * Count + IX + 1;
			const int32 I01 = (IY + 1) * Count + IX;
			const int32 I11 = (IY + 1) * Count + IX + 1;
			const int32 Quads[2][3] = { { I00, I10, I11 }, { I00, I11, I01 } };
			for (const auto& Tri : Quads)
			{
				const FVector& A = Positions[Tri[0]];
				const FVector& B = Positions[Tri[1]];
				const FVector& C = Positions[Tri[2]];
				const FVector FaceNormal = ((B - A) ^ (C - A)).GetSafeNormal();
				const FVector UpFacing = FaceNormal.Z < 0.0 ? -FaceNormal : FaceNormal;
				const int32 Section = Classify(A, B, C, UpFacing);

				TArray<FVector>& Vertices = SectionVertices[Section];
				const int32 Base = Vertices.Num();
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					Vertices.Add(Positions[Tri[Corner]]);
					SectionNormals[Section].Add(Normals[Tri[Corner]]);
					SectionUVs[Section].Add(FVector2D(Positions[Tri[Corner]].X / 400.0, Positions[Tri[Corner]].Y / 400.0));
				}
				AddOrientedTriangle(SectionTriangles[Section], Vertices, Base, Base + 1, Base + 2, FVector::UpVector);
			}
		}
	}

	const FLinearColor Colors[SectionCount] = { Region->Palette.Sand, Region->Palette.Rock, Region->Palette.Sand * 0.55f, Region->Palette.Vegetation };
	for (int32 Section = 0; Section < SectionCount; ++Section)
	{
		if (SectionVertices[Section].Num() == 0)
		{
			continue;
		}
		TArray<FLinearColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;
		TerrainMesh->CreateMeshSection_LinearColor(Section, SectionVertices[Section], SectionTriangles[Section], SectionNormals[Section],
			SectionUVs[Section], VertexColors, Tangents, true);
		TerrainMesh->SetMaterial(Section, Visuals->GetColorMaterial(Colors[Section]));
	}
}

// ------------------------------------------------------------------------------------ Scenery

void ASpearfishRegionBuilder::ScatterScenery()
{
	using namespace SpearfishRegionPrivate;
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const FSpearfishRegionDef* Region = GetRegionDef();
	USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this);
	if (!Ocean || !Region)
	{
		return;
	}
	const FSpearfishTerrain& Terrain = Ocean->GetTerrain();
	const FSpearfishTerrainParams& Params = Region->Terrain;
	const float SeaLevel = Ocean->GetSeaLevel();
	FRandomStream Rng(Seed * 13 + 1);

	const int32 Sphere = static_cast<int32>(ESpearfishShape::Sphere);
	const int32 Cube = static_cast<int32>(ESpearfishShape::Cube);
	const int32 Cylinder = static_cast<int32>(ESpearfishShape::Cylinder);
	const int32 Cone = static_cast<int32>(ESpearfishShape::Cone);
	static const FName Reef(TEXT("Reef"));
	static const FName Seagrass(TEXT("Seagrass"));
	static const FName Kelp(TEXT("Kelp"));
	static const FName Wall(TEXT("Wall"));
	static const FName Land(TEXT("Land"));

	const TArray<FLinearColor> CoralColors = Region->Palette.Coral.Num() > 0 ? Region->Palette.Coral : TArray<FLinearColor>({ FLinearColor(0.9f, 0.4f, 0.5f) });
	const float Range = Terrain.GetHalfExtent() - 200.f;
	const float Grid = 420.f;

	for (float GX = -Range; GX < Range; GX += Grid)
	{
		for (float GY = -Range; GY < Range; GY += Grid)
		{
			const float X = GX + Rng.FRandRange(-Grid * 0.45f, Grid * 0.45f);
			const float Y = GY + Rng.FRandRange(-Grid * 0.45f, Grid * 0.45f);
			const float Z = Terrain.GetSeabedZ(X, Y);
			const FName Biome = Terrain.GetBiome(X, Y);
			const float DepthM = (SeaLevel - Z) / 100.f;
			if (Biome == Land || DepthM < 1.2f)
			{
				continue;
			}
			const FVector Ground(X, Y, Z);
			const FVector Normal = Terrain.GetNormal(X, Y);

			// Rocks: more on walls and reefs; they give fish cover and block divers.
			const float RockChance = Params.RockDensity * (Biome == Wall ? 0.9f : (Biome == Reef ? 0.35f : 0.12f));
			if (Rng.FRand() < RockChance)
			{
				const float Size = Rng.FRandRange(120.f, 420.f) * (Biome == Wall ? 1.4f : 1.f);
				const FLinearColor Tint = Region->Palette.Rock * Rng.FRandRange(0.8f, 1.15f);
				const FLinearColor Quantized(FMath::GridSnap(Tint.R, 0.05f), FMath::GridSnap(Tint.G, 0.05f), FMath::GridSnap(Tint.B, 0.05f));
				const FTransform Rock(FRotator(Rng.FRandRange(-20.f, 20.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-20.f, 20.f)),
					Ground + Normal * (Size * 0.2), FVector(Size, Size * Rng.FRandRange(0.6f, 1.2f), Size * Rng.FRandRange(0.4f, 0.8f)) / 100.0);
				GetInstancer(Rng.FRand() < 0.6f ? Sphere : Cube, Quantized, FullCollision)->AddInstance(Rock, true);
				if (Fish)
				{
					Fish->RegisterCoverPoint(Ground + FVector(0.0, 0.0, Size * 0.6));
				}
			}

			// Coral clusters on the reef.
			if (Biome == Reef && Rng.FRand() < Params.ReefDensity * 0.85f)
			{
				const int32 Pieces = Rng.RandRange(3, 7);
				for (int32 Piece = 0; Piece < Pieces; ++Piece)
				{
					const FLinearColor Color = CoralColors[Rng.RandRange(0, CoralColors.Num() - 1)];
					const FVector Offset(Rng.FRandRange(-160.f, 160.f), Rng.FRandRange(-160.f, 160.f), 0.0);
					const FVector Base(X + Offset.X, Y + Offset.Y, Terrain.GetSeabedZ(X + static_cast<float>(Offset.X), Y + static_cast<float>(Offset.Y)));
					const int32 Kind = Rng.RandRange(0, 3);
					if (Kind == 0)
					{
						// Branching staghorn.
						for (int32 Branch = 0; Branch < 4; ++Branch)
						{
							const float Height = Rng.FRandRange(50.f, 130.f);
							const FRotator Tilt(Rng.FRandRange(-35.f, 35.f), Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-35.f, 35.f));
							GetInstancer(Cone, Color, LineCollision)->AddInstance(FTransform(Tilt, Base + FVector(0, 0, Height * 0.45f), FVector(14.f, 14.f, Height) / 100.0), true);
						}
					}
					else if (Kind == 1)
					{
						const float Size = Rng.FRandRange(60.f, 160.f);
						GetInstancer(Sphere, Color, LineCollision)->AddInstance(FTransform(FRotator::ZeroRotator, Base + FVector(0, 0, Size * 0.25f), FVector(Size, Size, Size * 0.6f) / 100.0), true);
					}
					else if (Kind == 2)
					{
						const float Size = Rng.FRandRange(70.f, 160.f);
						GetInstancer(Cube, Color, LineCollision)->AddInstance(FTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), Rng.FRandRange(-10.f, 10.f)), Base + FVector(0, 0, Size * 0.5f), FVector(4.f, Size, Size) / 100.0), true);
					}
					else
					{
						const float Size = Rng.FRandRange(90.f, 220.f);
						GetInstancer(Cylinder, Color, LineCollision)->AddInstance(FTransform(FRotator::ZeroRotator, Base + FVector(0, 0, 45.f), FVector(10.f, 10.f, 90.f) / 100.0), true);
						GetInstancer(Cylinder, Color, LineCollision)->AddInstance(FTransform(FRotator::ZeroRotator, Base + FVector(0, 0, 92.f), FVector(Size, Size * 0.8f, 8.f) / 100.0), true);
					}
				}
				if (Fish)
				{
					Fish->RegisterCoverPoint(Ground + FVector(0.0, 0.0, 80.0));
				}
			}

			// Seagrass meadows.
			if (Biome == Seagrass)
			{
				const FLinearColor Grass = Region->Palette.Vegetation * Rng.FRandRange(0.8f, 1.2f);
				const FLinearColor Quantized(FMath::GridSnap(Grass.R, 0.05f), FMath::GridSnap(Grass.G, 0.05f), FMath::GridSnap(Grass.B, 0.05f));
				for (int32 Blade = 0; Blade < 14; ++Blade)
				{
					const float Height = Rng.FRandRange(30.f, 80.f);
					const FVector Base(X + Rng.FRandRange(-150.f, 150.f), Y + Rng.FRandRange(-150.f, 150.f), 0.0);
					const FVector Position(Base.X, Base.Y, Terrain.GetSeabedZ(static_cast<float>(Base.X), static_cast<float>(Base.Y)) + Height * 0.5f);
					GetInstancer(Cube, Quantized, NoCollision)->AddInstance(FTransform(FRotator(Rng.FRandRange(-12.f, 12.f), Rng.FRandRange(0.f, 360.f), 0.f), Position, FVector(1.5f, 5.f, Height) / 100.0), true);
				}
			}

			// Kelp forests: tall stalks that tangle harpoon lines.
			if (Biome == Kelp && DepthM > 3.f)
			{
				const int32 Stalks = Rng.RandRange(1, 3);
				for (int32 Stalk = 0; Stalk < Stalks; ++Stalk)
				{
					const float Height = (DepthM - 1.f) * 100.f * Rng.FRandRange(0.7f, 0.95f);
					const FVector Base(X + Rng.FRandRange(-120.f, 120.f), Y + Rng.FRandRange(-120.f, 120.f), Z);
					GetInstancer(Cylinder, Region->Palette.Vegetation, LineCollision)->AddInstance(FTransform(FRotator(Rng.FRandRange(-6.f, 6.f), 0.f, Rng.FRandRange(-6.f, 6.f)), Base + FVector(0, 0, Height * 0.5f), FVector(7.f, 7.f, Height) / 100.0), true);
					for (int32 Leaf = 0; Leaf < 5; ++Leaf)
					{
						const float LeafZ = Height * (0.3f + 0.14f * Leaf);
						GetInstancer(Cube, Region->Palette.Vegetation * 1.2f, NoCollision)->AddInstance(FTransform(FRotator(Rng.FRandRange(-30.f, 30.f), Rng.FRandRange(0.f, 360.f), 0.f), Base + FVector(0, 0, LeafZ), FVector(60.f, 18.f, 1.5f) / 100.0), true);
					}
				}
				if (Fish)
				{
					Fish->RegisterCoverPoint(Ground + FVector(0.0, 0.0, 200.0));
				}
			}
		}
	}

	// Reef heads (bommies): big coral mounds that are landmarks and fish magnets.
	for (const FSpearfishPOI& POI : Terrain.GetLayout().POIs)
	{
		if (POI.Type != ESpearfishPOIType::ReefHead)
		{
			continue;
		}
		for (int32 Piece = 0; Piece < 9; ++Piece)
		{
			const FLinearColor Color = CoralColors[Rng.RandRange(0, CoralColors.Num() - 1)];
			const FVector Offset(Rng.FRandRange(-260.f, 260.f), Rng.FRandRange(-260.f, 260.f), 0.0);
			const float Size = Rng.FRandRange(120.f, 260.f);
			const FVector Base = POI.Location + Offset;
			const float BaseZ = Terrain.GetSeabedZ(static_cast<float>(Base.X), static_cast<float>(Base.Y));
			GetInstancer(Sphere, Color, FullCollision)->AddInstance(FTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(Base.X, Base.Y, BaseZ + Size * 0.3f), FVector(Size, Size * 0.9f, Size * 0.7f) / 100.0), true);
		}
		if (Fish)
		{
			Fish->RegisterCoverPoint(POI.Location + FVector(0.0, 0.0, 250.0));
		}
	}
}

void ASpearfishRegionBuilder::BuildWreck(const FSpearfishPOI& POI)
{
	using namespace SpearfishRegionPrivate;
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this);
	if (!Visuals)
	{
		return;
	}
	// A listing hull with a hollow interior you can swim into.
	USceneComponent* Pivot = NewObject<USceneComponent>(this);
	Pivot->SetupAttachment(Root);
	Pivot->SetWorldLocationAndRotation(POI.Location + FVector(0, 0, 160), FRotator(0.f, POI.Yaw, 14.f));
	Pivot->RegisterComponent();

	const FLinearColor Hull(0.32f, 0.27f, 0.22f);
	const FLinearColor Rust(0.45f, 0.25f, 0.12f);
	const FLinearColor Plank(0.4f, 0.33f, 0.25f);
	auto Part = [this, Visuals, Pivot](ESpearfishShape Shape, const FVector& Location, const FRotator& Rotation, const FVector& Size, const FLinearColor& Color)
	{
		return Visuals->AddPart(this, Pivot, Shape, Location, Rotation, Size, Color, true);
	};
	Part(ESpearfishShape::Cube, FVector(0, 0, -150), FRotator::ZeroRotator, FVector(1700, 460, 30), Hull);
	Part(ESpearfishShape::Cube, FVector(0, -225, 0), FRotator::ZeroRotator, FVector(1700, 30, 320), Hull);
	Part(ESpearfishShape::Cube, FVector(-450, 225, 0), FRotator::ZeroRotator, FVector(800, 30, 320), Hull);
	Part(ESpearfishShape::Cube, FVector(600, 225, 0), FRotator::ZeroRotator, FVector(500, 30, 320), Rust);
	Part(ESpearfishShape::Cube, FVector(-850, 0, 0), FRotator::ZeroRotator, FVector(30, 460, 320), Hull);
	Part(ESpearfishShape::Cube, FVector(-300, 0, 165), FRotator::ZeroRotator, FVector(1100, 460, 20), Plank);
	Part(ESpearfishShape::Cube, FVector(900, 0, -20), FRotator(0, 45, 0), FVector(330, 330, 260), Hull);
	// Fallen mast and a broken spar across the sand.
	Part(ESpearfishShape::Cylinder, FVector(200, 500, -120), FRotator(80, 30, 0), FVector(26, 26, 1200), Plank);
	Part(ESpearfishShape::Cylinder, FVector(-200, -600, -150), FRotator(90, -20, 0), FVector(18, 18, 600), Plank);

	if (Fish)
	{
		Fish->RegisterCoverPoint(Pivot->GetComponentTransform().TransformPosition(FVector(-300, 0, -60)));
		Fish->RegisterCoverPoint(Pivot->GetComponentTransform().TransformPosition(FVector(300, 0, -60)));
		Fish->RegisterCoverPoint(Pivot->GetComponentTransform().TransformPosition(FVector(800, 0, -40)));
	}
}

void ASpearfishRegionBuilder::BuildCave(const FSpearfishPOI& POI)
{
	using namespace SpearfishRegionPrivate;
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	const FSpearfishRegionDef* Region = GetRegionDef();
	USpearfishFishSubsystem* Fish = USpearfishFishSubsystem::Get(this);
	if (!Ocean || !Region)
	{
		return;
	}
	const FSpearfishTerrain& Terrain = Ocean->GetTerrain();
	FRandomStream Rng(Seed + static_cast<int32>(POI.Location.X));
	const FVector Forward = FRotator(0.f, POI.Yaw, 0.f).Vector();
	const FVector Right = FRotator(0.f, POI.Yaw + 90.f, 0.f).Vector();
	const int32 Sphere = static_cast<int32>(ESpearfishShape::Sphere);
	const int32 Cube = static_cast<int32>(ESpearfishShape::Cube);
	const FLinearColor RockColor = Region->Palette.Rock * 0.8f;

	// A tunnel: rock walls either side and a slab roof, open at both ends.
	const int32 Segments = 7;
	for (int32 Segment = 0; Segment < Segments; ++Segment)
	{
		const float Along = (Segment - Segments / 2) * 230.f;
		const FVector Center = POI.Location + Forward * Along;
		const float FloorZ = Terrain.GetSeabedZ(static_cast<float>(Center.X), static_cast<float>(Center.Y));
		for (const float Side : { -1.f, 1.f })
		{
			const FVector WallBase = Center + Right * (Side * 260.0);
			const float Size = Rng.FRandRange(260.f, 340.f);
			GetInstancer(Sphere, RockColor, FullCollision)->AddInstance(FTransform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f),
				FVector(WallBase.X, WallBase.Y, FloorZ + 120.f), FVector(Size, Size * 0.8f, 320.f) / 100.0), true);
		}
		GetInstancer(Cube, RockColor, FullCollision)->AddInstance(FTransform(FRotator(Rng.FRandRange(-6.f, 6.f), POI.Yaw, Rng.FRandRange(-6.f, 6.f)),
			FVector(Center.X, Center.Y, FloorZ + 330.f), FVector(260.f, 720.f, 90.f) / 100.0), true);
		GetInstancer(Sphere, RockColor * 1.1f, FullCollision)->AddInstance(FTransform(FRotator::ZeroRotator,
			FVector(Center.X, Center.Y, FloorZ + 420.f), FVector(380.f, 680.f, 160.f) / 100.0), true);
		if (Fish)
		{
			Fish->RegisterCoverPoint(FVector(Center.X, Center.Y, FloorZ + 120.f));
		}
	}
}

void ASpearfishRegionBuilder::BuildIsland()
{
	using namespace SpearfishRegionPrivate;
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	USpearfishVisualSubsystem* Visuals = USpearfishVisualSubsystem::Get(this);
	if (!Ocean || !Visuals)
	{
		return;
	}
	const FSpearfishTerrain& Terrain = Ocean->GetTerrain();
	const FSpearfishTerrainLayout& Layout = Terrain.GetLayout();
	FRandomStream Rng(Seed * 3 + 11);
	const int32 Cylinder = static_cast<int32>(ESpearfishShape::Cylinder);
	const int32 Cube = static_cast<int32>(ESpearfishShape::Cube);
	const FLinearColor Trunk(0.45f, 0.33f, 0.2f);
	const FLinearColor Leaf(0.2f, 0.55f, 0.18f);

	// Palm trees.
	for (int32 Tree = 0; Tree < 14; ++Tree)
	{
		const float Angle = Rng.FRandRange(0.f, 360.f);
		const float Distance = Rng.FRandRange(0.f, Layout.IslandRadiusCm * 0.85f);
		const FVector2D XY = Layout.IslandCenter + FVector2D(FMath::Cos(FMath::DegreesToRadians(Angle)), FMath::Sin(FMath::DegreesToRadians(Angle))) * static_cast<double>(Distance);
		const float GroundZ = Terrain.GetSeabedZ(static_cast<float>(XY.X), static_cast<float>(XY.Y));
		if (GroundZ < 40.f)
		{
			continue;
		}
		const float Height = Rng.FRandRange(450.f, 750.f);
		const float Lean = Rng.FRandRange(-12.f, 12.f);
		const FVector Base(XY.X, XY.Y, GroundZ);
		const FRotator TrunkRotation(Lean, Rng.FRandRange(0.f, 360.f), 0.f);
		GetInstancer(Cylinder, Trunk, FullCollision)->AddInstance(FTransform(TrunkRotation, Base + TrunkRotation.RotateVector(FVector(0, 0, Height * 0.5f)), FVector(26.f, 26.f, Height) / 100.0), true);
		const FVector Crown = Base + TrunkRotation.RotateVector(FVector(0, 0, Height));
		for (int32 Frond = 0; Frond < 7; ++Frond)
		{
			const FRotator FrondRotation(-25.f, Frond * (360.f / 7.f), 0.f);
			GetInstancer(Cube, Leaf, NoCollision)->AddInstance(FTransform(FrondRotation, Crown + FrondRotation.RotateVector(FVector(160.f, 0, 0)), FVector(320.f, 70.f, 4.f) / 100.0), true);
		}
	}

	// The dock: planks on posts from the shore out over the water.
	const FRotator DockRotation(0.f, Layout.DockYaw, 0.f);
	const FVector DockForward = DockRotation.Vector();
	const FLinearColor DockWood(0.5f, 0.37f, 0.25f);
	const float DeckZ = 90.f;
	for (float Along = 0.f; Along <= Layout.DockLengthCm; Along += 200.f)
	{
		const FVector Center = Layout.DockLocation + DockForward * Along;
		Visuals->AddPart(this, Root, ESpearfishShape::Cube, FVector(Center.X, Center.Y, DeckZ), DockRotation, FVector(205.f, 320.f, 16.f), DockWood, true);
		for (const float Side : { -140.f, 140.f })
		{
			const FVector Post = Center + FRotator(0.f, Layout.DockYaw + 90.f, 0.f).Vector() * Side;
			const float GroundZ = Terrain.GetSeabedZ(static_cast<float>(Post.X), static_cast<float>(Post.Y));
			const float PostHeight = FMath::Max(DeckZ - GroundZ, 50.f);
			Visuals->AddPart(this, Root, ESpearfishShape::Cylinder, FVector(Post.X, Post.Y, GroundZ + PostHeight * 0.5f), FRotator::ZeroRotator, FVector(22.f, 22.f, PostHeight), DockWood * 0.8f, true);
		}
	}

	// A beach shack next to the dock.
	const FVector Shack = Layout.DockLocation - DockForward * 500.0 + FRotator(0.f, Layout.DockYaw + 90.f, 0.f).Vector() * 450.0;
	const float ShackZ = Terrain.GetSeabedZ(static_cast<float>(Shack.X), static_cast<float>(Shack.Y));
	Visuals->AddPart(this, Root, ESpearfishShape::Cube, FVector(Shack.X, Shack.Y, ShackZ + 150.f), DockRotation, FVector(500.f, 400.f, 300.f), FLinearColor(0.9f, 0.85f, 0.7f), true);
	Visuals->AddPart(this, Root, ESpearfishShape::Cone, FVector(Shack.X, Shack.Y, ShackZ + 400.f), DockRotation, FVector(650.f, 550.f, 220.f), FLinearColor(0.75f, 0.6f, 0.3f), true);
}

void ASpearfishRegionBuilder::SpawnLocalActors()
{
	UWorld* World = GetWorld();
	const FSpearfishRegionDef* Region = GetRegionDef();
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!World || !Region || !Ocean)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!ASpearfishSkyController::Get(this))
	{
		LocalActors.Add(World->SpawnActor<ASpearfishSkyController>(ASpearfishSkyController::StaticClass(), FTransform::Identity, Params));
	}
	ASpearfishOceanSurface* Surface = World->SpawnActor<ASpearfishOceanSurface>(ASpearfishOceanSurface::StaticClass(), FTransform::Identity, Params);
	if (Surface)
	{
		Surface->ApplyPalette(Region->Palette.ShallowWater, Region->Palette.DeepWater);
		LocalActors.Add(Surface);
	}

	// Decorative fish schools (local only, no network cost) around the reef heads and rocks.
	FRandomStream Rng(Seed * 5 + 9);
	const TArray<FLinearColor> Colors = { FLinearColor(0.2f, 0.45f, 0.95f), FLinearColor(0.95f, 0.85f, 0.2f), FLinearColor(0.85f, 0.85f, 0.9f), FLinearColor(0.95f, 0.45f, 0.2f) };
	int32 Spawned = 0;
	for (const FSpearfishPOI& POI : Ocean->GetTerrain().GetLayout().POIs)
	{
		if (POI.Type != ESpearfishPOIType::ReefHead && POI.Type != ESpearfishPOIType::ClamBed && POI.Type != ESpearfishPOIType::Wreck)
		{
			continue;
		}
		const FVector Home = POI.Location + FVector(0.0, 0.0, 250.0);
		ASpearfishAmbientSchool* School = World->SpawnActor<ASpearfishAmbientSchool>(ASpearfishAmbientSchool::StaticClass(), FTransform(Home), Params);
		if (School)
		{
			School->Configure(Colors[Rng.RandRange(0, Colors.Num() - 1)], Rng.RandRange(25, 55), Rng.FRandRange(6.f, 12.f), Rng.RandRange(1, 99999));
			LocalActors.Add(School);
			++Spawned;
		}
	}
}

// ---------------------------------------------------------------------------- Daily content

void ASpearfishRegionBuilder::SpawnHubStations()
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!HasAuthority() || HubActors.Num() > 0 || !Ocean)
	{
		return;
	}
	const FSpearfishTerrainLayout& Layout = Ocean->GetTerrain().GetLayout();
	const FRotator DockRotation(0.f, Layout.DockYaw, 0.f);
	const FVector Side = FRotator(0.f, Layout.DockYaw + 90.f, 0.f).Vector();

	auto Spawn = [this](ESpearfishStationType Type, const FVector& Location, const FRotator& Rotation)
	{
		const FTransform Transform(Rotation, Location);
		ASpearfishStation* Station = GetWorld()->SpawnActorDeferred<ASpearfishStation>(ASpearfishStation::StaticClass(), Transform, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Station)
		{
			Station->Configure(Type, 0, nullptr);
			Station->FinishSpawning(Transform);
			HubActors.Add(Station);
		}
	};
	const FVector ShopLocation = Layout.DockLocation - DockRotation.Vector() * 150.0 - Side * 380.0;
	const FVector BoardLocation = Layout.DockLocation - DockRotation.Vector() * 150.0 + Side * 260.0;
	const float ShopZ = Ocean->GetTerrain().GetSeabedZ(static_cast<float>(ShopLocation.X), static_cast<float>(ShopLocation.Y));
	const float BoardZ = Ocean->GetTerrain().GetSeabedZ(static_cast<float>(BoardLocation.X), static_cast<float>(BoardLocation.Y));
	Spawn(ESpearfishStationType::ShopKiosk, FVector(ShopLocation.X, ShopLocation.Y, ShopZ), DockRotation + FRotator(0.f, 90.f, 0.f));
	Spawn(ESpearfishStationType::JournalBoard, FVector(BoardLocation.X, BoardLocation.Y, BoardZ), DockRotation + FRotator(0.f, -90.f, 0.f));
}

void ASpearfishRegionBuilder::ClearDailyContent()
{
	for (AActor* Actor : DailyActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	DailyActors.Reset();
}

FVector ASpearfishRegionBuilder::GetHabitatLocation(FName Biome, int32 InSeed) const
{
	FRandomStream Rng(InSeed);
	TArray<const FSpearfishHabitatPoint*> Matches;
	for (const FSpearfishHabitatPoint& Point : Habitat)
	{
		if (Point.Biome == Biome)
		{
			Matches.Add(&Point);
		}
	}
	if (Matches.Num() == 0)
	{
		return Habitat.Num() > 0 ? Habitat[Rng.RandRange(0, Habitat.Num() - 1)].Location : FVector::ZeroVector;
	}
	return Matches[Rng.RandRange(0, Matches.Num() - 1)]->Location;
}

void ASpearfishRegionBuilder::SpawnDailyContent(int32 Day, int32 DaySeed)
{
	if (!HasAuthority())
	{
		return;
	}
	BuildStatic();
	SpawnHubStations();
	ClearDailyContent();

	const FSpearfishRegionDef* Region = GetRegionDef();
	const USpearfishDataRegistry* Registry = USpearfishDataRegistry::Get(this);
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!Region || !Registry || !Ocean)
	{
		return;
	}
	FRandomStream Rng(DaySeed);
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Loose loot, different every morning so routes change.
	float TotalWeight = 0.f;
	for (const FSpearfishWeightedId& Entry : Region->Loot)
	{
		TotalWeight += Entry.Weight;
	}
	const int32 LootCount = USpearfishSettings::Get()->DailyLootPickups;
	for (int32 Index = 0; Index < LootCount && TotalWeight > 0.f; ++Index)
	{
		float Roll = Rng.FRandRange(0.f, TotalWeight);
		FName LootId = Region->Loot.Last().Id;
		for (const FSpearfishWeightedId& Entry : Region->Loot)
		{
			Roll -= Entry.Weight;
			if (Roll <= 0.f)
			{
				LootId = Entry.Id;
				break;
			}
		}
		const FSpearfishLootDef* Loot = Registry->FindLoot(LootId);
		if (!Loot)
		{
			continue;
		}
		// Find a habitat point matching the loot's biomes and depth.
		for (int32 Attempt = 0; Attempt < 30; ++Attempt)
		{
			const FSpearfishHabitatPoint& Point = Habitat[Rng.RandRange(0, Habitat.Num() - 1)];
			if (Point.Biome == FName(TEXT("OpenWater")) || Point.DepthM < Loot->MinDepthM || Point.DepthM > Loot->MaxDepthM)
			{
				continue;
			}
			if (Loot->Biomes.Num() > 0 && !Loot->Biomes.Contains(Point.Biome) && Attempt < 25)
			{
				continue;
			}
			const FTransform Transform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), Point.Location + FVector(0.0, 0.0, 12.0));
			ASpearfishLootPickup* Pickup = World->SpawnActorDeferred<ASpearfishLootPickup>(ASpearfishLootPickup::StaticClass(), Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Pickup)
			{
				Pickup->SetLoot(LootId, Rng.FRandRange(0.6f, 1.f));
				Pickup->FinishSpawning(Transform);
				DailyActors.Add(Pickup);
			}
			break;
		}
	}

	// Giant clams at the clam beds.
	for (const FSpearfishPOI& POI : Ocean->GetTerrain().GetLayout().POIs)
	{
		if (POI.Type == ESpearfishPOIType::ClamBed)
		{
			const FTransform Transform(FRotator(0.f, POI.Yaw, 0.f), POI.Location);
			if (ASpearfishGiantClam* Clam = World->SpawnActor<ASpearfishGiantClam>(ASpearfishGiantClam::StaticClass(), Transform, Params))
			{
				DailyActors.Add(Clam);
			}
		}
		else if (POI.Type == ESpearfishPOIType::Wreck)
		{
			// Salvage crates scattered around the wreck - pull them loose with the speargun.
			for (int32 Crate = 0; Crate < 3; ++Crate)
			{
				const FVector Offset(Rng.FRandRange(-700.f, 700.f), Rng.FRandRange(-700.f, 700.f), 0.0);
				const FVector Location = POI.Location + Offset;
				const float GroundZ = Ocean->GetTerrain().GetSeabedZ(static_cast<float>(Location.X), static_cast<float>(Location.Y));
				const FTransform Transform(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f), FVector(Location.X, Location.Y, GroundZ + 60.f));
				ASpearfishPullable* Prop = World->SpawnActorDeferred<ASpearfishPullable>(ASpearfishPullable::StaticClass(), Transform, nullptr, nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
				if (Prop)
				{
					Prop->ConfigureContents(ESpearfishPullableKind::Crate, TEXT("BrassPorthole"), Rng.RandRange(1, 2));
					Prop->FinishSpawning(Transform);
					DailyActors.Add(Prop);
				}
			}
		}
	}
}

void ASpearfishRegionBuilder::SpawnTreasureChest(FName LootId, int32 Count, int32 DaySeed)
{
	const USpearfishOceanSubsystem* Ocean = USpearfishOceanSubsystem::Get(this);
	if (!HasAuthority() || !Ocean)
	{
		return;
	}
	const FVector Location = GetHabitatLocation(FName(TEXT("Cave")), DaySeed) + FVector(0.0, 0.0, 50.0);
	const FTransform Transform(FRotator::ZeroRotator, Location);
	ASpearfishPullable* Chest = GetWorld()->SpawnActorDeferred<ASpearfishPullable>(ASpearfishPullable::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Chest)
	{
		Chest->ConfigureContents(ESpearfishPullableKind::Chest, LootId, Count);
		Chest->FinishSpawning(Transform);
		DailyActors.Add(Chest);
	}
}

#include "World/SpearfishTerrain.h"

#include "Rules/SpearfishRulesTypes.h"

namespace SpearfishTerrainPrivate
{
	inline float Smooth01(float X)
	{
		const float C = FMath::Clamp(X, 0.f, 1.f);
		return C * C * (3.f - 2.f * C);
	}

	inline float Dist2D(const FVector2D& A, const FVector2D& B)
	{
		return static_cast<float>(FVector2D::Distance(A, B));
	}
}

void FSpearfishTerrain::Initialize(const FSpearfishTerrainParams& InParams, int32 InSeed)
{
	using namespace SpearfishTerrainPrivate;

	Params = InParams;
	Params.HalfExtentCm = FMath::Max(Params.HalfExtentCm, 3000.f);
	Params.ShallowDepthM = FMath::Max(Params.ShallowDepthM, 1.5f);
	Params.MaxDepthM = FMath::Max(Params.MaxDepthM, Params.ShallowDepthM + 5.f);
	Seed = InSeed;

	FRandomStream Rng(Seed);
	const float H = Params.HalfExtentCm;
	Layout = FSpearfishTerrainLayout();
	Layout.IslandCenter = (FVector2D(-0.62, -0.62) + SpearfishRandom::Vector2D(Rng, -0.05f, 0.05f)) * static_cast<double>(H);
	Layout.IslandRadiusCm = 0.22f * H;
	Layout.IslandPeakCm = 650.f + Rng.FRandRange(0.f, 250.f);
	Layout.BeachWidthCm = 0.12f * H;
	Layout.ReefDirection = (FVector2D(0.7 * H, 0.7 * H) - Layout.IslandCenter).GetSafeNormal();

	const float FarDistance = Dist2D(FVector2D(H, H), Layout.IslandCenter) - Layout.IslandRadiusCm - Layout.BeachWidthCm;
	FarDistanceCm = FMath::Max(FarDistance, 1000.f);

	bInitialized = true;
	PlanLayout();
}

float FSpearfishTerrain::DepthProfile(float T)
{
	using namespace SpearfishTerrainPrivate;
	if (T < 0.42f)
	{
		return 0.32f * FMath::Pow(T / 0.42f, 1.2f);
	}
	if (T < 0.58f)
	{
		return 0.32f + 0.43f * Smooth01((T - 0.42f) / 0.16f);
	}
	return 0.75f + 0.25f * FMath::Clamp((T - 0.58f) / 0.42f, 0.f, 1.f);
}

float FSpearfishTerrain::BaseHeight(float X, float Y) const
{
	using namespace SpearfishTerrainPrivate;

	const FVector2D Point(X, Y);
	const float Radius = Layout.IslandRadiusCm;
	const float Beach = Layout.BeachWidthCm;
	const float Distance = Dist2D(Point, Layout.IslandCenter);

	float Z = 0.f;
	if (Distance < Radius)
	{
		const float Land = Layout.IslandPeakCm * FMath::Pow(1.f - Distance / Radius, 1.3f);
		const float Bumps = (FBM(X * 0.002f, Y * 0.002f, Seed + 5, 3) - 0.5f) * 120.f * (1.f - Distance / Radius);
		Z = FMath::Max(25.f, Land + Bumps);
	}
	else if (Distance < Radius + Beach)
	{
		const float Alpha = Smooth01((Distance - Radius) / Beach);
		Z = FMath::Lerp(25.f, -Params.ShallowDepthM * 100.f, Alpha);
	}
	else
	{
		const float T = FMath::Clamp((Distance - Radius - Beach) / FarDistanceCm, 0.f, 1.f);
		float DepthM = Params.ShallowDepthM + (Params.MaxDepthM - Params.ShallowDepthM) * DepthProfile(T);
		DepthM += (FBM(X * 0.00025f, Y * 0.00025f, Seed + 7, 3) - 0.5f) * 3.f * (1.f - 0.5f * T);

		// Reef ridges in the sunlit band.
		const float ReefFade = Smooth01((DepthM - 1.5f) / 2.f) * (1.f - Smooth01((DepthM - 22.f) / 4.f));
		const float Ridge = Ridged(X * 0.0011f, Y * 0.0011f, Seed + 13, 3);
		const float Bump = Params.ReefDensity * 3.2f * Ridge * Ridge * ReefFade;
		Z = FMath::Min(-(DepthM - Bump) * 100.f, -80.f);
	}

	// Beyond the playable square the seabed falls away into the blue.
	const float Edge = FMath::Max(FMath::Abs(X), FMath::Abs(Y)) - Params.HalfExtentCm;
	if (Edge > 0.f)
	{
		Z = FMath::Lerp(Z, -(Params.MaxDepthM + 15.f) * 100.f, Smooth01(Edge / 3000.f));
	}
	return Z;
}

float FSpearfishTerrain::GetSeabedZ(float X, float Y) const
{
	using namespace SpearfishTerrainPrivate;
	if (!bInitialized)
	{
		return -1000.f;
	}

	float Z = BaseHeight(X, Y);
	const FVector2D Point(X, Y);
	for (const FSpearfishPOI& POI : Layout.POIs)
	{
		const float Distance = Dist2D(Point, FVector2D(POI.Location.X, POI.Location.Y));
		switch (POI.Type)
		{
		case ESpearfishPOIType::Wreck:
		{
			// Flatten a bed for the hull to rest on.
			const float Weight = 1.f - Smooth01((Distance - POI.RadiusCm * 0.7f) / (POI.RadiusCm * 0.6f));
			Z = FMath::Lerp(Z, static_cast<float>(POI.Location.Z), Weight);
			break;
		}
		case ESpearfishPOIType::Cave:
		{
			const float Weight = 1.f - Smooth01(Distance / POI.RadiusCm);
			Z -= 80.f * Weight;
			break;
		}
		case ESpearfishPOIType::ReefHead:
		{
			const float Weight = 1.f - Smooth01(Distance / POI.RadiusCm);
			Z += 180.f * Weight * Weight;
			break;
		}
		case ESpearfishPOIType::Trench:
		{
			const float Weight = 1.f - Smooth01(Distance / POI.RadiusCm);
			Z -= 600.f * Weight;
			break;
		}
		default:
			break;
		}
	}
	return Z;
}

float FSpearfishTerrain::GetWaterDepthM(float X, float Y, float SeaLevelZ) const
{
	return FMath::Max(0.f, (SeaLevelZ - GetSeabedZ(X, Y)) / 100.f);
}

FVector FSpearfishTerrain::GetNormal(float X, float Y) const
{
	const float Step = 100.f;
	const float DzDx = (GetSeabedZ(X + Step, Y) - GetSeabedZ(X - Step, Y)) / (2.f * Step);
	const float DzDy = (GetSeabedZ(X, Y + Step) - GetSeabedZ(X, Y - Step)) / (2.f * Step);
	return FVector(-DzDx, -DzDy, 1.0).GetSafeNormal();
}

FName FSpearfishTerrain::GetBiome(float X, float Y) const
{
	static const FName Land(TEXT("Land"));
	static const FName Sand(TEXT("Sand"));
	static const FName Reef(TEXT("Reef"));
	static const FName Seagrass(TEXT("Seagrass"));
	static const FName Kelp(TEXT("Kelp"));
	static const FName Wall(TEXT("Wall"));
	static const FName Wreck(TEXT("Wreck"));
	static const FName Cave(TEXT("Cave"));

	const float Z = GetSeabedZ(X, Y);
	if (Z > 0.f)
	{
		return Land;
	}

	const FVector2D Point(X, Y);
	for (const FSpearfishPOI& POI : Layout.POIs)
	{
		const float Distance = static_cast<float>(FVector2D::Distance(Point, FVector2D(POI.Location.X, POI.Location.Y)));
		if (POI.Type == ESpearfishPOIType::Wreck && Distance < POI.RadiusCm)
		{
			return Wreck;
		}
		if (POI.Type == ESpearfishPOIType::Cave && Distance < POI.RadiusCm)
		{
			return Cave;
		}
	}

	const float DepthM = -Z / 100.f;
	if (DepthM < 1.5f)
	{
		return Sand;
	}
	if (DepthM > 15.f && GetNormal(X, Y).Z < 0.82)
	{
		return Wall;
	}
	if (Params.KelpDensity > 0.f && DepthM > 3.f && DepthM < 32.f && FBM(X * 0.0009f, Y * 0.0009f, Seed + 11, 3) > 1.f - Params.KelpDensity * 0.6f)
	{
		return Kelp;
	}
	if (DepthM > 2.f && DepthM < 24.f && FBM(X * 0.0011f, Y * 0.0011f, Seed + 21, 3) > 1.f - Params.ReefDensity * 0.75f)
	{
		return Reef;
	}
	if (DepthM < 14.f && FBM(X * 0.0016f, Y * 0.0016f, Seed + 31, 2) > 1.f - Params.SeagrassDensity * 0.7f)
	{
		return Seagrass;
	}
	return Sand;
}

bool FSpearfishTerrain::IsInsideBounds(float X, float Y, float Margin) const
{
	return FMath::Abs(X) <= Params.HalfExtentCm - Margin && FMath::Abs(Y) <= Params.HalfExtentCm - Margin;
}

TArray<FSpearfishHabitatPoint> FSpearfishTerrain::SampleHabitat(float SpacingCm, float SeaLevelZ) const
{
	static const FName OpenWater(TEXT("OpenWater"));
	TArray<FSpearfishHabitatPoint> Points;
	if (!bInitialized || SpacingCm < 100.f)
	{
		return Points;
	}
	const float Range = Params.HalfExtentCm - 300.f;
	FRandomStream Rng(Seed * 7 + 3);
	for (float X = -Range; X <= Range; X += SpacingCm)
	{
		for (float Y = -Range; Y <= Range; Y += SpacingCm)
		{
			const float JX = X + Rng.FRandRange(-SpacingCm * 0.4f, SpacingCm * 0.4f);
			const float JY = Y + Rng.FRandRange(-SpacingCm * 0.4f, SpacingCm * 0.4f);
			const float Z = GetSeabedZ(JX, JY);
			const float DepthM = (SeaLevelZ - Z) / 100.f;
			if (DepthM < 1.f)
			{
				continue;
			}
			FSpearfishHabitatPoint Point;
			Point.Location = FVector(JX, JY, Z);
			Point.Biome = GetBiome(JX, JY);
			Point.DepthM = DepthM;
			Points.Add(Point);

			// Mid-water point above deeper ground for pelagic species.
			if (DepthM > 6.f)
			{
				FSpearfishHabitatPoint Water;
				const float MidZ = FMath::Lerp(Z + 200.f, SeaLevelZ - 200.f, Rng.FRandRange(0.25f, 0.75f));
				Water.Location = FVector(JX, JY, MidZ);
				Water.Biome = OpenWater;
				Water.DepthM = (SeaLevelZ - MidZ) / 100.f;
				Points.Add(Water);
			}
		}
	}
	// Points of interest always contribute samples so caves and wrecks get residents.
	for (const FSpearfishPOI& POI : Layout.POIs)
	{
		FSpearfishHabitatPoint Point;
		Point.Location = POI.Location;
		Point.Biome = GetBiome(static_cast<float>(POI.Location.X), static_cast<float>(POI.Location.Y));
		Point.DepthM = (SeaLevelZ - static_cast<float>(POI.Location.Z)) / 100.f;
		Points.Add(Point);
	}
	return Points;
}

bool FSpearfishTerrain::FindPOISpot(FRandomStream& Rng, float MinDepthM, float MaxDepthM, float Spacing, float MaxReliefCm, float ReliefRadius, FVector& OutLocation) const
{
	const float Range = Params.HalfExtentCm * 0.88f;
	const FVector2D Boat(Layout.BoatStart.X, Layout.BoatStart.Y);
	for (int32 Attempt = 0; Attempt < 400; ++Attempt)
	{
		const float X = Rng.FRandRange(-Range, Range);
		const float Y = Rng.FRandRange(-Range, Range);
		const float Z = BaseHeight(X, Y);
		const float DepthM = -Z / 100.f;
		if (DepthM < MinDepthM || DepthM > MaxDepthM)
		{
			continue;
		}
		const FVector2D Candidate(X, Y);
		if (FVector2D::Distance(Candidate, Boat) < 2200.0)
		{
			continue;
		}
		if (MaxReliefCm > 0.f)
		{
			// Reject steep ground: sample a ring around the spot.
			float Relief = 0.f;
			for (int32 Sample = 0; Sample < 8; ++Sample)
			{
				const float Angle = static_cast<float>(Sample) * UE_TWO_PI / 8.f;
				const float SampleZ = BaseHeight(X + FMath::Cos(Angle) * ReliefRadius, Y + FMath::Sin(Angle) * ReliefRadius);
				Relief = FMath::Max(Relief, FMath::Abs(SampleZ - Z));
			}
			if (Relief > MaxReliefCm)
			{
				continue;
			}
		}
		bool bTooClose = false;
		for (const FSpearfishPOI& Existing : Layout.POIs)
		{
			if (FVector2D::Distance(Candidate, FVector2D(Existing.Location.X, Existing.Location.Y)) < static_cast<double>(Spacing))
			{
				bTooClose = true;
				break;
			}
		}
		if (!bTooClose)
		{
			OutLocation = FVector(X, Y, Z);
			return true;
		}
	}
	return false;
}

void FSpearfishTerrain::PlanLayout()
{
	FRandomStream Rng(Seed * 31 + 7);
	const FVector2D Dir = Layout.ReefDirection;
	const FVector2D Side(-Dir.Y, Dir.X);
	const float Radius = Layout.IslandRadiusCm;
	const float Beach = Layout.BeachWidthCm;

	const FVector2D DockShore = Layout.IslandCenter + Dir * static_cast<double>(Radius - 150.f);
	Layout.DockLocation = FVector(DockShore.X, DockShore.Y, BaseHeight(static_cast<float>(DockShore.X), static_cast<float>(DockShore.Y)));
	Layout.DockYaw = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Dir.Y), static_cast<float>(Dir.X)));
	Layout.DockLengthCm = Beach + 450.f;

	const FVector2D BoatXY = Layout.IslandCenter + Dir * static_cast<double>(Radius + Beach + 500.f) + Side * 480.0;
	Layout.BoatStart = FVector(BoatXY.X, BoatXY.Y, 0.0);
	Layout.BoatStartYaw = Layout.DockYaw;

	auto AddPOIs = [this, &Rng](ESpearfishPOIType Type, int32 Count, float MinDepth, float MaxDepth, float Spacing, float POIRadius, float MaxRelief)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FVector Location;
			if (FindPOISpot(Rng, MinDepth, MaxDepth, Spacing, MaxRelief, POIRadius * 1.3f, Location))
			{
				FSpearfishPOI POI;
				POI.Type = Type;
				POI.Location = Location;
				POI.Yaw = Rng.FRandRange(0.f, 360.f);
				POI.RadiusCm = POIRadius;
				Layout.POIs.Add(POI);
			}
		}
	};

	// Wrecks need a flat bed: try the shelf edge first, then the deep flats beyond the drop-off.
	const int32 PlacedBefore = Layout.POIs.Num();
	AddPOIs(ESpearfishPOIType::Wreck, Params.Wrecks, 14.f, 24.f, 3200.f, 1100.f, 500.f);
	const int32 MissingWrecks = Params.Wrecks - (Layout.POIs.Num() - PlacedBefore);
	AddPOIs(ESpearfishPOIType::Wreck, MissingWrecks, 24.f, 40.f, 3200.f, 1100.f, 350.f);
	AddPOIs(ESpearfishPOIType::Cave, Params.Caves, 8.f, 20.f, 2600.f, 700.f, 0.f);
	AddPOIs(ESpearfishPOIType::ClamBed, Params.GiantClams, 5.f, 24.f, 1200.f, 200.f, 0.f);
	AddPOIs(ESpearfishPOIType::ReefHead, 7, 3.f, 14.f, 1400.f, 450.f, 0.f);

	FSpearfishPOI Trench;
	Trench.Type = ESpearfishPOIType::Trench;
	const float TrenchXY = Params.HalfExtentCm * 0.72f;
	Trench.Location = FVector(TrenchXY, TrenchXY, BaseHeight(TrenchXY, TrenchXY));
	Trench.RadiusCm = 2500.f;
	Layout.POIs.Add(Trench);
}

float FSpearfishTerrain::Hash2D(int32 X, int32 Y, int32 InSeed)
{
	uint32 Hash = static_cast<uint32>(X) * 0x8da6b343u;
	Hash ^= static_cast<uint32>(Y) * 0xd8163841u;
	Hash ^= static_cast<uint32>(InSeed) * 0xcb1ab31fu;
	Hash ^= Hash >> 13;
	Hash *= 0x5bd1e995u;
	Hash ^= Hash >> 15;
	return static_cast<float>(Hash & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

float FSpearfishTerrain::ValueNoise(float X, float Y, int32 InSeed)
{
	const int32 X0 = FMath::FloorToInt(X);
	const int32 Y0 = FMath::FloorToInt(Y);
	const float FX = X - static_cast<float>(X0);
	const float FY = Y - static_cast<float>(Y0);
	const float SX = FX * FX * (3.f - 2.f * FX);
	const float SY = FY * FY * (3.f - 2.f * FY);
	const float A = Hash2D(X0, Y0, InSeed);
	const float B = Hash2D(X0 + 1, Y0, InSeed);
	const float C = Hash2D(X0, Y0 + 1, InSeed);
	const float D = Hash2D(X0 + 1, Y0 + 1, InSeed);
	return FMath::Lerp(FMath::Lerp(A, B, SX), FMath::Lerp(C, D, SX), SY);
}

float FSpearfishTerrain::FBM(float X, float Y, int32 InSeed, int32 Octaves)
{
	float Sum = 0.f;
	float Amplitude = 0.5f;
	float Frequency = 1.f;
	float Norm = 0.f;
	for (int32 Octave = 0; Octave < FMath::Max(Octaves, 1); ++Octave)
	{
		Sum += Amplitude * ValueNoise(X * Frequency, Y * Frequency, InSeed + Octave * 101);
		Norm += Amplitude;
		Amplitude *= 0.5f;
		Frequency *= 2.03f;
	}
	return Norm > 0.f ? Sum / Norm : 0.f;
}

float FSpearfishTerrain::Ridged(float X, float Y, int32 InSeed, int32 Octaves)
{
	float Sum = 0.f;
	float Amplitude = 0.5f;
	float Frequency = 1.f;
	float Norm = 0.f;
	for (int32 Octave = 0; Octave < FMath::Max(Octaves, 1); ++Octave)
	{
		const float N = ValueNoise(X * Frequency, Y * Frequency, InSeed + Octave * 211);
		Sum += Amplitude * (1.f - FMath::Abs(N * 2.f - 1.f));
		Norm += Amplitude;
		Amplitude *= 0.5f;
		Frequency *= 2.11f;
	}
	return Norm > 0.f ? Sum / Norm : 0.f;
}

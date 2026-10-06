#include "AoE/AoEShapeMeshComponent.h"

#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AoEShapeMeshComponent)

const TCHAR* UAoEShapeMeshComponent::TelegraphMaterialPath = TEXT("/Game/BossArena/Materials/M_Telegraph.M_Telegraph");

namespace AoEShapeMeshImpl
{
	constexpr float OutlineThickness = 12.0f;
	constexpr float AreaZ = 3.0f;
	constexpr float OutlineZ = 4.0f;
	constexpr float FillZ = 5.0f;

	struct FBuilder
	{
		TArray<FVector> Vertices;
		TArray<int32> Triangles;

		void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D)
		{
			const int32 Base = Vertices.Add(A);
			Vertices.Add(B);
			Vertices.Add(C);
			Vertices.Add(D);
			// Both windings so the indicator is visible with one-sided fallback materials.
			Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
			Triangles.Append({ Base, Base + 2, Base + 1, Base, Base + 3, Base + 2 });
		}

		void Sector(float InnerRadius, float OuterRadius, float StartDeg, float EndDeg, float Z)
		{
			if (OuterRadius <= InnerRadius + KINDA_SMALL_NUMBER)
			{
				return;
			}
			const int32 Segments = FMath::Clamp(FMath::CeilToInt((EndDeg - StartDeg) / 5.0f), 3, 96);
			for (int32 Index = 0; Index < Segments; ++Index)
			{
				const float A0 = FMath::DegreesToRadians(FMath::Lerp(StartDeg, EndDeg, float(Index) / Segments));
				const float A1 = FMath::DegreesToRadians(FMath::Lerp(StartDeg, EndDeg, float(Index + 1) / Segments));
				const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0.0f);
				const FVector D1(FMath::Cos(A1), FMath::Sin(A1), 0.0f);
				const FVector Up(0.0f, 0.0f, Z);
				Quad(D0 * InnerRadius + Up, D0 * OuterRadius + Up, D1 * OuterRadius + Up, D1 * InnerRadius + Up);
			}
		}

		void Rect(float X0, float X1, float Y0, float Y1, float Z)
		{
			if (X1 <= X0 || Y1 <= Y0)
			{
				return;
			}
			Quad(FVector(X0, Y0, Z), FVector(X1, Y0, Z), FVector(X1, Y1, Z), FVector(X0, Y1, Z));
		}

		void RadialEdge(float AngleDeg, float Radius, float Thickness, float Z)
		{
			const float Angle = FMath::DegreesToRadians(AngleDeg);
			const FVector Dir(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
			const FVector Side(-Dir.Y, Dir.X, 0.0f);
			const FVector Up(0.0f, 0.0f, Z);
			const FVector Half = Side * Thickness * 0.5f;
			Quad(Up - Half, Dir * Radius + Up - Half, Dir * Radius + Up + Half, Up + Half);
		}

		void RectOutline(float X0, float X1, float Y0, float Y1, float T, float Z)
		{
			Rect(X0, X1, Y0, Y0 + T, Z);
			Rect(X0, X1, Y1 - T, Y1, Z);
			Rect(X0, X0 + T, Y0 + T, Y1 - T, Z);
			Rect(X1 - T, X1, Y0 + T, Y1 - T, Z);
		}
	};

	void BuildArea(const FAoEShape& S, FBuilder& B)
	{
		const float HalfW = S.Width * 0.5f;
		switch (S.Shape)
		{
		case EBossAoEShape::Circle:        B.Sector(0.0f, S.Radius, 0.0f, 360.0f, AreaZ); break;
		case EBossAoEShape::Donut:         B.Sector(S.InnerRadius, S.Radius, 0.0f, 360.0f, AreaZ); break;
		case EBossAoEShape::Cone:          B.Sector(0.0f, S.Radius, -S.ConeAngle * 0.5f, S.ConeAngle * 0.5f, AreaZ); break;
		case EBossAoEShape::Line:
		case EBossAoEShape::RotatingSweep: B.Rect(0.0f, S.Length, -HalfW, HalfW, AreaZ); break;
		case EBossAoEShape::Rectangle:     B.Rect(-S.Length * 0.5f, S.Length * 0.5f, -HalfW, HalfW, AreaZ); break;
		}
	}

	void BuildOutline(const FAoEShape& S, FBuilder& B)
	{
		const float T = OutlineThickness;
		const float HalfW = S.Width * 0.5f;
		switch (S.Shape)
		{
		case EBossAoEShape::Circle:
			B.Sector(FMath::Max(0.0f, S.Radius - T), S.Radius, 0.0f, 360.0f, OutlineZ);
			break;
		case EBossAoEShape::Donut:
			B.Sector(FMath::Max(0.0f, S.Radius - T), S.Radius, 0.0f, 360.0f, OutlineZ);
			B.Sector(S.InnerRadius, S.InnerRadius + T, 0.0f, 360.0f, OutlineZ);
			break;
		case EBossAoEShape::Cone:
			B.Sector(FMath::Max(0.0f, S.Radius - T), S.Radius, -S.ConeAngle * 0.5f, S.ConeAngle * 0.5f, OutlineZ);
			B.RadialEdge(-S.ConeAngle * 0.5f, S.Radius, T, OutlineZ);
			B.RadialEdge(S.ConeAngle * 0.5f, S.Radius, T, OutlineZ);
			break;
		case EBossAoEShape::Line:
		case EBossAoEShape::RotatingSweep:
			B.RectOutline(0.0f, S.Length, -HalfW, HalfW, T, OutlineZ);
			break;
		case EBossAoEShape::Rectangle:
			B.RectOutline(-S.Length * 0.5f, S.Length * 0.5f, -HalfW, HalfW, T, OutlineZ);
			break;
		}
	}

	void BuildFill(const FAoEShape& S, float Fill, FBuilder& B)
	{
		if (Fill <= KINDA_SMALL_NUMBER)
		{
			return;
		}
		const float HalfW = S.Width * 0.5f;
		switch (S.Shape)
		{
		case EBossAoEShape::Circle:        B.Sector(0.0f, S.Radius * Fill, 0.0f, 360.0f, FillZ); break;
		case EBossAoEShape::Donut:         B.Sector(S.Radius - (S.Radius - S.InnerRadius) * Fill, S.Radius, 0.0f, 360.0f, FillZ); break;
		case EBossAoEShape::Cone:          B.Sector(0.0f, S.Radius * Fill, -S.ConeAngle * 0.5f, S.ConeAngle * 0.5f, FillZ); break;
		case EBossAoEShape::Line:
		case EBossAoEShape::RotatingSweep: B.Rect(0.0f, S.Length * Fill, -HalfW, HalfW, FillZ); break;
		case EBossAoEShape::Rectangle:     B.Rect(-S.Length * 0.5f, -S.Length * 0.5f + S.Length * Fill, -HalfW, HalfW, FillZ); break;
		}
	}

	bool ShapesEqual(const FAoEShape& A, const FAoEShape& B)
	{
		return A.Shape == B.Shape && A.Radius == B.Radius && A.InnerRadius == B.InnerRadius && A.ConeAngle == B.ConeAngle
			&& A.Length == B.Length && A.Width == B.Width;
	}
}

UAoEShapeMeshComponent::UAoEShapeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetCastShadow(false);
	SetGenerateOverlapEvents(false);
	bUseAsyncCooking = true;
	SetCanEverAffectNavigation(false);
}

void UAoEShapeMeshComponent::EnsureMaterials()
{
	if (SectionMaterials.Num() == 3)
	{
		return;
	}

	UMaterialInterface* Material = TelegraphMaterial;
	if (!Material)
	{
		Material = LoadObject<UMaterialInterface>(nullptr, TelegraphMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	if (!Material)
	{
		bUsesFallbackMaterial = true;
		Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	SectionMaterials.Reset();
	for (int32 Section = 0; Section < 3; ++Section)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, this);
		SectionMaterials.Add(MID);
		SetMaterial(Section, MID);
	}
}

void UAoEShapeMeshComponent::SetShape(const FAoEShape& InShape, float FillFraction)
{
	using namespace AoEShapeMeshImpl;

	EnsureMaterials();
	FillFraction = FMath::Clamp(FillFraction, 0.0f, 1.0f);

	const bool bShapeChanged = !bHasGeometry || !ShapesEqual(InShape, CachedShape);
	if (bShapeChanged)
	{
		FBuilder Area;
		FBuilder Outline;
		BuildArea(InShape, Area);
		BuildOutline(InShape, Outline);
		CreateMeshSection_LinearColor(0, Area.Vertices, Area.Triangles, {}, {}, {}, {}, false);
		CreateMeshSection_LinearColor(1, Outline.Vertices, Outline.Triangles, {}, {}, {}, {}, false);
		CachedShape = InShape;
		bHasGeometry = true;
		CachedFill = -1.0f;
	}

	if (!FMath::IsNearlyEqual(FillFraction, CachedFill, 0.002f))
	{
		FBuilder Fill;
		BuildFill(InShape, FillFraction, Fill);
		if (Fill.Vertices.Num() > 0)
		{
			CreateMeshSection_LinearColor(2, Fill.Vertices, Fill.Triangles, {}, {}, {}, {}, false);
		}
		else
		{
			ClearMeshSection(2);
		}
		CachedFill = FillFraction;
	}

	for (int32 Section = 0; Section < SectionMaterials.Num(); ++Section)
	{
		SetMaterial(Section, SectionMaterials[Section]);
	}
}

void UAoEShapeMeshComponent::SetStyle(const FLinearColor& Color, float AreaOpacity, float OutlineOpacity, float FillOpacity)
{
	EnsureMaterials();
	const float Opacities[3] = { AreaOpacity, OutlineOpacity, FillOpacity };
	for (int32 Section = 0; Section < SectionMaterials.Num(); ++Section)
	{
		if (UMaterialInstanceDynamic* MID = SectionMaterials[Section])
		{
			// The opaque fallback material cannot fade, so darken toward black instead.
			const FLinearColor Tint = bUsesFallbackMaterial ? Color * FMath::Lerp(0.25f, 1.0f, Opacities[Section]) : Color;
			MID->SetVectorParameterValue(TEXT("Color"), Tint);
			MID->SetScalarParameterValue(TEXT("Opacity"), Opacities[Section]);
		}
		SetMeshSectionVisible(Section, Opacities[Section] > 0.0f);
	}
}

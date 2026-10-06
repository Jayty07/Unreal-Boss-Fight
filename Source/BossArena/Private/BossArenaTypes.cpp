#include "BossArenaTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BossArenaTypes)

float FAoEShape::GetBoundingRadius() const
{
	switch (Shape)
	{
	case EBossAoEShape::Line:
	case EBossAoEShape::RotatingSweep:
		return FMath::Sqrt(FMath::Square(Length) + FMath::Square(Width * 0.5f));
	case EBossAoEShape::Rectangle:
		return FMath::Sqrt(FMath::Square(Length * 0.5f) + FMath::Square(Width * 0.5f));
	default:
		return Radius;
	}
}

FAoEShape FAoEShape::Scaled(float Scale) const
{
	FAoEShape Result = *this;
	Result.Radius *= Scale;
	Result.Length *= Scale;
	Result.Width *= Scale;
	Result.InnerRadius = FMath::Min(Result.InnerRadius, Result.Radius * 0.9f);
	return Result;
}

FAoEShape FAoEShape::MakeCircle(float InRadius)
{
	FAoEShape Result;
	Result.Shape = EBossAoEShape::Circle;
	Result.Radius = InRadius;
	return Result;
}

FAoEShape FAoEShape::MakeCone(float InRadius, float InAngle)
{
	FAoEShape Result;
	Result.Shape = EBossAoEShape::Cone;
	Result.Radius = InRadius;
	Result.ConeAngle = InAngle;
	return Result;
}

FAoEShape FAoEShape::MakeLine(float InLength, float InWidth)
{
	FAoEShape Result;
	Result.Shape = EBossAoEShape::Line;
	Result.Length = InLength;
	Result.Width = InWidth;
	return Result;
}

FAoEShape FAoEShape::MakeRectangle(float InLength, float InWidth)
{
	FAoEShape Result;
	Result.Shape = EBossAoEShape::Rectangle;
	Result.Length = InLength;
	Result.Width = InWidth;
	return Result;
}

FAoEShape FBossAoEShapeRow::ToShape() const
{
	FAoEShape Result;
	Result.Shape = Shape;
	Result.Radius = Radius;
	Result.InnerRadius = InnerRadius;
	Result.ConeAngle = ConeAngle;
	Result.Length = Length;
	Result.Width = Width;
	return Result;
}

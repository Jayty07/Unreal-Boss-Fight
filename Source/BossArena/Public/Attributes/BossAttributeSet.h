#pragma once

#include "CoreMinimal.h"
#include "Attributes/BossArenaAttributeSetBase.h"
#include "BossAttributeSet.generated.h"

/** Boss/add attributes: Health (inherited), Damage (outgoing multiplier), EnrageTimer (seconds until hard enrage). */
UCLASS()
class BOSSARENA_API UBossAttributeSet : public UBossArenaAttributeSetBase
{
	GENERATED_BODY()

public:
	UBossAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Damage)
	FGameplayAttributeData Damage;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UBossAttributeSet, Damage)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_EnrageTimer)
	FGameplayAttributeData EnrageTimer;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UBossAttributeSet, EnrageTimer)

protected:
	UFUNCTION()
	void OnRep_Damage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_EnrageTimer(const FGameplayAttributeData& OldValue);
};

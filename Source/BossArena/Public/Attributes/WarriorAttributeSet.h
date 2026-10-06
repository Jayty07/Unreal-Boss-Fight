#pragma once

#include "CoreMinimal.h"
#include "Attributes/BossArenaAttributeSetBase.h"
#include "WarriorAttributeSet.generated.h"

/** Warrior attributes: Health (inherited), Rage, Armor, Strength. */
UCLASS()
class BOSSARENA_API UWarriorAttributeSet : public UBossArenaAttributeSetBase
{
	GENERATED_BODY()

public:
	UWarriorAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Rage)
	FGameplayAttributeData Rage;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UWarriorAttributeSet, Rage)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxRage)
	FGameplayAttributeData MaxRage;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UWarriorAttributeSet, MaxRage)

	/** Mitigation: damage * 100 / (100 + Armor). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UWarriorAttributeSet, Armor)

	/** Outgoing AoE damage scale: damage * (1 + Strength / 100). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Strength)
	FGameplayAttributeData Strength;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UWarriorAttributeSet, Strength)

	/** Fraction of damage removed while blocking. */
	UPROPERTY(EditDefaultsOnly, Category = "Tuning")
	float BlockMitigation = 0.5f;

	/** Rage gained per point of post-mitigation damage taken. */
	UPROPERTY(EditDefaultsOnly, Category = "Tuning")
	float RagePerDamageTaken = 0.01f;

protected:
	virtual float MitigateDamage(float RawDamage, const FGameplayEffectModCallbackData& Data) const override;
	virtual void OnDamageApplied(float FinalDamage, const FGameplayEffectModCallbackData& Data) override;
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const override;

	UFUNCTION()
	void OnRep_Rage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxRage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldValue);
};

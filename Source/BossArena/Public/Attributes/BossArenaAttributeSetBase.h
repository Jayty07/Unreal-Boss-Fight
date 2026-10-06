#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BossArenaAttributeSetBase.generated.h"

#define BOSSARENA_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Shared Health/MaxHealth plus the IncomingDamage meta attribute. All AoE damage in the
 * encounter is applied (server-side) as a GameplayEffect on IncomingDamage, so this is the
 * single choke point for i-frames, mitigation, threat and death.
 */
UCLASS(Abstract)
class BOSSARENA_API UBossArenaAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBossArenaAttributeSetBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UBossArenaAttributeSetBase, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UBossArenaAttributeSetBase, MaxHealth)

	/** Meta attribute (server only, not replicated). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData IncomingDamage;
	BOSSARENA_ATTRIBUTE_ACCESSORS(UBossArenaAttributeSetBase, IncomingDamage)

protected:
	/** Armor/block mitigation hook. Unblockable damage skips it. */
	virtual float MitigateDamage(float RawDamage, const FGameplayEffectModCallbackData& Data) const { return RawDamage; }
	virtual void OnDamageApplied(float FinalDamage, const FGameplayEffectModCallbackData& Data) {}
	virtual void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	static bool IsUnblockable(const FGameplayEffectSpec& Spec);

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NetBattleMenuWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;

UCLASS(Blueprintable)
class MYPOKEMONDEMO_API UNetBattleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> AddressInput;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UButton* AddMenuButton(UVerticalBox* Parent, const FText& Label, FName WidgetName);
	void SetStatus(const FText& Message, const FLinearColor& Color);

	UFUNCTION()
	void HostBattle();

	UFUNCTION()
	void JoinBattle();

	UFUNCTION()
	void OpenSinglePlayerDemo();

	UFUNCTION()
	void ExitGame();
};

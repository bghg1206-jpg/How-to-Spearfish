#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SpearfishHUD.generated.h"

class ASpearfishCharacter;
class ASpearfishGameState;
class ASpearfishPlayerController;
class UFont;
struct FSpearfishMinigameState;
struct FSpearfishUIRow;

/**
 * Canvas HUD for the vertical slice (no widget assets needed). Everything it draws comes from replicated
 * state or the local player controller; it owns no gameplay state. Layout scales with screen height.
 *
 * Diegetic rules: the diver gets an analog air gauge and a depth gauge, never a dive computer or sonar;
 * the boat marker only shows from the surface where the boat is actually visible.
 */
UCLASS()
class HOWTOSPEARFISH_API ASpearfishHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	// Layers
	void DrawStatusBar();
	void DrawRoleBanner();
	void DrawDiverGauges();
	void DrawLineAndGun();
	void DrawCrosshairAndPrompt();
	void DrawFeed();
	void DrawMarkers();
	void DrawKitchenBoard();
	void DrawContextHints();
	void DrawPanel();
	void DrawRows(const TArray<FSpearfishUIRow>& Rows, float X, float Y, float Width, float Height);
	void DrawMinigame();
	void DrawMinigameBody(const FSpearfishMinigameState& Game, float X, float Y, float Width, float Height);
	void DrawSummary(float X, float Y, float Width);
	void DrawHelp(float X, float Y, float Width);
	void DrawFadeAndBlackout();

	// Primitives
	float S(float Pixels) const { return Pixels * UIScale; }
	void Text(const FString& String, float X, float Y, const FLinearColor& Color, UFont* Font, float Scale = 1.f, bool bShadow = true);
	void TextCentered(const FString& String, float CenterX, float Y, const FLinearColor& Color, UFont* Font, float Scale = 1.f);
	void TextRight(const FString& String, float RightX, float Y, const FLinearColor& Color, UFont* Font, float Scale = 1.f);
	FVector2D Measure(const FString& String, UFont* Font, float Scale = 1.f);
	void Box(float X, float Y, float W, float H, const FLinearColor& Color);
	void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness = 1.f);
	void Bar(float X, float Y, float W, float H, float Fraction, const FLinearColor& Fill, const FLinearColor& Back);
	void Marker(const FVector& World, const FString& Label, const FLinearColor& Color);

	ASpearfishPlayerController* GetSpearfishController() const;
	ASpearfishCharacter* GetSpearfishCharacter() const;
	ASpearfishGameState* GetSpearfishGameState() const;

	UFont* LargeFont = nullptr;
	UFont* MediumFont = nullptr;
	UFont* SmallFont = nullptr;
	float UIScale = 1.f;
	float FontScale = 1.f;
	float Time = 0.f;
};

/*
** Copyright (c) 2024 Cognitive3D, Inc. All rights reserved.
*/

#pragma once

#include "C3DCommonTypes.h"
#include "Cognitive3D/Public/Cognitive3D.h"
#include "CoreMinimal.h"
#include "Cognitive3DActor.generated.h"

//this class basically only exists to send EndPlay to the CognitiveProvider
//and as a simple way of getting the right UWorld

class FAnalyticsProviderCognitive3D;

UCLASS()
class COGNITIVE3D_API ACognitive3DActor : public AActor
{
	GENERATED_BODY()

public:	
	
	ACognitive3DActor();

	static ACognitive3DActor* GetCognitive3DActor();
    /**
     * Get the Cognitive3DActor for a specific world
     */
	static ACognitive3DActor* GetCognitive3DActor(UWorld *World);
	static UWorld* GetCognitiveSessionWorld();

	//used only for blueprint events
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Begin Session"))
		void ReceiveBeginSession();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Pre End Session"))
		void ReceivePreEndSession();
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Post End Session"))
		void ReceivePostEndSession();

private:

	static ACognitive3DActor* instance;
	TSharedPtr<FAnalyticsProviderCognitive3D> cog;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};

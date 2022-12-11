/*
** Copyright (c) 2016 CognitiveVR, Inc. All rights reserved.
*/
#pragma once

#include "Analytics.h"
#include "CognitiveVR/Public/CognitiveVRProvider.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "CognitiveVR/Private/Fixations.h"
#include "FixationDataRecorder.generated.h"

class FAnalyticsCognitiveVR;
class FAnalyticsProviderCognitiveVR;
class UCognitiveVRBlueprints;



UCLASS()
	class COGNITIVEVR_API UFixationDataRecorder : public UObject
	{
		GENERATED_BODY()

	private:

		int32 FixationBatchSize = 64;
		int32 AutoTimer = 2;
		int32 MinTimer = 2;
		int32 ExtremeBatchSize = 64;
		float LastSendTime = -60;
		FTimerHandle AutoSendHandle;
		
		TSharedPtr<FAnalyticsProviderCognitiveVR> cog;
		int32 jsonPart = 1;
		int32 CustomEventBatchSize = 16;

		//TODO likely CRASH. getting garbage collected between scenes? can't use UPROPERTY because jsonobject isn't a UCLASS USTRUCT or UENUM
		//TArray<TSharedPtr<FJsonObject>> Fixations;
		UPROPERTY()
			TArray<FFixation> Fixations;

		UFUNCTION()
		void StartSession();
		UFUNCTION()
		void PreSessionEnd();
		UFUNCTION()
		void PostSessionEnd();

	public:
		UFixationDataRecorder();
		//call this immediately after creation - sets callbacks and reference to CognitiveVRProvider
		void Initialize();
		void RecordFixationEnd(const FFixation& data);

		//send all outstanding fixations to Cognitive dashboard
		UFUNCTION()
		void SendData(bool copyDataToCache);

		float GetLastSendTime() { return LastSendTime; }
		int32 GetPartNumber() { return jsonPart; }
		int32 GetDataPoints() { return Fixations.Num(); }
	};
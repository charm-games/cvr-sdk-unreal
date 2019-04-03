// Fill out your copyright notice in the Description page of Project Settings.

#include "CognitiveVR.h"
#include "FixationRecorder.h"


// Sets default values for this component's properties
UFixationRecorder::UFixationRecorder()
{
	PrimaryComponentTick.bCanEverTick = true;

	FString ValueReceived;

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationBatchSize", false);
	if (ValueReceived.Len() > 0)
	{
		int32 fixationLimit = FCString::Atoi(*ValueReceived);
		if (fixationLimit > 0)
		{
			FixationBatchSize = fixationLimit;
		}
	}

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationExtremeLimit", false);
	if (ValueReceived.Len() > 0)
	{
		int32 parsedValue = FCString::Atoi(*ValueReceived);
		if (parsedValue > 0)
		{
			ExtremeBatchSize = parsedValue;
		}
	}

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationMinTimer", false);
	if (ValueReceived.Len() > 0)
	{
		int32 parsedValue = FCString::Atoi(*ValueReceived);
		if (parsedValue > 0)
		{
			MinTimer = parsedValue;
		}
	}

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "FixationAutoTimer", false);
	if (ValueReceived.Len() > 0)
	{
		int32 parsedValue = FCString::Atoi(*ValueReceived);
		if (parsedValue > 0)
		{
			AutoTimer = parsedValue;
			cog->GetWorld()->GetGameInstance()->GetTimerManager().SetTimer(AutoSendHandle, this, &UFixationRecorder::SendData, AutoTimer, false);
		}
	}
}

int32 UFixationRecorder::GetIndex(int32 offset)
{
	if (index + offset < 0)
		return (CachedEyeCaptureCount + index + offset) % CachedEyeCaptureCount;
	return (index + offset) % CachedEyeCaptureCount;
}

void UFixationRecorder::BeginPlay()
{
	world = GetWorld();
	//create a bunch of eye capture data and put into array
	cog = FAnalyticsCognitiveVR::Get().GetCognitiveVRProvider();
	if (cog.IsValid())
	{
		for (int32 i = 0; i < CachedEyeCaptureCount; i++)
		{
			EyeCaptures.Add(FEyeCapture());
		}
	}
	else
	{
		GLog->Log("UFixationRecorder::BeginPlay cannot find CognitiveVRProvider!");
	}
}

bool UFixationRecorder::IsGazeOutOfRange(FEyeCapture eyeCapture)
{
	if (!IsFixating) { return true; }

	if (ActiveFixation.IsLocal)
	{
		if (eyeCapture.SkipPositionForFixationAverage || eyeCapture.OffTransform)
		{
			FVector fixationWorldPosition = ActiveFixation.LocalTransform->GetComponentTransform().TransformPosition(ActiveFixation.LocalPosition);
			FVector fixationDirection = (fixationWorldPosition - eyeCapture.HMDPosition);
			fixationDirection.Normalize();

			FVector eyeCaptureWorldPosition = ActiveFixation.LocalTransform->GetComponentTransform().TransformPosition(eyeCapture.LocalPosition);
			FVector eyeCaptureDirection = eyeCaptureWorldPosition - eyeCapture.HMDPosition;
			eyeCaptureDirection.Normalize();

			float angle = FMath::Cos(FMath::DegreesToRadians(MaxFixationAngle));

			if (FVector::DotProduct(eyeCaptureDirection, fixationDirection) < angle)
			{
				return true;
			}
			return false;
		}
		else
		{
			//combine cached gaze (local) to get fixation

			FVector averageLocalPos = FVector::ZeroVector;
			for (int32 i = 0; i < CachedEyeCapturePositions.Num(); i++)
			{
				averageLocalPos += CachedEyeCapturePositions[i];
			}
			averageLocalPos += eyeCapture.LocalPosition;
			averageLocalPos /= (CachedEyeCapturePositions.Num() + 1);


			FVector fixationWorldPosition = ActiveFixation.LocalTransform->GetComponentTransform().TransformPosition(averageLocalPos);
			FVector fixationDirection = (fixationWorldPosition - eyeCapture.HMDPosition);
			fixationDirection.Normalize();

			FVector eyeCaptureWorldPosition = ActiveFixation.LocalTransform->GetComponentTransform().TransformPosition(eyeCapture.LocalPosition);
			FVector eyeCaptureDirection = eyeCaptureWorldPosition - eyeCapture.HMDPosition;
			eyeCaptureDirection.Normalize();

			float angle = FMath::Cos(FMath::DegreesToRadians(MaxFixationAngle));
			if (FVector::DotProduct(eyeCaptureDirection, fixationDirection) < angle)
			{
				return true;
			}

			float distance = FVector::Distance(ActiveFixation.WorldPosition, eyeCapture.HMDPosition);
			float currentRadius = FMath::Atan(FMath::DegreesToRadians(MaxFixationAngle))*distance;
			ActiveFixation.MaxRadius = FMath::Max(ActiveFixation.MaxRadius, currentRadius);

			CachedEyeCapturePositions.Add(eyeCapture.LocalPosition);
			ActiveFixation.LocalPosition = averageLocalPos;

			return false;
		}
	}
	else //world fixation
	{
		if (eyeCapture.SkipPositionForFixationAverage) //probably looking at skybox
		{
			FVector WorldPosition = eyeCapture.WorldPosition;
			FSceneViewProjectionData ProjectionData;
			FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
			FIntRect const ViewRect = ProjectionData.GetConstrainedViewRect();
			FVector2D screenPos;

			FSceneView::ProjectWorldToScreen(WorldPosition, ViewRect, ViewProjectionMatrix, screenPos);
			FVector lookDir = eyeCapture.WorldPosition - eyeCapture.HMDPosition;
			lookDir.Normalize();
			FVector fixationDir = ActiveFixation.WorldPosition - eyeCapture.HMDPosition;
			fixationDir.Normalize();

			float angle = FMath::Cos(FMath::DegreesToRadians(MaxFixationAngle));

			if (FVector::DotProduct(lookDir, fixationDir) < angle)
			{
				return true;
			}
		}
		else
		{
			FVector WorldPosition = eyeCapture.WorldPosition;
			FSceneViewProjectionData ProjectionData;
			FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
			FIntRect const ViewRect = ProjectionData.GetConstrainedViewRect();
			FVector2D screenPos;

			FVector averageWorldPos = FVector::ZeroVector;
			for (int32 i = 0; i < CachedEyeCapturePositions.Num(); i++)
			{
				averageWorldPos += CachedEyeCapturePositions[i];
			}
			averageWorldPos += eyeCapture.WorldPosition;
			averageWorldPos /= (CachedEyeCapturePositions.Num() + 1);

			FVector lookDir = eyeCapture.WorldPosition - eyeCapture.HMDPosition;
			lookDir.Normalize();
			FVector fixationDir = ActiveFixation.WorldPosition - eyeCapture.HMDPosition;
			fixationDir.Normalize();

			float angle = FMath::Cos(FMath::DegreesToRadians(MaxFixationAngle));

			if (FVector::DotProduct(lookDir, fixationDir) < angle)
			{
				return true;
			}

			float distance = FVector::Distance(ActiveFixation.WorldPosition, eyeCapture.HMDPosition);
			float currentRadius = FMath::Atan(FMath::DegreesToRadians(MaxFixationAngle))*distance;
			ActiveFixation.MaxRadius = FMath::Max(ActiveFixation.MaxRadius, currentRadius);

			CachedEyeCapturePositions.Add(eyeCapture.WorldPosition);
			ActiveFixation.WorldPosition = averageWorldPos;
		}
	}
	return false;
}
bool UFixationRecorder::IsGazeOffTransform(FEyeCapture eyeCapture)
{
	if (!IsFixating) { return true; }
	if (eyeCapture.HitDynamicTransform != FixationTransform) { return true; }

	return false;
}
bool UFixationRecorder::CheckEndFixation(FFixation testFixation)
{
	if (EyeCaptures[index].Time > testFixation.LastInRange + FixationEndSaccadeConfirmMS)
	{
		return true;
	}
	if (EyeCaptures[index].Time > testFixation.LastEyesOpen + MaxBlinkMs)
	{
		return true;
	}
	if (EyeCaptures[index].Time > testFixation.LastNonDiscardedTime + MaxFixationConsecutiveNoiseMs)
	{
		return true;
	}
	if (testFixation.IsLocal)
	{
		if (EyeCaptures[index].Time > testFixation.LastOnTransform + MaxFixationConsecutiveNoiseDynamicMs)
		{
			return true;
		}
	}
	return false;
}

#if defined TOBII_EYETRACKING_ACTIVE
int64 UFixationRecorder::GetEyeCaptureTimestamp(TSharedPtr<ITobiiEyeTracker, ESPMode::ThreadSafe> eyetracker)
{
	long ts = eyetracker->GetCombinedGazeData().TimeStamp.ToUnixTimestamp();
	double miliseconds = eyetracker->GetCombinedGazeData().TimeStamp.GetMillisecond();
	int64 finalTime = ts * 1000 + miliseconds;

	return finalTime;
}

bool UFixationRecorder::AreEyesClosed(TSharedPtr<ITobiiEyeTracker, ESPMode::ThreadSafe> eyetracker)
{
	bool leftEyeClosed = eyetracker->GetLeftGazeData().EyeOpenness < 0.3f;
	bool rightEyeClosed = eyetracker->GetRightGazeData().EyeOpenness < 0.3f;

	if (leftEyeClosed && rightEyeClosed)
	{
		if (!eyesClosed)
		{
			for (int32 i = 0; i < CachedEyeCaptureCount; i++)
			{
				if (EyeCaptures[index].Time - PreBlinkDiscardMs > EyeCaptures[GetIndex(-i)].Time)
				{
					EyeCaptures[GetIndex(-i)].EyesClosed = true;
				}
				else
				{
					break;
				}
			}
		}
		eyesClosed = true;
		return true;
	}

	if (eyesClosed && !leftEyeClosed && !rightEyeClosed)
	{
		eyesClosed = false;
		EyeUnblinkTime = EyeCaptures[index].Time;
	}

	if (EyeUnblinkTime + BlinkEndWarmupMs > EyeCaptures[index].Time)
	{
		return true;
	}
	return false;
}
#else
bool UFixationRecorder::AreEyesClosed()
{
	return false;
}

int64 UFixationRecorder::GetEyeCaptureTimestamp()
{
	return 0;
}

#endif

// Called every frame from player tracker. needs direction and origin. dot products from screenspace into world
void UFixationRecorder::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (!IsFixating)
	{
		if (TryBeginLocalFixation())
		{
			ActiveFixation.IsLocal = true;
			IsFixating = true;
		}
		else if (TryBeginFixation())
		{
			ActiveFixation.IsLocal = false;
			FixationTransform = NULL;
			IsFixating = true;
		}
		//try to begin a fixation
	}
	else
	{
		if (ActiveFixation.IsLocal)
		{
			FVector localtoworld = ActiveFixation.LocalTransform->GetComponentTransform().TransformPosition(ActiveFixation.LocalPosition);
			//DrawDebugSphere(world, localtoworld, ActiveFixation.MaxRadius, 8, FColor::Magenta, false);
		}
		else
		{
			//DrawDebugSphere(world, ActiveFixation.WorldPosition, ActiveFixation.MaxRadius, 8, FColor::Orange, false);
		}

		EyeCaptures[index].OffTransform = IsGazeOffTransform(EyeCaptures[index]);
		EyeCaptures[index].OutOfRange = IsGazeOutOfRange(EyeCaptures[index]);
		ActiveFixation.AddEyeCapture(EyeCaptures[index]);

		ActiveFixation.DurationMs = EyeCaptures[index].Time - ActiveFixation.StartMs;

		if (CheckEndFixation(ActiveFixation))
		{
			RecordFixationEnd(ActiveFixation);
			IsFixating = false;
			if (ActiveFixation.IsLocal)
				FixationTransform = NULL;
			CachedEyeCapturePositions.Empty();
		}
	}

	EyeCaptures[index].Discard = false; // !eyetracker->GetCombinedGazeData().bIsGazeDataValid;
	EyeCaptures[index].SkipPositionForFixationAverage = false;
	EyeCaptures[index].OffTransform = false;
	EyeCaptures[index].OutOfRange = false;
	EyeCaptures[index].HitDynamicTransform = NULL;
	EyeCaptures[index].HMDPosition = cog->GetPlayerHMDPosition();

#if defined TOBII_EYETRACKING_ACTIVE
	auto eyetracker = ITobiiCore::GetEyeTracker();
	EyeCaptures[index].EyesClosed = AreEyesClosed(eyetracker);
	EyeCaptures[index].Time = GetEyeCaptureTimestamp(eyetracker);
	FVector Start = eyetracker->GetCombinedGazeData().WorldGazeOrigin;
	FVector End = eyetracker->GetCombinedGazeData().WorldGazeOrigin + eyetracker->GetCombinedGazeData().WorldGazeDirection * 100000.0f;
#else
	EyeCaptures[index].EyesClosed = AreEyesClosed();
	EyeCaptures[index].Time = GetEyeCaptureTimestamp();

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
#endif

	FCollisionQueryParams Params; // You can use this to customize various properties about the trace
								  //Params.AddIgnoredActor(GetOwner()); // Ignore the player's pawn

	FHitResult Hit; // The hit result gets populated by the line trace


					//TODO check if gaze hit a dynamic object
	bool bHit = false;
	FCollisionQueryParams gazeparams = FCollisionQueryParams(FName(), true);
	bHit = world->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_Visibility, gazeparams);
	if (bHit)
	{
		if (Hit.Actor.IsValid())
		{
			UDynamicObject* dyn = Hit.Actor->FindComponentByClass<UDynamicObject>();
			if (dyn != NULL)
			{
				EyeCaptures[index].HitDynamicTransform = dyn;
				EyeCaptures[index].LocalPosition = dyn->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
			}
		}

		//DrawDebugSphere(world, Hit.ImpactPoint, 3, 3, FColor::White, false, 0.2);
		EyeCaptures[index].WorldPosition = Hit.ImpactPoint;
	}
	else
	{
		EyeCaptures[index].SkipPositionForFixationAverage = true;
		EyeCaptures[index].HitDynamicTransform = NULL;
		EyeCaptures[index].WorldPosition = End; //direction of gaze 100m out
	}

	index = (index + 1) % CachedEyeCaptureCount;
}

bool UFixationRecorder::TryBeginLocalFixation()
{
	int32 sampleCount = 0;
	for (int32 i = 0; i < CachedEyeCaptureCount; i++)
	{
		if (EyeCaptures[index].Time + MinFixationMs < EyeCaptures[GetIndex(i)].Time) { break; }
		if (EyeCaptures[GetIndex(i)].Discard || EyeCaptures[GetIndex(i)].EyesClosed) { return false; }
		sampleCount++;
	}

	TArray<UDynamicObject*> hitTransforms;
	hitTransforms.Init(NULL, sampleCount);

	for (int32 i = 0; i < sampleCount; i++)
	{
		if (EyeCaptures[GetIndex(i)].HitDynamicTransform != NULL)
		{
			hitTransforms[i] = EyeCaptures[GetIndex(i)].HitDynamicTransform;
		}
	}

	int32 anyHitTransformCount = 0;
	for (int32 i = 0; i < sampleCount; i++)
	{
		if (hitTransforms[i] != NULL)
			anyHitTransformCount++;
	}

	if (anyHitTransformCount == 0) { return false; } //early escape for not looking at any dynamic objects

	TMap<UDynamicObject*, int32> transformUseCount;
	for (int i = 0; i < sampleCount; i++)
	{
		if (EyeCaptures[GetIndex(i)].HitDynamicTransform != NULL)
		{
			//add to map
			if (transformUseCount.Contains(EyeCaptures[GetIndex(i)].HitDynamicTransform))
			{
				transformUseCount[EyeCaptures[GetIndex(i)].HitDynamicTransform] ++;
			}
			else
			{
				transformUseCount.Add(EyeCaptures[GetIndex(i)].HitDynamicTransform, 1);
			}
		}
	}

	int32 useCount = 0;
	UDynamicObject* mostUsed = NULL;

	for (auto& t : transformUseCount)
	{
		if (t.Value > useCount)
		{
			mostUsed = t.Key;
			useCount = t.Value;
		}
	}

	FVector averageLocalPosition = FVector::ZeroVector;
	for (int32 i = 0; i < sampleCount; i++)
	{
		if (EyeCaptures[GetIndex(i)].HitDynamicTransform == mostUsed)
		{
			averageLocalPosition += EyeCaptures[GetIndex(i)].LocalPosition;
		}
	}

	if (mostUsed == NULL)
	{
		GLog->Log("most used dynamic object is null! should be impossible");
		return false;
	}

	averageLocalPosition /= useCount;

	bool withinRadius = true;

	FVector WorldPosition = EyeCaptures[index].WorldPosition;
	FSceneViewProjectionData ProjectionData;
	FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
	FIntRect const ViewRect = ProjectionData.GetConstrainedViewRect();
	FVector2D screenPos;

	FSceneView::ProjectWorldToScreen(WorldPosition, ViewRect, ViewProjectionMatrix, screenPos);

	float angle = FMath::Cos(FMath::DegreesToRadians(MaxFixationAngle));

	for (int32 i = 0; i < sampleCount; i++)
	{
		FVector lookDir = (EyeCaptures[GetIndex(i)].HMDPosition - EyeCaptures[GetIndex(i)].LocalPosition);
		lookDir.Normalize();
		FVector fixationDir = EyeCaptures[GetIndex(i)].HMDPosition - averageLocalPosition;
		fixationDir.Normalize();

		if (FVector::DotProduct(lookDir, fixationDir) < angle)
		{
			withinRadius = false;
			break;
		}
	}

	if (withinRadius)
	{
		ActiveFixation.LocalPosition = averageLocalPosition;
		ActiveFixation.WorldPosition = mostUsed->GetComponentTransform().TransformPosition(averageLocalPosition);

		FixationTransform = mostUsed;
		ActiveFixation.LocalTransform = mostUsed;
		ActiveFixation.DynamicObjectId = mostUsed->GetObjectId()->Id;
		float distance = FVector::Distance(ActiveFixation.WorldPosition, EyeCaptures[index].HMDPosition);
		float opposite = FMath::Atan(FMath::DegreesToRadians(MaxFixationAngle)) * distance;
		ActiveFixation.StartMs = EyeCaptures[index].Time;
		ActiveFixation.LastInRange = ActiveFixation.StartMs;
		ActiveFixation.StartDistance = distance;
		ActiveFixation.MaxRadius = opposite;
		ActiveFixation.IsLocal = true;

		for (int32 i = 0; i < sampleCount; i++)
		{
			if (EyeCaptures[GetIndex(i)].SkipPositionForFixationAverage) { continue; }
			CachedEyeCapturePositions.Add(EyeCaptures[GetIndex(i)].LocalPosition);
		}
		return true;
	}
	return false;
}

bool UFixationRecorder::TryBeginFixation()
{
	FVector averageWorldPos = FVector::ZeroVector;
	int32 averageWorldSamples = 0;
	int32 sampleCount = 0;

	for (int32 i = 0; i < CachedEyeCaptureCount; i++)
	{
		if (EyeCaptures[index].Time + MinFixationMs < EyeCaptures[GetIndex(i)].Time) { break; }
		if (EyeCaptures[GetIndex(i)].Discard || EyeCaptures[GetIndex(i)].EyesClosed) { return false; }
		sampleCount++;
		if (EyeCaptures[GetIndex(i)].SkipPositionForFixationAverage) { continue; }
		averageWorldPos += EyeCaptures[GetIndex(i)].WorldPosition;
		averageWorldSamples++;
	}
	if (averageWorldSamples == 0)
	{
		return false;
	}
	averageWorldPos /= averageWorldSamples;

	bool withinRadius = true;

	FVector WorldPosition = EyeCaptures[index].WorldPosition;
	FSceneViewProjectionData ProjectionData;
	FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
	FIntRect const ViewRect = ProjectionData.GetConstrainedViewRect();
	FVector2D screenPos;

	FSceneView::ProjectWorldToScreen(WorldPosition, ViewRect, ViewProjectionMatrix, screenPos);

	float angle = FMath::Cos(FMath::DegreesToRadians(MaxFixationAngle));

	for (int32 i = 0; i < sampleCount; i++)
	{
		FVector lookDir = (EyeCaptures[GetIndex(i)].HMDPosition - EyeCaptures[GetIndex(i)].WorldPosition);
		lookDir.Normalize();
		FVector fixationDir = EyeCaptures[GetIndex(i)].HMDPosition - averageWorldPos;
		fixationDir.Normalize();

		if (FVector::DotProduct(lookDir, fixationDir) < angle)
		{
			withinRadius = false;
			break;
		}
	}

	if (withinRadius)
	{
		ActiveFixation.WorldPosition = averageWorldPos;
		float distance = FVector::Distance(ActiveFixation.WorldPosition, EyeCaptures[index].HMDPosition);
		float opposite = FMath::Atan(FMath::DegreesToRadians(MaxFixationAngle)) * distance;
		ActiveFixation.StartMs = EyeCaptures[index].Time;
		ActiveFixation.LastInRange = ActiveFixation.StartMs;
		ActiveFixation.StartDistance = distance;
		ActiveFixation.MaxRadius = opposite;

		for (int32 i = 0; i < sampleCount; i++)
		{
			if (EyeCaptures[GetIndex(i)].SkipPositionForFixationAverage) { continue; }
			CachedEyeCapturePositions.Add(EyeCaptures[GetIndex(i)].WorldPosition);
		}
		return true;
	}
	return false;
}

void UFixationRecorder::RecordFixationEnd(FFixation fixation)
{
	//write fixation to json
	TSharedPtr<FJsonObject>fixObj = MakeShareable(new FJsonObject);
	fixObj->SetNumberField("starttime", fixation.StartMs * 1000);
	fixObj->SetNumberField("duration", fixation.DurationMs * 1000);
	fixObj->SetNumberField("maxradius", fixation.MaxRadius);

	if (fixation.IsLocal)
	{
		TArray<TSharedPtr<FJsonValue>> posArray;
		TSharedPtr<FJsonValueNumber> JsonValue;
		JsonValue = MakeShareable(new FJsonValueNumber(-(int32)fixation.LocalPosition.X)); //right
		posArray.Add(JsonValue);
		JsonValue = MakeShareable(new FJsonValueNumber((int32)fixation.LocalPosition.Z)); //up
		posArray.Add(JsonValue);
		JsonValue = MakeShareable(new FJsonValueNumber((int32)fixation.LocalPosition.Y));  //forward
		posArray.Add(JsonValue);

		fixObj->SetArrayField("p", posArray);

		fixObj->SetStringField("dynamicid", fixation.DynamicObjectId);
	}
	else
	{
		TArray<TSharedPtr<FJsonValue>> posArray;
		TSharedPtr<FJsonValueNumber> JsonValue;
		JsonValue = MakeShareable(new FJsonValueNumber(-(int32)fixation.WorldPosition.X)); //right
		posArray.Add(JsonValue);
		JsonValue = MakeShareable(new FJsonValueNumber((int32)fixation.WorldPosition.Z)); //up
		posArray.Add(JsonValue);
		JsonValue = MakeShareable(new FJsonValueNumber((int32)fixation.WorldPosition.Y));  //forward
		posArray.Add(JsonValue);

		fixObj->SetArrayField("p", posArray);
	}

	Fixations.Add(fixObj);
	if (Fixations.Num() > FixationBatchSize)
	{
		SendData();
	}

	//DrawDebugSphere(world, fixation.WorldPosition, 10, 8, FColor::Cyan, false, 100);
}

void UFixationRecorder::SendData()
{
	if (!cog.IsValid() || !cog->HasStartedSession()) { return; }
	if (cog->GetCurrentSceneVersionNumber().Len() == 0) { return; }
	if (!cog->GetCurrentSceneData().IsValid()) { return; }

	if (Fixations.Num() == 0) { return; }

	TSharedPtr<FJsonObject> wholeObj = MakeShareable(new FJsonObject);

	wholeObj->SetStringField("userid", cog->GetUserID());
	wholeObj->SetStringField("sessionid", cog->GetSessionID());
	wholeObj->SetNumberField("timestamp", (int32)cog->GetSessionTimestamp());
	wholeObj->SetNumberField("part", jsonFixationPart);
	wholeObj->SetStringField("formatversion", "1.0");

	TArray<TSharedPtr<FJsonValue>> dataArray;
	for (int32 i = 0; i != Fixations.Num(); ++i)
	{
		TSharedPtr<FJsonValueObject> snapshotValue;
		snapshotValue = MakeShareable(new FJsonValueObject(Fixations[i]));
		dataArray.Add(snapshotValue);
	}

	wholeObj->SetArrayField("data", dataArray);


	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(wholeObj.ToSharedRef(), Writer);

	if (OutputString.Len() > 0)
	{
		//cog->network->NetworkCall("fixation", OutputString);
	}
	Fixations.Empty();
}
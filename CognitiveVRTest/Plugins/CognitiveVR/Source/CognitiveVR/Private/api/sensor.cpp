/*
** Copyright (c) 2016 CognitiveVR, Inc. All rights reserved.
*/

#include "CognitiveVR/Private/api/sensor.h"

Sensors::Sensors()
{
	cog = FAnalyticsCognitiveVR::Get().GetCognitiveVRProvider().Pin();

	FString ValueReceived;
	LastSensorValues.Empty();

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorDataLimit", false);
	if (ValueReceived.Len() > 0)
	{
		int32 sensorLimit = FCString::Atoi(*ValueReceived);
		if (sensorLimit > 0)
		{
			SensorThreshold = sensorLimit;
		}
	}

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorExtremeLimit", false);
	if (ValueReceived.Len() > 0)
	{
		int32 parsedValue = FCString::Atoi(*ValueReceived);
		if (parsedValue > 0)
		{
			ExtremeBatchSize = parsedValue;
		}
	}

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorMinTimer", false);
	if (ValueReceived.Len() > 0)
	{
		int32 parsedValue = FCString::Atoi(*ValueReceived);
		if (parsedValue > 0)
		{
			MinTimer = parsedValue;
		}
	}

	ValueReceived = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "SensorAutoTimer", false);
	if (ValueReceived.Len() > 0)
	{
		int32 parsedValue = FCString::Atoi(*ValueReceived);
		if (parsedValue > 0)
		{
			AutoTimer = parsedValue;
		}
	}
}

void Sensors::StartSession()
{
	if (!cog.IsValid()) {
		return;
	}
	if (cog->EnsureGetWorld() == NULL)
	{
		CognitiveLog::Warning("Sensors::StartSession - GetWorld is Null! Likely missing PlayerTrackerComponent on Player actor");
		return;
	}
	if (cog->EnsureGetWorld()->GetGameInstance() == NULL) {
		return;
	}
	cog->EnsureGetWorld()->GetGameInstance()->GetTimerManager().SetTimer(AutoSendHandle, FTimerDelegate::CreateRaw(this, &Sensors::SendData), AutoTimer, true);
}

void Sensors::RecordSensor(FString Name, float value)
{
	LastSensorValues.Add(Name, value);

	if (SensorDataPoints.Contains(Name))
	{
		SensorDataPoints[Name].Append(",[" + FString::SanitizeFloat(Util::GetTimestamp()) + "," + FString::SanitizeFloat(value) + "]");
	}
	else
	{
		SensorDataPoints.Emplace(Name, "[" + FString::SanitizeFloat(Util::GetTimestamp()) + "," + FString::SanitizeFloat(value) + "]");
	}

	sensorDataCount ++;
	if (sensorDataCount >= SensorThreshold)
	{
		Sensors::TrySendData();
	}
}

void Sensors::RecordSensor(FString Name, double value)
{
	LastSensorValues.Add(Name, (float)value);

	if (SensorDataPoints.Contains(Name))
	{
		SensorDataPoints[Name].Append(",[" + FString::SanitizeFloat(Util::GetTimestamp()) + "," + FString::SanitizeFloat(value) + "]");
	}
	else
	{
		SensorDataPoints.Emplace(Name, "[" + FString::SanitizeFloat(Util::GetTimestamp()) + "," + FString::SanitizeFloat(value) + "]");
	}

	sensorDataCount++;
	if (sensorDataCount >= SensorThreshold)
	{
		Sensors::TrySendData();
	}
}

void Sensors::TrySendData()
{
	if (cog->EnsureGetWorld() != NULL)
	{
		bool withinMinTimer = LastSendTime + MinTimer > UCognitiveVRBlueprints::GetSessionDuration();
		bool withinExtremeBatchSize = sensorDataCount < ExtremeBatchSize;
		
		if (withinMinTimer && withinExtremeBatchSize)
		{
			return;
		}
	}
	SendData();
}

void Sensors::SendData()
{
	if (!cog.IsValid() || !cog->HasStartedSession())
	{
		return;
	}

	if (SensorDataPoints.Num() == 0)
	{
		return;
	}

	LastSendTime = UCognitiveVRBlueprints::GetSessionDuration();

	TSharedPtr<FJsonObject> wholeObj = MakeShareable(new FJsonObject);

	wholeObj->SetStringField("name", cog->GetUserID());
	if (!cog->LobbyId.IsEmpty())
	{
		wholeObj->SetStringField("lobbyId", cog->LobbyId);
	}
	wholeObj->SetNumberField("timestamp", cog->GetSessionTimestamp());
	wholeObj->SetStringField("sessionid", cog->GetSessionID());
	wholeObj->SetNumberField("part", jsonPart);
	wholeObj->SetStringField("formatversion", "1.0");
	jsonPart++;

	wholeObj->SetStringField("data", "SENSORDATAHERE");

	FString OutputString;
	auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
	FJsonSerializer::Serialize(wholeObj.ToSharedRef(), Writer);

	FString allData;
	int32 dataEntries = 0;

	for (const auto& Entry : SensorDataPoints)
	{
		dataEntries++;
		allData = allData.Append("{\"name\":\"" + Entry.Key + "\",\"data\":[" + Entry.Value + "]}");
		if (dataEntries < SensorDataPoints.Num())
			allData.Append(",");
	}

	FString complete = "[" + allData + "]";
	const TCHAR* charcomplete = *complete;
	OutputString = OutputString.Replace(TEXT("\"SENSORDATAHERE\""), charcomplete);

	cog->network->NetworkCall("sensors", OutputString);

	SensorDataPoints.Empty();
	sensorDataCount = 0;
}

TMap<FString, float> Sensors::GetLastSensorValues()
{
	return LastSensorValues;
}
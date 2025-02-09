/*
** Copyright (c) 2024 Cognitive3D, Inc. All rights reserved.
*/

#pragma once

#include "Internationalization/Text.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

#include "DynamicIdPoolAsset.generated.h"


/**
 * Implements an asset that can be used to store known ids for dynamic objects spawned at runtime
 */
UCLASS(BlueprintType, hidecategories=(Object))
class COGNITIVE3D_API UDynamicIdPoolAsset
	: public UObject
{
	GENERATED_BODY()

public:

	//used by dynamic object when selecting ids
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Id Pool")
	TArray<FString> Ids;

	//used when uploading these ids for aggregation. indicates what mesh to render with
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Id Pool")
	FString MeshName;

	//used when uploading these ids for aggregation. display name on the dashboard
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Id Pool")
	FString PrefabName;

	void GetId(FString& outId)
	{
		if (Ids.Num() > 0)
		{
			outId = *Ids[0];
			Ids.RemoveAt(0);
			return;
		}
		outId = FGuid::NewGuid().ToString();
	}

	void ReturnId(FString id)
	{
		if (Ids.Contains(id))
			return;
		Ids.Add(id);
	}

	void GenerateNewId()
	{
		FString guid = FGuid::NewGuid().ToString();
		Ids.Add(guid);
	}
};

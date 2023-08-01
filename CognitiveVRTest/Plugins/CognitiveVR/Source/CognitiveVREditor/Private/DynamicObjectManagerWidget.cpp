
#include "DynamicObjectManagerWidget.h"

#define LOCTEXT_NAMESPACE "BaseToolEditor"

TArray<FDashboardObject> SDynamicObjectManagerWidget::dashboardObjects = TArray<FDashboardObject>();

TArray<TSharedPtr<FDynamicData>> SDynamicObjectManagerWidget::GetSceneDynamics()
{
	return FCognitiveEditorTools::GetInstance()->GetSceneDynamics();
}

void SDynamicObjectManagerWidget::CheckForExpiredDeveloperKey()
{
	if (FCognitiveEditorTools::GetInstance()->HasDeveloperKey())
	{
		auto Request = FHttpModule::Get().CreateRequest();
		Request->OnProcessRequestComplete().BindRaw(this, &SDynamicObjectManagerWidget::OnDeveloperKeyResponseReceived);
		FString gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
		FString url = "https://" + gateway + "/v0/apiKeys/verify";
		Request->SetURL(url);
		Request->SetVerb("GET");
		Request->SetHeader(TEXT("Authorization"), "APIKEY:DEVELOPER " + FCognitiveEditorTools::GetInstance()->DeveloperKey);
		Request->ProcessRequest();
	}
}

void SDynamicObjectManagerWidget::OnDeveloperKeyResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (Response.IsValid() == false)
	{
		SGenericDialogWidget::OpenDialog(FText::FromString("Your developer key has expired"), SNew(STextBlock).Text(FText::FromString("Please log in to the dashboard, select your project, and generate a new developer key.\n\nNote:\nDeveloper keys allow you to upload and modify Scenes, and the keys expire after 90 days.\nApplication keys authorize your app to send data to our server, and they never expire.")));

		GLog->Log("Developer Key Response is invalid. Developer key may be invalid or expired. Check your internet connection");
		return;
	}

	int32 responseCode = Response->GetResponseCode();
	if (responseCode == 200)
	{
		GLog->Log("Developer Key Response Code is 200");
	}
	else
	{
		SGenericDialogWidget::OpenDialog(FText::FromString("Your developer key has expired"), SNew(STextBlock).Text(FText::FromString("Please log in to the dashboard, select your project, and generate a new developer key.\n\nNote:\nDeveloper keys allow you to upload and modify Scenes, and the keys expire after 90 days.\nApplication keys authorize your app to send data to our server, and they never expire.")));
		GLog->Log("Developer Key Response Code is not 200. Developer key may be invalid or expired");
	}
}

void SDynamicObjectManagerWidget::GetDashboardManifest()
{
	if (FCognitiveEditorTools::GetInstance()->HasDeveloperKey())
	{
		auto currentSceneData = FCognitiveEditorTools::GetInstance()->GetCurrentSceneData();
		if (!currentSceneData.IsValid())
		{
			GLog->Log("SDynamicObjectManagerWidget::GetDashboardManifest failed. current scene is null");
			return;
		}

		auto Request = FHttpModule::Get().CreateRequest();
		Request->OnProcessRequestComplete().BindRaw(this, &SDynamicObjectManagerWidget::OnDashboardManifestResponseReceived);
		FString gateway = FAnalytics::Get().GetConfigValueFromIni(GEngineIni, "/Script/CognitiveVR.CognitiveVRSettings", "Gateway", false);
		FString versionid = FString::FromInt(currentSceneData->VersionId);
		FString url = "https://" + gateway + "/v0/versions/"+versionid+"/objects";
		Request->SetURL(url);
		Request->SetVerb("GET");
		Request->SetHeader(TEXT("Authorization"), "APIKEY:DEVELOPER " + FCognitiveEditorTools::GetInstance()->DeveloperKey);
		Request->ProcessRequest();
	}
	else
	{
		GLog->Log("SDynamicObjectManagerWidget::GetDashboardManifest failed. developer key missing");
	}
}

void SDynamicObjectManagerWidget::OnDashboardManifestResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (Response.IsValid() == false)
	{
		SGenericDialogWidget::OpenDialog(FText::FromString("Your developer key has expired"), SNew(STextBlock).Text(FText::FromString("Please log in to the dashboard, select your project, and generate a new developer key.\n\nNote:\nDeveloper keys allow you to upload and modify Scenes, and the keys expire after 90 days.\nApplication keys authorize your app to send data to our server, and they never expire.")));

		GLog->Log("Developer Key Response is invalid. Developer key may be invalid or expired. Check your internet connection");
		return;
	}

	int32 responseCode = Response->GetResponseCode();
	if (responseCode == 200)
	{		
		auto content = Response->GetContentAsString();		
		if (FJsonObjectConverter::JsonArrayStringToUStruct(content, &dashboardObjects, 0, 0))
		{
			RefreshList();
		}
		else
		{
			GLog->Log("SDynamicObjectManagerWidget::OnDashboardManifestResponseReceived failed to deserialize dynamic object list");
			GLog->Log(content);
		}
	}
	else
	{
		GLog->Log("SDynamicObjectManagerWidget::OnDashboardManifestResponseReceived response code " + FString::FromInt(responseCode));
	}
}

void SDynamicObjectManagerWidget::Construct(const FArguments& Args)
{
	float padding = 10;

	CheckForExpiredDeveloperKey();

	ChildSlot
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot() //warning for invalid scenes
			.Padding(0, 10, 0, padding) //slight padding so
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SBox)
					[
						SNew(STextBlock)
						.Visibility(this, &SDynamicObjectManagerWidget::GetSceneWarningVisibility)
						.Text(this, &SDynamicObjectManagerWidget::GetUploadInvalidCause)
					]
				]
			]

			+ SVerticalBox::Slot() //warning for invalid scenes
			.Padding(0, 0, 0, padding)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SBox)
					[
						SNew(STextBlock)
						.Text(this, &SDynamicObjectManagerWidget::GetSceneText)
					]
				]
			]

			+SVerticalBox::Slot() //refresh button and dynamic object count
			.AutoHeight()
			.Padding(0, 0, 0, padding)
			[
				SNew(SBox)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(FText::FromString("Refresh"))
						.OnClicked(this, &SDynamicObjectManagerWidget::RefreshDisplayDynamicObjectsCountInScene)
					]
					+SHorizontalBox::Slot()
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Justification(ETextJustify::Center)
						.Text_Raw(FCognitiveEditorTools::GetInstance(), &FCognitiveEditorTools::DisplayDynamicObjectsCountInScene)
					]
				]
			]
			+ SVerticalBox::Slot() //list widget
			.AutoHeight()
			.MaxHeight(250)
			.Padding(0, 0, 0, padding)
			[
				SNew(SBox)
				[
					SAssignNew(SceneDynamicObjectTable,SDynamicObjectTableWidget)
				]
			]

			
			+ SVerticalBox::Slot() //upload buttons
			.Padding(0, 0, 0, padding)
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				[
					SNew(SBox)
					.HeightOverride(64)
					.WidthOverride(128)
					[
						SNew(SButton)
						.IsEnabled(this, &SDynamicObjectManagerWidget::IsUploadSelectedEnabled)
						.Text_Raw(this, &SDynamicObjectManagerWidget::UploadSelectedText)
						.OnClicked_Raw(this, &SDynamicObjectManagerWidget::UploadSelectedDynamicObjects)
					]
				]
				+SHorizontalBox::Slot()
				[
					SNew(SBox)
					.HeightOverride(64)
					.WidthOverride(128)
					[
						SNew(SButton)
						.IsEnabled(this, &SDynamicObjectManagerWidget::IsUploadAllEnabled)
						.Text(FText::FromString("Upload All Meshes"))
						.OnClicked_Raw(this, &SDynamicObjectManagerWidget::UploadAllDynamicObjects)
					]
				]
			]

			+SVerticalBox::Slot() //invalid dynamic data warning
			.AutoHeight()
			.Padding(FMargin(64, 24, 64, 24))
			[
				SNew(SBorder)
				.Visibility_Raw(FCognitiveEditorTools::GetInstance(), &FCognitiveEditorTools::GetDuplicateDyanmicObjectVisibility)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.LightGroupBorder"))
				.Padding(8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("SettingsEditor.WarningIcon"))
					]
						// Notice
					+SHorizontalBox::Slot()
					.Padding(16.0f, 0.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.ColorAndOpacity(FLinearColor::Black)
						.AutoWrapText(true)
						.Text(FText::FromString("Dynamic Objects must have a valid Mesh Name\nTo have data aggregated, Dynamic Objects must have a Unique Id"))
					]
					+SHorizontalBox::Slot()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &SDynamicObjectManagerWidget::ValidateAndRefresh)
						[
							SNew(STextBlock)
							.Justification(ETextJustify::Center)
							.AutoWrapText(true)
							.Text(FText::FromString("Validate Mesh Names and Unique Ids"))
						]
					]
				]
			]
			]
		];

		FCognitiveEditorTools::GetInstance()->ReadSceneDataFromFile();
		RefreshDisplayDynamicObjectsCountInScene();
}

void SDynamicObjectManagerWidget::RefreshList()
{
	SceneDynamicObjectTable->TableViewWidget->RequestListRefresh();
}

FReply SDynamicObjectManagerWidget::SelectDynamic(TSharedPtr<FDynamicData> data)
{
	GEditor->SelectNone(false, true, false);

	for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
	{
		UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
		if (actorComponent == NULL)
		{
			continue;
		}
		UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
		if (dynamic == NULL)
		{
			continue;
		}
		if (dynamic->GetOwner()->GetName() != data->Name) { continue; }
		if (dynamic->CustomId != data->Id) { continue; }
		if (dynamic->MeshName != data->MeshName) { continue; }

		GEditor->SelectActor((*ActorItr), true, true, true, true);

		break;
	}

	return FReply::Handled();
}

FReply SDynamicObjectManagerWidget::RefreshDisplayDynamicObjectsCountInScene()
{
	FCognitiveEditorTools::GetInstance()->RefreshDisplayDynamicObjectsCountInScene();
	GetDashboardManifest();
	
	if (!SceneDynamicObjectTable.IsValid())
	{
		GLog->Log("SceneDynamicObjectTable invalid!");
	}
	else
	{
		SceneDynamicObjectTable->RefreshTable();
	}

	return FReply::Handled();
}

EVisibility SDynamicObjectManagerWidget::GetDuplicateDyanmicObjectVisibility() const
{
	return FCognitiveEditorTools::GetInstance()->GetDuplicateDyanmicObjectVisibility();
}

FReply SDynamicObjectManagerWidget::UploadAllDynamicObjects()
{
	//popup asking if meshes should be exported too
	FSuppressableWarningDialog::FSetupInfo Info(LOCTEXT("ExportSelectedDynamicsBody", "Do you want to export the selected Dynamic Object meshes before uploading to Scene Explorer?"), LOCTEXT("ExportSelectedDynamicsTitle", "Export Selected Dynamic Objects"), "ExportSelectedDynamicsBody");
	Info.ConfirmText = LOCTEXT("Yes", "Yes");
	Info.CancelText = LOCTEXT("No", "No");
	Info.CheckBoxText = FText();
	FSuppressableWarningDialog ExportSelectedDynamicMeshes(Info);
	FSuppressableWarningDialog::EResult result = ExportSelectedDynamicMeshes.ShowModal();

	if (result == FSuppressableWarningDialog::EResult::Confirm)
	{
		//try to export all dynamics that don't have directories
		FProcHandle fph = FCognitiveEditorTools::GetInstance()->ExportNewDynamics();
		if (fph.IsValid())
		{
			FPlatformProcess::WaitForProc(fph);
		}
	}

	FSuppressableWarningDialog::FSetupInfo Info2(LOCTEXT("UploadSelectedDynamicsBody", "Do you want to upload the selected Dynamic Object to Scene Explorer?"), LOCTEXT("UploadSelectedDynamicsTitle", "Upload Selected Dynamic Objects"), "UploadSelectedDynamicsBody");
	Info2.ConfirmText = LOCTEXT("Yes", "Yes");
	Info2.CancelText = LOCTEXT("No", "No");
	Info2.CheckBoxText = FText();
	FSuppressableWarningDialog UploadSelectedDynamicMeshes(Info2);
	FSuppressableWarningDialog::EResult result2 = UploadSelectedDynamicMeshes.ShowModal();

	if (result2 == FSuppressableWarningDialog::EResult::Confirm)
	{
		//then upload all
		FCognitiveEditorTools::GetInstance()->UploadDynamics();

		//upload aggregation manifest data
		FCognitiveEditorTools::GetInstance()->UploadDynamicsManifest();
	}

	return FReply::Handled();
}

FReply SDynamicObjectManagerWidget::ValidateAndRefresh()
{
	FCognitiveEditorTools::GetInstance()->SetUniqueDynamicIds();
	RefreshDisplayDynamicObjectsCountInScene();
	return FReply::Handled();
}

void SDynamicObjectManagerWidget::OnBlenderPathChanged(const FText& Text)
{
	FCognitiveEditorTools::GetInstance()->BlenderPath = Text.ToString();
}

void SDynamicObjectManagerWidget::OnExportPathChanged(const FText& Text)
{
	FCognitiveEditorTools::GetInstance()->BaseExportDirectory = Text.ToString();
	RefreshList();
}

FReply SDynamicObjectManagerWidget::UploadSelectedDynamicObjects()
{
	auto selected = SceneDynamicObjectTable->TableViewWidget->GetSelectedItems();

	//popup asking if meshes should be exported too
	FSuppressableWarningDialog::FSetupInfo Info(LOCTEXT("ExportSelectedDynamicsBody", "Do you want to export the selected Dynamic Object meshes before uploading to Scene Explorer?"), LOCTEXT("ExportSelectedDynamicsTitle", "Export Selected Dynamic Objects"), "ExportSelectedDynamicsBody");
	Info.ConfirmText = LOCTEXT("Yes", "Yes");
	Info.CancelText = LOCTEXT("No", "No");
	Info.CheckBoxText = FText();
	FSuppressableWarningDialog ExportSelectedDynamicMeshes(Info);
	FSuppressableWarningDialog::EResult result = ExportSelectedDynamicMeshes.ShowModal();

	if (result == FSuppressableWarningDialog::EResult::Confirm)
	{
		//export
		TArray<TSharedPtr<FDynamicData>> dynamicData;

		for (auto& elem : selected)
		{
			//if files dont exist, export first
			FString path = FCognitiveEditorTools::GetInstance()->GetDynamicsExportDirectory() + "/" + elem->MeshName + "/" + elem->MeshName;
			FString gltfpath = path + ".gltf";
			if (!FPaths::FileExists(*gltfpath))
			{
				//not exported
				dynamicData.Add(elem);
			}
		}

		FProcHandle fph = FCognitiveEditorTools::GetInstance()->ExportDynamicData(dynamicData);
		if (fph.IsValid())
		{
			FPlatformProcess::WaitForProc(fph);
		}
	}	

	FSuppressableWarningDialog::FSetupInfo Info2(LOCTEXT("UploadSelectedDynamicsBody", "Do you want to upload the selected Dynamic Object to Scene Explorer?"), LOCTEXT("UploadSelectedDynamicsTitle", "Upload Selected Dynamic Objects"), "UploadSelectedDynamicsBody");
	Info2.ConfirmText = LOCTEXT("Yes", "Yes");
	Info2.CancelText = LOCTEXT("No", "No");
	Info2.CheckBoxText = FText();
	FSuppressableWarningDialog UploadSelectedDynamicMeshes(Info2);
	FSuppressableWarningDialog::EResult result2 = UploadSelectedDynamicMeshes.ShowModal();

	if (result2 == FSuppressableWarningDialog::EResult::Confirm)
	{
		//then upload
		for (auto& elem : selected)
		{
			FCognitiveEditorTools::GetInstance()->UploadDynamic(elem->MeshName);
		}

		//upload aggregation manifest data of selected objects

		TArray<UDynamicObject*> dynamics;

		//get all the dynamic objects in the scene
		for (TActorIterator<AActor> ActorItr(GWorld); ActorItr; ++ActorItr)
		{
			UActorComponent* actorComponent = (*ActorItr)->GetComponentByClass(UDynamicObject::StaticClass());
			if (actorComponent == NULL)
			{
				continue;
			}
			UDynamicObject* dynamic = Cast<UDynamicObject>(actorComponent);
			if (dynamic == NULL)
			{
				continue;
			}

			if (dynamic->IdSourceType == EIdSourceType::CustomId && dynamic->CustomId != "")
			{
				FString findId = dynamic->CustomId;

				auto isDynamicSelected = selected.ContainsByPredicate([findId](const TSharedPtr<FDynamicData> InItem) {return InItem->Id == findId;});

				if (isDynamicSelected)
				{
					dynamics.Add(dynamic);
				}
			}
		}

		FCognitiveEditorTools::GetInstance()->UploadSelectedDynamicsManifest(dynamics);
	}

	return FReply::Handled();
}

bool SDynamicObjectManagerWidget::IsUploadAllEnabled() const
{
	if (!FCognitiveEditorTools::GetInstance()->HasDeveloperKey()) { return false; }
	if (!FCognitiveEditorTools::GetInstance()->HasFoundBlender()) { return false; }
	if (!FCognitiveEditorTools::GetInstance()->HasSetExportDirectory()) { return false; }
	if (!FCognitiveEditorTools::GetInstance()->CurrentSceneHasSceneId()) { return false; }
	return true;
}

bool SDynamicObjectManagerWidget::IsUploadInvalid() const
{
	if (!FCognitiveEditorTools::GetInstance()->HasDeveloperKey()) { return true; }
	if (!FCognitiveEditorTools::GetInstance()->HasFoundBlender()) { return true; }
	if (!FCognitiveEditorTools::GetInstance()->HasSetExportDirectory()) { return true; }
	if (!FCognitiveEditorTools::GetInstance()->CurrentSceneHasSceneId()) { return true; }
	return false;
}

FText SDynamicObjectManagerWidget::GetUploadInvalidCause() const
{
	if (!FCognitiveEditorTools::GetInstance()->HasDeveloperKey()) { return FText::FromString("Upload Invalid. Developer Key is not set"); }
	if (!FCognitiveEditorTools::GetInstance()->HasFoundBlender()) { return FText::FromString("Upload Invalid. Blender path is invalid"); }
	if (!FCognitiveEditorTools::GetInstance()->HasSetExportDirectory()) { return FText::FromString("Upload Invalid. Export Path is invalid"); }
	if (!FCognitiveEditorTools::GetInstance()->CurrentSceneHasSceneId()) { return FText::FromString("Upload Invalid. Scene does not have a Scene ID"); }
	return FText::GetEmpty();
}

EVisibility SDynamicObjectManagerWidget::GetSceneWarningVisibility() const
{
	if (!FCognitiveEditorTools::GetInstance()->HasDeveloperKey()) { return EVisibility::Visible; }
	if (!FCognitiveEditorTools::GetInstance()->HasFoundBlender()) { return EVisibility::Visible; }
	if (!FCognitiveEditorTools::GetInstance()->HasSetExportDirectory()) { return EVisibility::Visible; }
	if (!FCognitiveEditorTools::GetInstance()->CurrentSceneHasSceneId()) { return EVisibility::Visible;}
	return EVisibility::Collapsed;
}

bool SDynamicObjectManagerWidget::IsUploadIdsEnabled() const
{
	if (!FCognitiveEditorTools::GetInstance()->HasDeveloperKey()) { return false; }
	if (!FCognitiveEditorTools::GetInstance()->CurrentSceneHasSceneId()) { return false; }
	return true;
}

bool SDynamicObjectManagerWidget::IsUploadSelectedEnabled() const
{
	if (!FCognitiveEditorTools::GetInstance()->HasDeveloperKey()) { return false; }
	if (!FCognitiveEditorTools::GetInstance()->HasFoundBlenderAndExportDir()) { return false; }
	if (!FCognitiveEditorTools::GetInstance()->CurrentSceneHasSceneId()) { return false; }
	
	auto tools = FCognitiveEditorTools::GetInstance();
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			//SelectionSetCache.Add(Actor);
			UActorComponent* actorComponent = Actor->GetComponentByClass(UDynamicObject::StaticClass());
			if (actorComponent == NULL)
			{
				continue;
			}
			UDynamicObject* dynamicComponent = Cast<UDynamicObject>(actorComponent);
			if (dynamicComponent == NULL)
			{
				continue;
			}
			return true;
		}
	}

	return false;
}

FText SDynamicObjectManagerWidget::UploadSelectedText() const
{
	auto selected = SceneDynamicObjectTable->TableViewWidget->GetSelectedItems();

	//should export all dynamic objects that need it, then wait for blender to fix them all

	TArray<FString> dynamicMeshNames;

	for (auto& elem : selected)
	{
		if (dynamicMeshNames.Contains(elem->MeshName))
		{
			continue;
		}
		dynamicMeshNames.Add(elem->MeshName);
	}

	//get selected dynamic data
	//for each unique mesh name

	return FText::FromString("Upload " + FString::FromInt(dynamicMeshNames.Num()) + " Selected Meshes");
}

FText SDynamicObjectManagerWidget::ExportSelectedText() const
{
	int32 validExportCount=0;
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			//SelectionSetCache.Add(Actor);
			UActorComponent* actorComponent = Actor->GetComponentByClass(UDynamicObject::StaticClass());
			if (actorComponent == NULL)
			{
				continue;
			}
			UDynamicObject* dynamicComponent = Cast<UDynamicObject>(actorComponent);
			if (dynamicComponent == NULL)
			{
				continue;
			}
			validExportCount++;
		}
	}

	return FText::FromString("Export " + FString::FromInt(validExportCount) + " Selected");
}

FText SDynamicObjectManagerWidget::GetSceneText() const
{
	auto tools = FCognitiveEditorTools::GetInstance();
	if (tools->CurrentSceneHasSceneId())
	{
		auto data = tools->GetCurrentSceneData();
		return FText::FromString("Scene: " + data->Name + "   Version: " + FString::FromInt(data->VersionNumber));
	}
	return FText::FromString("Scene has not been exported!");
}

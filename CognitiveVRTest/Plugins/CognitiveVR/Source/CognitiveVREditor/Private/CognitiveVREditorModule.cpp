#include "CognitiveVREditorPrivatePCH.h"
#include "BaseEditorTool.h"
#include "PropertyEditorModule.h"
#include "LevelEditor.h"
#include "CognitiveTools.h"
#include "SDockTab.h"
#include "SceneSetupWindow.h"
#include "SetupCustomization.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"
#include "SSceneSetupWidget.h"

//#include "DemoStyle.h"

#define LOCTEXT_NAMESPACE "DemoTools"

class FCognitiveVREditorModule : public IModuleInterface
{
public:

	//	FName SequenceRecorderTabName = FName("SequenceRecorder");
		// IMoudleInterface interface
		//virtual void StartupModule() override;
		//virtual void ShutdownModule() override;
		// End of IModuleInterface interface

		//static void OnToolWindowClosed(const TSharedRef<SWindow>& Window, UBaseEditorTool* Instance);

		//void AddMenuEntry(FMenuBuilder& MenuBuilder);
		//void DisplayPopup();
		//void SpawnSequenceRecorderTab(const FSpawnTabArgs& SpawnTabArgs);

		/*static void HandleTestCommandExcute();

		static bool HandleTestCommandCanExcute();*/

	//TSharedPtr<FUICommandList> CommandList;


	virtual void StartupModule() override
	{
#if WITH_EDITOR
		// Create the Extender that will add content to the menu
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		/*
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension(
			"EditMain",
			EExtensionHook::After,
			NULL,
			FMenuExtensionDelegate::CreateRaw(this, &FCognitiveVREditorModule::AddMenuEntry)
		);

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);*/


		// register 'keep simulation changes' recorder
		//FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		//LevelEditorModule.OnCaptureSingleFrameAnimSequence().BindStatic(&FCognitiveVREditorModule::HandleCaptureSingleFrameAnimSequence);

		GLog->Log("COGNITIVE MODULE BEGIN");

		// register standalone UI
		LevelEditorTabManagerChangedHandle = LevelEditorModule.OnTabManagerChanged().AddLambda([]()
		{
			TSharedPtr<FSlateStyleSet> StyleSet = MakeShareable(new FSlateStyleSet("CognitiveEditor"));
			StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));
			
			FString iconpath = IPluginManager::Get().FindPlugin(TEXT("CognitiveVR"))->GetBaseDir() / TEXT("Resources") / TEXT("CognitiveSceneWizardTabIcon.png");
			//FName BrushName = FName(*iconpath);

			//const TCHAR* charPath = *iconpath;
			StyleSet->Set(FName(*iconpath),new FSlateImageBrush(iconpath,FVector2D(128,128),FSlateColor()));

			//FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());


			GLog->Log("---------------......................ontabmanagerchagnd");
			FLevelEditorModule& LocalLevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			LocalLevelEditorModule.GetLevelEditorTabManager()->RegisterTabSpawner(FName("CognitiveSceneSetup"), FOnSpawnTab::CreateStatic(&FCognitiveVREditorModule::SpawnCognitiveSceneSetupTab))
				.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
				.SetDisplayName(LOCTEXT("SceneSetupTabTitle", "Cognitive Scene Setup"))
				.SetTooltipText(LOCTEXT("SceneSetupTooltipText", "Open the Sequence Recorder tab."));
				//.SetIcon(FSlateIcon(StyleSet, "CognitiveSceneWizardTabIcon"));
		});

		// Register the details customizations
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomClassLayout(TEXT("CognitiveVRSettings"), FOnGetDetailCustomizationInstance::CreateStatic(&FCognitiveTools::MakeInstance));
		PropertyModule.RegisterCustomClassLayout(TEXT("BaseEditorTool"), FOnGetDetailCustomizationInstance::CreateStatic(&FSetupCustomization::MakeInstance));
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		if (GEditor)
		{
			//FDemoCommands::Unregister();
			//FDemoStyle::Shutdown();

			if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
			{
				FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				//LevelEditorModule.OnCaptureSingleFrameAnimSequence().Unbind();
				LevelEditorModule.OnTabManagerChanged().Remove(LevelEditorTabManagerChangedHandle);
			}
		}
#endif
	}

	void OnToolWindowClosed(const TSharedRef<SWindow>& Window, UBaseEditorTool* Instance)
	{
		Instance->RemoveFromRoot();
	}

	static TSharedRef<SDockTab> SpawnCognitiveSceneSetupTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		const TSharedRef<SDockTab> MajorTab =
			SNew(SDockTab)
			//.Icon(FEditorStyle::Get().GetBrush("SequenceRecorder.TabIcon"))
			.TabRole(ETabRole::MajorTab);

		MajorTab->SetContent(SNew(SSceneSetupWidget));

		return MajorTab;
	}

	FDelegateHandle LevelEditorTabManagerChangedHandle;
};
IMPLEMENT_MODULE(FCognitiveVREditorModule, CognitiveVREditor);

#undef LOCTEXT_NAMESPACE
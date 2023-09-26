#include "editor.h"
#include "editorConfig.h"
#include "assetBrowser.h"
#include <TFE_RenderBackend/renderBackend.h>
#include <TFE_System/system.h>
#include <TFE_FileSystem/fileutil.h>
#include <TFE_FileSystem/paths.h>
#include <TFE_Archive/archive.h>
#include <TFE_Ui/ui.h>

#include <TFE_Ui/imGUI/imgui.h>

namespace TFE_Editor
{
	enum EditorMode
	{
		EDIT_CONFIG = 0,
		EDIT_ASSET,
		EDIT_LEVEL,
	};
	
	static bool s_showPerf = true;
	static bool s_showEditor = true;
	static EditorMode s_editorMode = EDIT_ASSET;
	static EditorMode s_prevEditorMode = EDIT_ASSET;
	static bool s_exitEditor = false;
	static bool s_configView = false;
	static WorkBuffer s_workBuffer;
	
	void menu();

	WorkBuffer& getWorkBuffer()
	{
		return s_workBuffer;
	}

	void enable()
	{
		// Ui begin/render is called so we have a "UI Frame" in which to setup UI state.
		s_editorMode = EDIT_ASSET;
		s_exitEditor = false;
		loadConfig();
		AssetBrowser::init();
	}

	void disable()
	{
		AssetBrowser::destroy();
	}

	bool update(bool consoleOpen)
	{
		TFE_RenderBackend::clearWindow();
		menu();

		if (configSetupRequired())
		{
			s_editorMode = EDIT_CONFIG;
		}

		if (s_editorMode == EDIT_CONFIG)
		{
			if (configUi())
			{
				s_editorMode = s_prevEditorMode;
			}
		}
		else if (s_editorMode == EDIT_ASSET)
		{
			AssetBrowser::update();
		}
		
		return s_exitEditor;
	}

	bool render()
	{
		if (s_editorMode == EDIT_ASSET)
		{
			AssetBrowser::render();
		}
		return true;
	}

	bool beginMenuBar()
	{
		bool menuActive = true;

		DisplayInfo displayInfo;
		TFE_RenderBackend::getDisplayInfo(&displayInfo);

		ImGui::SetWindowPos("MainMenuBar", { 0.0f, 0.0f });
		ImGui::SetWindowSize("MainMenuBar", { (f32)displayInfo.width, 32.0f });
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::Begin("MainMenuBar", &menuActive, window_flags);
		return ImGui::BeginMenuBar();
	}

	void endMenuBar()
	{
		ImGui::EndMenuBar();
		ImGui::End();
	}
		
	void menu()
	{
		beginMenuBar();
		{
			// General menu items.
			if (ImGui::BeginMenu("Editor"))
			{
				if (ImGui::MenuItem("Editor Config", NULL, s_editorMode == EDIT_CONFIG))
				{
					s_prevEditorMode = s_editorMode;
					if (s_prevEditorMode == EDIT_CONFIG)
					{
						s_prevEditorMode = EDIT_ASSET;
					}
					s_editorMode = EDIT_CONFIG;
				}
				if (ImGui::MenuItem("Asset Browser", NULL, s_editorMode == EDIT_ASSET))
				{
					s_editorMode = EDIT_ASSET;
				}
				if (ImGui::MenuItem("Level Editor", NULL, s_editorMode == EDIT_LEVEL))
				{
					s_editorMode = EDIT_LEVEL;
				}
				if (ImGui::MenuItem("Return to Game", NULL, (bool*)NULL))
				{
					s_exitEditor = true;
				}
				if (ImGui::MenuItem("Exit", NULL, (bool*)NULL))
				{
				}
				ImGui::EndMenu();
			}
		}
		endMenuBar();
	}
}

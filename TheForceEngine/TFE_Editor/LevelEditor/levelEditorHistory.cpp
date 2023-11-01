#include "levelEditorHistory.h"
#include "sharedState.h"
#include <TFE_Editor/LevelEditor/levelEditor.h>
#include <TFE_Editor/LevelEditor/levelEditorData.h>
#include <assert.h>
#include <algorithm>

using namespace TFE_Editor;

// TODO: Capture edit mode and selection data when creating a delta/snapshot.

namespace LevelEditor
{
	enum LevCommand
	{
		LCmd_MoveVertex = CMD_START,
		LCmd_SetVertex,
		LCmd_MoveFlat,
		LCmd_InsertVertex,
		LCmd_Count
	};
		
	// Command Functions
	void cmd_applyMoveVertices();
	void cmd_applySetVertex();
	void cmd_applyMoveFlats();
	void cmd_applyInsertVertex();

	void levHistory_init()
	{
		history_init(level_unpackSnapshot, level_createSnapshot);

		history_registerCommand(LCmd_MoveVertex, cmd_applyMoveVertices);
		history_registerCommand(LCmd_SetVertex, cmd_applySetVertex);
		history_registerCommand(LCmd_MoveFlat, cmd_applyMoveFlats);
		history_registerCommand(LCmd_InsertVertex, cmd_applyInsertVertex);
		history_registerName(LName_MoveVertex, "Move Vertice(s)");
		history_registerName(LName_SetVertex, "Set Vertex Position");
		history_registerName(LName_MoveWall, "Move Wall(s)");
		history_registerName(LName_MoveFlat, "Move Flat(s)");
		history_registerName(LName_InsertVertex, "Insert Vertex");
	}

	void levHistory_destroy()
	{
		history_destroy();
	}

	void levHistory_clear()
	{
		history_clear();
	}

	void levHistory_createSnapshot(const char* name)
	{
		history_createSnapshot(name);
	}
		
	void levHistory_undo()
	{
		// TODO: Figure out how to track sector changes properly.
		history_step(-1);
	}

	void levHistory_redo()
	{
		// TODO: Figure out how to track sector changes properly.
		history_step(1);
	}
		
	////////////////////////////////
	// Command Helpers
	////////////////////////////////
	// Requires either 4 or 8 bytes.
	void captureFeature(Feature feature)
	{
		hBuffer_addS16((s16)feature.featureIndex);
		hBuffer_addS16((s16)feature.part);
		// Save 4 bytes if there is no feature.
		if (feature.featureIndex >= 0)
		{
			hBuffer_addS32(feature.sector ? feature.sector->id : -1);
		}
		// Previous sector is not captured since it is optional.
	}

	void restoreFeature(Feature& feature)
	{
		// Previous sector is not captured, so clear.
		// This is only a convenience for 2D editing and is not required.

		feature.featureIndex = (s32)hBuffer_getS16();
		feature.part = (HitPart)hBuffer_getS16();
		feature.sector = nullptr;
		feature.prevSector = nullptr;
		if (feature.featureIndex >= 0)
		{
			s32 sectorId = hBuffer_getS32();
			feature.sector = sectorId < 0 ? nullptr : &s_level.sectors[sectorId];
		}
	}
		
	// First number is single instance, second is 65536 states with the same settings.
	// Min (no features): 16 bytes, 1.0Mb
	//        (features): 24 bytes, 1.5Mb
	// Assuming 10 items selected: 104 bytes, 6.5Mb
	// Most common case: 24 bytes + 1 selected = 32 bytes (2.0Mb @ 64k).
	void captureEditState()
	{
		hBuffer_addU32((u32)s_editMode);
		hBuffer_addU32((u32)s_selectionList.size());
		if (!s_selectionList.empty())
		{
			hBuffer_addArrayU64((u32)s_selectionList.size(), s_selectionList.data());
		}
		captureFeature(s_featureHovered);
		captureFeature(s_featureCur);
	}

	void restoreEditState()
	{
		s_editMode = (LevelEditMode)hBuffer_getU32();
		u32 selectionCount = hBuffer_getU32();
		s_selectionList.resize(selectionCount);
		if (selectionCount)
		{
			memcpy(s_selectionList.data(), hBuffer_getArrayU64(selectionCount), sizeof(u64) * selectionCount);
		}
		restoreFeature(s_featureHovered);
		restoreFeature(s_featureCur);
		// Clear the current wall, it will be reset when needed automatically.
		s_featureCurWall = {};
	}

	#define CMD_APPLY_BEGIN()
	#define CMD_APPLY_END() restoreEditState()

	#define CMD_BEGIN(cmdId, cmdName) if (!history_createCommand(cmdId, cmdName)) { return; }
	#define CMD_END() captureEditState()

	////////////////////////////////
	// Commands
	////////////////////////////////
	// cmd_add*   - called from the level editor to add the command to the history.
	// cmd_apply* - called from the history/undo/redo system to apply the command from the history.
	
	/////////////////////////////////
	// Move Vertices
	void cmd_addMoveVertices(s32 count, const FeatureId* vertices, Vec2f delta, LevCommandName name)
	{
		CMD_BEGIN(LCmd_MoveVertex, name);
		// Add the command data.
		hBuffer_addS32(count);
		hBuffer_addArrayU64(count, vertices);
		hBuffer_addVec2f(delta);
		CMD_END();
	}

	void cmd_applyMoveVertices()
	{
		CMD_APPLY_BEGIN();
		// Extract the command data.
		const s32 count = hBuffer_getS32();
		const FeatureId* vertices = (FeatureId*)hBuffer_getArrayU64(count);
		const Vec2f delta = hBuffer_getVec2f();
		// Call the editor command.
		edit_moveVertices(count, vertices, delta);
		CMD_APPLY_END();
	}

	/////////////////////////////////
	// Set Vertex
	void cmd_addSetVertex(FeatureId vertex, Vec2f pos)
	{
		CMD_BEGIN(LCmd_SetVertex, LName_SetVertex);
		// Add the command data.
		hBuffer_addU64(vertex);
		hBuffer_addVec2f(pos);
		CMD_END();
	}

	void cmd_applySetVertex()
	{
		CMD_APPLY_BEGIN();
		// Extract the command data.
		const FeatureId id = (FeatureId)hBuffer_getU64();
		const Vec2f pos = hBuffer_getVec2f();
		// Call the editor command.
		edit_setVertexPos(id, pos);
		CMD_APPLY_END();
	}

	/////////////////////////////////
	// Move Flats
	void cmd_addMoveFlats(s32 count, const FeatureId* flats, f32 delta)
	{
		CMD_BEGIN(LCmd_MoveFlat, LName_MoveFlat);
		// Add the command data.
		hBuffer_addS32(count);
		hBuffer_addArrayU64(count, flats);
		hBuffer_addF32(delta);
		CMD_END();
	}

	void cmd_applyMoveFlats()
	{
		CMD_APPLY_BEGIN();
		// Extract the command data.
		const s32 count = hBuffer_getS32();
		const FeatureId* flats = (FeatureId*)hBuffer_getArrayU64(count);
		const f32 delta = hBuffer_getF32();
		// Call the editor command.
		edit_moveFlats(count, flats, delta);
		CMD_APPLY_END();
	}

	/////////////////////////////////
	// Insert Vertex
	void cmd_addInsertVertex(s32 sectorIndex, s32 wallIndex, Vec2f newPos)
	{
		CMD_BEGIN(LCmd_InsertVertex, LName_InsertVertex);
		// Add the command data.
		hBuffer_addS32(sectorIndex);
		hBuffer_addS32(wallIndex);
		hBuffer_addVec2f(newPos);
		CMD_END();
	}

	void cmd_applyInsertVertex()
	{
		CMD_APPLY_BEGIN();
		// Extract the command data.
		const s32 sectorIndex = hBuffer_getS32();
		const s32 wallIndex = hBuffer_getS32();
		const Vec2f newPos = hBuffer_getVec2f();
		// Call the editor command.
		edit_splitWall(sectorIndex, wallIndex, newPos);
		CMD_APPLY_END();
	}
}
